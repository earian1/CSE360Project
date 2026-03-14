#pragma once

class RecordingManager
{
public:
    void startRecording()
    {
        // Placeholder for starting recording logic
        isRecording = true;
    }

    void stopRecording()
    {
        // Placeholder for stopping recording logic
        isRecording = false;
    }

    bool getIsRecording() const
    {
        return isRecording;
    }

private:
    bool isRecording { false };
};