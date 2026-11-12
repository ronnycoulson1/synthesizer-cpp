
#include "Filter.h"
#include <cmath>
#include <algorithm>

// Mathematical constant
namespace
{
    constexpr double twoPi = 6.283185307179586;
}

// Initialize the filter
void Filter::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    previousOutput = 0.0;

    updateCoefficient();
}

// Change the cutoff frequency
void Filter::setCutoff(double newCutoff)
{
    cutoff = std::clamp(
        newCutoff,
        20.0,
        currentSampleRate * 0.45
    );

    updateCoefficient();
}

// Calculate the filter coefficient
void Filter::updateCoefficient()
{
    alpha = 1.0 - std::exp(
        -twoPi * cutoff / currentSampleRate
    );
}

// Process one audio sample
float Filter::processSample(float input)
{
    previousOutput += alpha * (input - previousOutput);

    return static_cast<float>(previousOutput);
}
