
#include <atomic>
#include <memory>
#include <juce_audio_utils/juce_audio_utils.h>

#include "dsp/Oscillator.h"
#include "dsp/Filter.h"
#include "dsp/Envelope.h"

// ============================================
// SYNTHESIZER COMPONENT
// ============================================

class SynthComponent : public juce::AudioAppComponent,
                        private juce::MidiKeyboardStateListener
{
public:
    SynthComponent()
    {
        //display virtual midi keyboard 
        addAndMakeVisible(midiKeyboard);

        //start at middle c (midii note 60)

    // Display virtual MIDI keyboard
    addAndMakeVisible(midiKeyboard);

    // Set keyboard note range
    midiKeyboard.setAvailableRange(48, 84);
    midiKeyboard.setLowestVisibleKey(48);

    // Connect MIDI keyboard to our synthesizer
    midiKeyboardState.addListener(this);


        // Window dimensions
        setSize(660, 550);

        // ====================================
        // PLAY / STOP BUTTON
        // ====================================

        addAndMakeVisible(playButton);
        playButton.setButtonText("Play");

        playButton.onClick = [this]()
        {
            bool shouldPlay = !isPlaying.load();

            isPlaying.store(shouldPlay);

            playButton.setButtonText(
                shouldPlay ? "Stop" : "Play"
            );
        };

        // ====================================
        // FREQUENCY KNOB
        // ====================================

        addAndMakeVisible(frequencySlider);

        frequencySlider.setSliderStyle(
            juce::Slider::RotaryHorizontalVerticalDrag
        );

        frequencySlider.setTextBoxStyle(
            juce::Slider::TextBoxBelow,
            false,
            100,
            25
        );

        frequencySlider.setRange(50.0, 2000.0, 1.0);
        frequencySlider.setValue(440.0);
        frequencySlider.setTextValueSuffix(" Hz");

        frequencySlider.onValueChange = [this]()
        {
            targetFrequency.store(
                frequencySlider.getValue()
            );
        };

        // ====================================
        // WAVEFORM SELECTOR
        // ====================================

        addAndMakeVisible(waveformSelector);

        waveformSelector.addItem("Sine", 1);
        waveformSelector.addItem("Square", 2);
        waveformSelector.addItem("Sawtooth", 3);

        waveformSelector.setSelectedId(1);

        waveformSelector.onChange = [this]()
        {
            selectedWaveform.store(
                waveformSelector.getSelectedId() - 1
            );
        };

        // ====================================
        // FILTER CUTOFF KNOB
        // ====================================

        addAndMakeVisible(cutoffSlider);

        cutoffSlider.setSliderStyle(
            juce::Slider::RotaryHorizontalVerticalDrag
        );

        cutoffSlider.setTextBoxStyle(
            juce::Slider::TextBoxBelow,
            false,
            100,
            25
        );

        cutoffSlider.setRange(100.0, 10000.0, 1.0);

        cutoffSlider.setSkewFactorFromMidPoint(
            1000.0
        );

        cutoffSlider.setValue(1000.0);
        cutoffSlider.setTextValueSuffix(" Hz");

        cutoffSlider.onValueChange = [this]()
        {
            targetCutoff.store(
                cutoffSlider.getValue()
            );
        };

        // ADSR slider 
        juce::Slider* adsrSliders[] = {
            &attackSlider,
            &decaySlider,
            &sustainSlider,
            &releaseSlider
        }; 

        for (auto* slider : adsrSliders){
            addAndMakeVisible(*slider);

            slider->setSliderStyle(
                juce::Slider::RotaryHorizontalVerticalDrag
            );
            slider->setTextBoxStyle(
                juce::Slider::TextBoxBelow,
                false,
                75,
                22
            );
        }

        //atack: 1 ms to 2s
        attackSlider.setRange(0.001, 2.0, 0.001);
        attackSlider.setValue(0.05);
        attackSlider.setTextValueSuffix(" s");

        // Decay: 1 ms to 2 seconds
        decaySlider.setRange(0.001, 2.0, 0.001);
        decaySlider.setValue(0.20);
        decaySlider.setTextValueSuffix(" s");

        // Sustain: 0% to 100%
        sustainSlider.setRange(0.0, 1.0, 0.01);
        sustainSlider.setValue(0.50);

        // Release: 1 ms to 3 seconds
        releaseSlider.setRange(0.001, 3.0, 0.001);
        releaseSlider.setValue(0.30);
        releaseSlider.setTextValueSuffix(" s");

        //send knob changes to audio thread 

        attackSlider.onValueChange = [this](){
            targetAttack.store(attackSlider.getValue());
        };

        decaySlider.onValueChange = [this] () {
            targetDecay.store(decaySlider.getValue());
        };

        sustainSlider.onValueChange = [this] (){
            targetSustain.store(sustainSlider.getValue());
        };

        releaseSlider.onValueChange = [this]()
        {
            targetRelease.store(releaseSlider.getValue());
        };




        // Initialize stereo audio output
        setAudioChannels(0, 2);
    }

    // ========================================
    // DESTRUCTOR
    // ========================================

    ~SynthComponent() override
    {
        midiKeyboardState.removeListener(this);
        shutdownAudio();
    }

    // ========================================
    // PREPARE AUDIO
    // ========================================


void prepareToPlay(int, double newSampleRate) override
{
    // Initialize oscillator
    oscillator.prepare(newSampleRate);

    currentFrequency = targetFrequency.load();
    oscillator.setFrequency(currentFrequency);

    // Initialize low-pass filter
    lowPassFilter.prepare(newSampleRate);
    lowPassFilter.setCutoff(targetCutoff.load());

    // Initialize ADSR envelope
    amplitudeEnvelope.prepare(newSampleRate);

    // Read values from the ADSR knobs
    amplitudeEnvelope.setAttack(targetAttack.load());
    amplitudeEnvelope.setDecay(targetDecay.load());
    amplitudeEnvelope.setSustain(
    static_cast<float>(targetSustain.load()));
    amplitudeEnvelope.setRelease(targetRelease.load());

    envelopeGateOpen = false;
}


    // ========================================
    // AUDIO PROCESSING
    // ========================================

    void getNextAudioBlock(
        const juce::AudioSourceChannelInfo& bufferToFill
    ) override
    {
        auto* buffer = bufferToFill.buffer;

        // Update filter cutoff
        lowPassFilter.setCutoff(
            targetCutoff.load()
        );

        //updated adsr params from the gui knob
        amplitudeEnvelope.setAttack(targetAttack.load());
        amplitudeEnvelope.setDecay(targetDecay.load());
       amplitudeEnvelope.setSustain(static_cast<float>(targetSustain.load()));
        amplitudeEnvelope.setRelease(targetRelease.load());



        // Read selected waveform
        const int waveformIndex =
            selectedWaveform.load();

        switch (waveformIndex)
        {
            case 0:
                oscillator.setWaveform(
                    Oscillator::Waveform::Sine
                );
                break;

            case 1:
                oscillator.setWaveform(
                    Oscillator::Waveform::Square
                );
                break;

            case 2:
                oscillator.setWaveform(
                    Oscillator::Waveform::Saw
                );
                break;

            default:
                oscillator.setWaveform(
                    Oscillator::Waveform::Sine
                );
                break;
        }

        // Frequency smoothing coefficient
        const double smoothing = 0.001;
        
       
        const bool requestedPlaying =
            isPlaying.load() || midiGate.load();

        // Check whether a new MIDI key was pressed
        const unsigned int currentTriggerCount =
            midiTriggerCount.load();

        const bool newMidiNote =
            currentTriggerCount != lastMidiTriggerCount;

        // Remember the latest trigger
        lastMidiTriggerCount = currentTriggerCount;

        // Start a new Attack for each MIDI key press
        if (newMidiNote && midiGate.load())
        {
            amplitudeEnvelope.noteOn();
            envelopeGateOpen = true;
        }
        // Otherwise, handle normal Play/Stop transitions
        else if (requestedPlaying != envelopeGateOpen)
        {
            if (requestedPlaying)
            {
                amplitudeEnvelope.noteOn();
            }
            else
            {
                amplitudeEnvelope.noteOff();
            }

            envelopeGateOpen = requestedPlaying;
        }





        // Generate audio
        for (
            int sample = 0;
            sample < bufferToFill.numSamples;
            ++sample
        )
        {


            const int note = midiNote.load();
            const bool keyHeld = midiGate.load();

            if (keyHeld && note >= 0)
            {
                // Play the MIDI note at its exact frequency
                currentFrequency =
                    juce::MidiMessage::getMidiNoteInHertz(note);
            }
            else if (isPlaying.load())
            {
                // Use the frequency knob for manual playback
                currentFrequency = targetFrequency.load();
            }

            // Set the oscillator frequency
            oscillator.setFrequency(currentFrequency);



            // Generate oscillator sample
            float rawSample = oscillator.getNextSample();

            // Apply low-pass filter
            float filteredSample =
                lowPassFilter.processSample(rawSample);

            // Get the current ADSR amplitude
            float envelopeLevel =
                amplitudeEnvelope.getNextSample();

            // Apply envelope to the filtered waveform
            float value =
                0.15f * filteredSample * envelopeLevel;



            // Write to left and right channels
            for (
                int channel = 0;
                channel < buffer->getNumChannels();
                ++channel
            )
            {
                buffer->setSample(
                    channel,
                    bufferToFill.startSample + sample,
                    value
                );
            }
        }
    }

    void releaseResources() override
    {
    }

    // ========================================
    // USER INTERFACE
    // ========================================

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);

        g.setColour(juce::Colours::white);
        g.setFont(24.0f);

        g.drawFittedText(
            "Syntaziser",
            0,
            25,
            getWidth(),
            50,
            juce::Justification::centred,
            1
        );
    }


    void resized() override
    {
        // Oscillator and filter controls
        frequencySlider.setBounds(125, 80, 150, 115);
        cutoffSlider.setBounds(375, 80, 150, 115);

        // ADSR controls
        attackSlider.setBounds(35, 230, 130, 110);
        decaySlider.setBounds(185, 230, 130, 110);
        sustainSlider.setBounds(335, 230, 130, 110);
        releaseSlider.setBounds(485, 230, 130, 110);

        // Waveform selector and Play button
        waveformSelector.setBounds(255, 190, 140, 25);
        playButton.setBounds(275, 365, 100, 38);

        //virtual piano 
        midiKeyboard.setBounds(30,430,590, 90);
    }


