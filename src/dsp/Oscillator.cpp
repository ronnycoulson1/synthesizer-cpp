
#include "Oscillator.h"
#include <cmath>

namespace
{
    constexpr double twoPi = 6.283185307179586;
}

void Oscillator::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    phase = 0.0;
    updatePhaseIncrement();
}

void Oscillator::setFrequency(double newFrequency)
{
    frequency = newFrequency;
    updatePhaseIncrement();
}

void Oscillator::updatePhaseIncrement()
{
    phaseIncrement = twoPi * frequency / currentSampleRate;
}

float Oscillator::getNextSample()
{
    float sample = static_cast<float>(std::sin(phase));

    phase += phaseIncrement;

    if (phase >= twoPi)
        phase -= twoPi;

    return sample;
}
