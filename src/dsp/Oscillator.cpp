
#include "Oscillator.h"
#include <cmath>

// Mathematical constants
namespace
{
    constexpr double twoPi = 6.283185307179586;
}

// Initialize the oscillator
void Oscillator::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    phase = 0.0;

    updatePhaseIncrement();
}

// Change the oscillator frequency
void Oscillator::setFrequency(double newFrequency)
{
    frequency = newFrequency;

    updatePhaseIncrement();
}

// Change the oscillator waveform
void Oscillator::setWaveform(Waveform newWaveform)
{
    waveform = newWaveform;
}

// Calculate how much phase advances per sample
void Oscillator::updatePhaseIncrement()
{
    phaseIncrement = twoPi * frequency / currentSampleRate;
}

// Generate one audio sample
float Oscillator::getNextSample()
{
    float sample = 0.0f;

    // Choose which waveform to generate
    switch (waveform)
    {
        // SINE WAVE
        case Waveform::Sine:
        {
            sample = static_cast<float>(
                std::sin(phase)
            );
            break;
        }

        // SQUARE WAVE
        case Waveform::Square:
        {
            sample = (phase < twoPi / 2.0)
                ? 1.0f
                : -1.0f;
            break;
        }

        // SAWTOOTH WAVE
        case Waveform::Saw:
        {
            sample = static_cast<float>(
                2.0 * (phase / twoPi) - 1.0
            );
            break;
        }
    }

    // Move forward through the waveform
    phase += phaseIncrement;

    // Wrap phase after one complete cycle
    if (phase >= twoPi)
    {
        phase = std::fmod(phase, twoPi);
    }

    return sample;
}
