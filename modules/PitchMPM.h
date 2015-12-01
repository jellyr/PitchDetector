#include "JuceHeader.h"
#include <float.h>

#define CUTOFF 0.93 //0.97 is default
#define SMALL_CUTOFF 0.5
#define LOWER_PITCH_CUTOFF 80 //hz

class PitchMPM
{

public:

    PitchMPM(int bufferSize) : bufferSize (bufferSize), sampleRate (44100)
    {
        PitchMPM(bufferSize, sampleRate);
    }
    
    PitchMPM(int sampleRate, int bufferSize) : bufferSize (bufferSize), sampleRate (sampleRate)
    {
        nsdf.insertMultiple(0, 0.0, bufferSize);
    }
    
    
    ~PitchMPM()
    {
        nsdf.clear();
        maxPositions.clear();
        ampEstimates.clear();
        periodEstimates.clear();
        
    }
    
    float getPitch(float *audioBuffer)
    {
        
        float pitch;
        
        maxPositions.clearQuick();
        periodEstimates.clearQuick();
        ampEstimates.clearQuick();
        
        nsdfTimeDomain(audioBuffer);
        
        peakPicking();
        
        float highestAmplitude = -FLT_MAX;
        
        for (auto tau : maxPositions)
        {
            highestAmplitude = jmax(highestAmplitude, nsdf[tau]);
            
            if (nsdf[tau] > SMALL_CUTOFF)
            {
                parabolicInterpolation(tau);
                ampEstimates.add (turningPointY);
                periodEstimates.add (turningPointX);
                highestAmplitude = jmax (highestAmplitude, turningPointY);
            }
        }
        
        if (periodEstimates.size() == 0)
        {
            pitch = -1;
        }
        else
        {
            float actualCutoff = CUTOFF * highestAmplitude;
            
            int periodIndex = 0;
            for (int i = 0; i < ampEstimates.size(); i++)
            {
                if (ampEstimates[i] >= actualCutoff)
                {
                    periodIndex = i;
                    break;
                }
            }
            
            float period = periodEstimates[periodIndex];
            float pitchEstimate = (sampleRate / period);
            if (pitchEstimate > LOWER_PITCH_CUTOFF)
            {
                pitch = pitchEstimate;
            }
            else
            {
                pitch = -1;
            }
        }
        return pitch;
    }
    
    void setSampleRate (int newSampleRate)
    {
        sampleRate = newSampleRate;
    }

private:
    int bufferSize;
    float sampleRate;
    
    float turningPointX, turningPointY;
    Array<float> nsdf;
    
    Array<int> maxPositions;
    Array<float> periodEstimates;
    Array<float> ampEstimates;
    
//    int maxPositionsIndex, periodEstimatesIndex, ampEstimatesIndex;
    
    void parabolicInterpolation(int tau)
    {
        
        float nsdfa = nsdf[tau - 1];
        float nsdfb = nsdf[tau];
        float nsdfc = nsdf[tau + 1];
        float bValue = tau;
        float bottom = nsdfc + nsdfa - 2 * nsdfb;
        if (bottom == 0.0)
        {
            turningPointX = bValue;
            turningPointY = nsdfb;
        }
        else
        {
            float delta = nsdfa - nsdfc;
            turningPointX = bValue + delta / (2 * bottom);
            turningPointY = nsdfb - delta * delta / (8 * bottom);
        }
    }
    
    void peakPicking()
    {
        
        int pos = 0;
        int curMaxPos = 0;
        
        while (pos < (bufferSize - 1) / 3 && nsdf.getUnchecked(pos) > 0) {
            pos++;
        }
        
        while (pos < bufferSize - 1 && nsdf.getUnchecked(pos) <= 0.0) {
            pos++;
        }
        
        if (pos == 0) {
            pos = 1;
        }
        
        while (pos < bufferSize - 1) {
            if (nsdf.getUnchecked(pos) > nsdf.getUnchecked(pos - 1) && nsdf.getUnchecked(pos) >= nsdf.getUnchecked(pos + 1)) {
                if (curMaxPos == 0) {
                    curMaxPos = pos;
                } else if (nsdf.getUnchecked(pos) > nsdf.getUnchecked(curMaxPos)) {
                    curMaxPos = pos;
                }
            }
            pos++;
            if (pos < bufferSize - 1 && nsdf.getUnchecked(pos) <= 0)
            {
                if (curMaxPos > 0)
                {
                    maxPositions.add (curMaxPos);
                    curMaxPos = 0;
                }
                while (pos < bufferSize - 1 && nsdf.getUnchecked(pos) <= 0.0f)
                {
                    pos++;
                }
            }
        }
        if (curMaxPos > 0)
        {
            maxPositions.add (curMaxPos);
        }
    }
    
    void nsdfTimeDomain(float *audioBuffer)
    {
        int tau;
        for (tau = 0; tau < bufferSize; tau++) {
            float acf = 0;
            float divisorM = 0;
            for (int i = 0; i < bufferSize - tau; i++) {
                acf += audioBuffer[i] * audioBuffer[i + tau];
                divisorM += audioBuffer[i] * audioBuffer[i] + audioBuffer[i + tau] * audioBuffer[i + tau];
            }
            nsdf.setUnchecked(tau, 2 * acf / divisorM);
        }
    }

};
