
#pragma once

class Oscillator
{
public:
    void prepare(double sampleRate);
    void setFrequency(double newFrequency);
    float getNextSample();

private:
    double frequency = 440.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    double currentSampleRate = 44100.0;

    void updatePhaseIncrement();
};