private:
    // DSP components
    Oscillator oscillator;
    Filter lowPassFilter;
    Envelope amplitudeEnvelope;
    bool envelopeGateOpen = false;



    // Interface controls
    juce::TextButton playButton;
    juce::Slider frequencySlider;
    juce::Slider cutoffSlider;
    juce::ComboBox waveformSelector;


    // ADSR envelope knobs
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;


    //values shared between GUI and audio 
    std::atomic<double> targetAttack {0.05};
    std::atomic<double> targetDecay {0.20};
    std::atomic<double> targetSustain {0.50};
    std::atomic<double> targetRelease {0.30};

    // Thread-safe playback state
    std::atomic<bool> isPlaying { false };

    // Frequency controls
    std::atomic<double> targetFrequency { 440.0 };
    double currentFrequency = 440.0;

    // Waveform selection
    std::atomic<int> selectedWaveform { 0 };

    // Filter cutoff control
    std::atomic<double> targetCutoff { 1000.0 };



    

    // MIDI KEYBOARD 
    juce::MidiKeyboardState midiKeyboardState;

    juce::MidiKeyboardComponent midiKeyboard {
        midiKeyboardState,
        juce::MidiKeyboardComponent::horizontalKeyboard
    };


    std::atomic<int> midiNote {-1};
    std::atomic<bool> midiGate {false};
    std::atomic<unsigned int> midiTriggerCount {0};
    unsigned int lastMidiTriggerCount = 0;


    
    void handleNoteOn(
        juce::MidiKeyboardState*,
        int,
        int midiNoteNumber,
        float
    ) override{
        // Store the note being played
        midiNote.store(midiNoteNumber);

        // Open the MIDI gate
        midiGate.store(true);

        // Signal a new note press
        midiTriggerCount.fetch_add(1);
    }


    void handleNoteOff(
        juce::MidiKeyboardState*,
        int,
        int midiNoteNumber,
        float
    ) override {
        if (midiNote.load() == midiNoteNumber){
            midiGate.store(false);
        }
    }





};

// ============================================
// APPLICATION
// ============================================

class SyntaziserApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override
    {
        return "Syntaziser";
    }

    const juce::String getApplicationVersion() override
    {
        return "0.4.0";
    }

    void initialise(const juce::String&) override
    {
        mainWindow.reset(
            new MainWindow(getApplicationName())
        );
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

private:
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(
                name,
                juce::Colours::black,
                allButtons
            )
        {
            setUsingNativeTitleBar(true);

            setContentOwned(
                new SynthComponent(),
                true
            );

            centreWithSize(
                getWidth(),
                getHeight()
            );

            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()
                ->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(SyntaziserApplication)
