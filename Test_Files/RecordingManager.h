#pragma once
#include <string>

class RecordingManager
{
public:
    void startRecording()
    {
        isRecording = true;
        reachedMax = false;
        sampleCount = 0; // ← clears previous recording
    }

    void stopRecording()
    {
        isRecording = false;
    }

    void simulateMaxDuration()
    {
        isRecording = false;
        reachedMax = true;
    }

    void addSamples(int count)
    {
        sampleCount += count;
    }

    int getSampleCount() const { return sampleCount; }
    bool hasReachedMaxDuration() const { return reachedMax; }
    bool getIsRecording() const { return isRecording; }

private:
    bool isRecording = false;
    bool reachedMax = false;
    int sampleCount = 0;
};