#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>

class SynthComponent : public juce::AudioAppComponent
{
public:
    SynthComponent()
    {
        setSize(400, 200);
        setAudioChannels(0, 2);
    }

    ~SynthComponent() override
    {
        shutdownAudio();
    }

    void prepareToPlay(int, double newSampleRate) override
    {
        phase = 0.0;
        phaseIncrement = 2.0 * juce::MathConstants<double>::pi
                       * frequency / newSampleRate;
    }

    void getNextAudioBlock(
        const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        auto* buffer = bufferToFill.buffer;

        for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
        {
            //oscillator generates sine-wave
            float value = static_cast<float>(0.15 * std::sin(phase));

            phase += phaseIncrement;

            if (phase >= 2.0 * juce::MathConstants<double>::pi)
                phase -= 2.0 * juce::MathConstants<double>::pi;

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
        g.drawFittedText("Syntaziser - 440 Hz",
                         getLocalBounds(),
                         juce::Justification::centred,
                         1);
    }

private:
    double frequency = 440.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
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
        return "0.1.0";
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
            : DocumentWindow(name,
                             juce::Colours::black,
                             allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new SynthComponent(), true);
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
