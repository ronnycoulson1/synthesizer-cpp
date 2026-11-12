#pragma once 

class Envelope
{
    public:
    void prepare(double sampleRate);

    void setAttack(double seconds);
    void setDecay(double seconds);
    void setSustain(float level);
    void setRelease(double seconds);

    void noteOn();
    void noteOff();

    float getNextSample();

    private:
    enum class Stage {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    Stage stage = Stage::Idle;
    double sampleRate = 44100.0;
    double attack = 0.1;
    double decay = 0.2;
    float sustain = 0.5;
    double release = 0.3;

    double level = 0.0;
    double releaseStartLevel = 0.0;
    double stageTime = 0.0;

};