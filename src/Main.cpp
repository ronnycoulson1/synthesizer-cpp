
#include <atomic>
#include <memory>
#include <juce_audio_utils/juce_audio_utils.h>

#include "dsp/Oscillator.h"
#include "dsp/Filter.h"

// ============================================
// SYNTHESIZER COMPONENT
// ============================================

class SynthComponent : public juce::AudioAppComponent
{
public:
    SynthComponent()
    {
        // Window dimensions
        setSize(500, 300);

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

        // Initialize stereo audio output
        setAudioChannels(0, 2);
    }

    // ========================================
    // DESTRUCTOR
    // ========================================

    ~SynthComponent() override
    {
        shutdownAudio();
    }

    // ========================================
    // PREPARE AUDIO
    // ========================================

    void prepareToPlay(
        int,
        double newSampleRate
    ) override
    {
        oscillator.prepare(newSampleRate);

        currentFrequency = targetFrequency.load();

        oscillator.setFrequency(currentFrequency);

        lowPassFilter.prepare(newSampleRate);

        lowPassFilter.setCutoff(
            targetCutoff.load()
        );
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

        // Generate audio
        for (
            int sample = 0;
            sample < bufferToFill.numSamples;
            ++sample
        )
        {
            const double target =
                targetFrequency.load();

            currentFrequency += smoothing *
                (target - currentFrequency);

            oscillator.setFrequency(
                currentFrequency
            );

            float value = 0.0f;

            if (isPlaying.load())
            {
                // Generate waveform
                float rawSample =
                    oscillator.getNextSample();

                // Apply low-pass filter
                float filteredSample =
                    lowPassFilter.processSample(rawSample);

                // Control output amplitude
                value = 0.15f * filteredSample;
            }

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
        frequencySlider.setBounds(
            70, 85, 140, 115
        );

        cutoffSlider.setBounds(
            290, 85, 140, 115
        );

        waveformSelector.setBounds(
            180, 205, 140, 25
        );

        playButton.setBounds(
            200, 250, 100, 35
        );
    }

private:
    // DSP components
    Oscillator oscillator;
    Filter lowPassFilter;

    // Interface controls
    juce::TextButton playButton;
    juce::Slider frequencySlider;
    juce::Slider cutoffSlider;
    juce::ComboBox waveformSelector;

    // Thread-safe playback state
    std::atomic<bool> isPlaying { false };

    // Frequency controls
    std::atomic<double> targetFrequency { 440.0 };
    double currentFrequency = 440.0;

    // Waveform selection
    std::atomic<int> selectedWaveform { 0 };

    // Filter cutoff control
    std::atomic<double> targetCutoff { 1000.0 };
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
