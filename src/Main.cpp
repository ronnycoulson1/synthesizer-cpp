
#include <atomic>
#include <juce_audio_utils/juce_audio_utils.h>
#include "dsp/Oscillator.h"

class SynthComponent : public juce::AudioAppComponent
{
public:
    SynthComponent()
    {
        setSize(400, 200);

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

        setAudioChannels(0, 2);
    }

    ~SynthComponent() override
    {
        shutdownAudio();
    }

    void prepareToPlay(int, double newSampleRate) override
    {
        oscillator.prepare(newSampleRate);
    }

    void getNextAudioBlock(
        const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        auto* buffer = bufferToFill.buffer;

        for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
        {
            float value = 0.0f;

            if (isPlaying.load())
            {
                value = 0.15f * oscillator.getNextSample();
            }

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
        playButton.setBounds(150, 120, 100, 40);
    }

private:
    Oscillator oscillator;
    juce::TextButton playButton;
    std::atomic<bool> isPlaying { false };
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
