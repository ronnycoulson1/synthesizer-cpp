#pragma once 

class Filter {
    public:

        void prepare(double sampleRate);
        void setCutoff(double newCutoff);
        float processSample(float input);


    private: 
        double currentSampleRate = 44100.0;
        double cutoff = 1000.0;
        double alpha = 0.0;
        double previousOutput = 0.0;

        void updateCoefficient();
};