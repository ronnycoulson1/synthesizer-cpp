
#include <atomic>
#include <juce_audio_utils/juce_audio_utils.h>
#include "dsp/Oscillator.h"

class SynthComponent : public juce::AudioAppComponent
{
public:
    SynthComponent()
    {
        setSize(400, 200);
        //play / stop btn 
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

        //frequency knob 
        addAndMakeVisible(frequencySlider);

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
            targetFrequency.store(frequencySlider.getValue());
        };
        //initialize audio 
        setAudioChannels(0, 2);
    }

    ~SynthComponent() override
    {
        shutdownAudio();
    }

    void prepareToPlay(int, double newSampleRate) override
    {
        oscillator.prepare(newSampleRate);
        currentFrequency = targetFrequency.load();
        oscillator.setFrequency(currentFrequency);
    }



void getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill) override
{
    // Access the audio buffer
    auto* buffer = bufferToFill.buffer;

    // Frequency smoothing
    const double smoothing = 0.001;

    // Generate audio sample by sample
    for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
    {
        // Read the frequency selected by the knob
        const double target = targetFrequency.load();

        // Smoothly move toward the target frequency
        currentFrequency += smoothing * (target - currentFrequency);

        // Update our oscillator's frequency
        oscillator.setFrequency(currentFrequency);

        // Start with silence
        float value = 0.0f;

        // Generate sound only when Play is active
        if (isPlaying.load())
        {
            value = 0.15f * oscillator.getNextSample();
        }

        // Send the same sound to left and right channels
        for (int channel = 0; channel < buffer->getNumChannels(); ++channel)
        {
            buffer->setSample(
                channel,
                bufferToFill.startSample + sample,
                value
            );
        }
    }
}



    void releaseResources() override {}

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);

        g.setColour(juce::Colours::white);
        g.setFont(24.0f);

        g.drawFittedText(
            "Syntaziser - 440 Hz",
            0, 30, getWidth(), 50,
            juce::Justification::centred,
            1
        );
    }

    void resized() override
    {
        frequencySlider.setBounds(135, 65, 130, 100);
        playButton.setBounds(150, 165, 100, 30);
    }

private:
    Oscillator oscillator;
    
    juce::TextButton playButton; // user interface 
    juce::Slider frequencySlider; // user interface
    juce::Label frequencyLabel; // user interface

    std::atomic<bool> isPlaying { false }; //audio playback state

    //frequency controls
    std::atomic<double> targetFrequency { 440.0 };
    double currentFrequency = 440.0;
};

class SyntaziserApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override
    {
        return "Syntaziser";
    }

    const juce::String getApplicationVersion() override
    {
        return "0.2.0";
    }

    void initialise(const juce::String&) override
    {
        mainWindow.reset(new MainWindow(getApplicationName()));
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
                allButtons)
        {
            setUsingNativeTitleBar(true);

            setContentOwned(
                new SynthComponent(),
                true
            );

            centreWithSize(getWidth(), getHeight());
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
