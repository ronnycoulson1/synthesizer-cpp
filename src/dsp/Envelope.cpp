#include "Envelope.h"
#include <algorithm>

void::Envelope::prepare(double newSampleRate){
    sampleRate = newSampleRate;
    stage = Stage::Idle;
    level = 0.0;
    stageTime = 0.0;
}

void Envelope::setAttack(double seconds){
    attack = std::max(0.001, seconds);
}

void Envelope::setDecay(double seconds){
    decay = std::max(0.001, seconds);
}

void Envelope::setSustain(float newLevel){
    sustain = std::clamp(newLevel, 0.0f, 1.0f);
}

void Envelope::setRelease(double seconds){
    release = std::max(0.001, seconds);
}

void Envelope::noteOff(){
    if (stage == Stage::Idle){
        return;
    }
    releaseStartLevel = level;
    stage = Stage::Release;
    stageTime = 0.0;
}


void Envelope::noteOn()
{
    // Start the Attack stage
    stage = Stage::Attack;

    // Reset the timer for this stage
    stageTime = 0.0;
}


float Envelope::getNextSample(){
    const double dt = 1.0 / sampleRate;

    switch(stage){
        case Stage::Idle:
            level = 0.0;
            break;
        
        case Stage::Attack:
            stageTime += dt;
            level = std::min(1.0, stageTime / attack );

            if (stageTime >= attack){
                stage = Stage::Decay;
                stageTime = 0.0;
            }
            break;
        case Stage::Decay:
            stageTime += dt;
            level = 1.0 - (1.0 - sustain) * std::min(1.0, stageTime / decay);

            if (stageTime >= decay){
                stage = Stage::Sustain;
                level = sustain;
            }
            break;
        case Stage::Sustain:
            level = sustain;
            break;

        case Stage::Release:
            stageTime += dt;
            level = releaseStartLevel * (1.0 - std::min(1.0, stageTime / release));

            if(stageTime >= release){
                stage = Stage::Idle;
                level = 0.0;
            }
            break;
    }
    return static_cast<float>(level);
}