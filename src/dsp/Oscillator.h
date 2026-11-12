
#pragma once

class Oscillator
{
public:
    enum class Waveform
    {
        Sine,
        Square,
        Saw
    };

    void prepare(double sampleRate);
    void setFrequency(double newFrequency);
    void setWaveform(Waveform newWaveform);

    float getNextSample();

private:
    double frequency = 440.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    double currentSampleRate = 44100.0;

    Waveform waveform = Waveform::Sine;

    void updatePhaseIncrement();
};
