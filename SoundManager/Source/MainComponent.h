#pragma once

#include <JuceHeader.h>

// Forward declaration
class BufferAudioSource;

class MainComponent final
    : public juce::Component,
      public juce::MenuBarModel,
      public juce::AudioIODeviceCallback,
      public juce::Timer
{
public:
    explicit MainComponent(juce::ApplicationProperties& props);
    ~MainComponent() override;
    
        void saveRecording();
        void playRecordedAudio();
        

    void paint(juce::Graphics&) override;
    void resized() override;
    

    // Menu bar
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int, const juce::String&) override;
    void menuItemSelected(int, int) override;

    // Audio callback
void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                       int numInputChannels,
                                       float* const* outputChannelData,
                                       int numOutputChannels,
                                       int numSamples,
                                       const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    enum class AppState
    {
        FIRST_USER_SETUP,
        LOGIN,
        MAIN_APP
    };

    // Playback
    std::unique_ptr<BufferAudioSource> bufferSource;
    juce::AudioBuffer<float> playbackBuffer;

    juce::PropertiesFile* userStorage { nullptr };
    AppState currentState { AppState::FIRST_USER_SETUP };

    // FIRST USER SETUP
    juce::Label usernameLabel_setup, passwordLabel_setup, accountInfoLabel_setup, roleLabel_setup;
    juce::TextEditor usernameField_setup, passwordField_setup, accountInfoField_setup;
    juce::ComboBox roleSelector_setup;
    juce::TextButton submitButton { "Submit" };
    juce::TextButton backButton { "Back" };

    // LOGIN
    juce::Label usernameLabel_login, passwordLabel_login;
    juce::TextEditor usernameField_login, passwordField_login;
    juce::Label roleDisplay_login;
    juce::TextButton loginButton { "Login" };

    // MAIN APP
    juce::Label mainAppPlaceholder;

    // Menu bar
    juce::MenuBarComponent menuBar;

    // Owner dashboard
    juce::TextButton recordButton { "Record" };
    juce::TextButton playButton { "Play" };
    juce::TextButton stopButton { "Stop" };
    juce::TextButton saveButton { "Save" };
    juce::TextButton createGuestButton { "Create Guest Account" };

    // Audio
    juce::AudioFormatManager formatManager;
    juce::AudioTransportSource transportSource;
    juce::AudioDeviceManager deviceManager;

    // Recording
    juce::AudioBuffer<float> recordingBuffer;
    bool isRecording { false };
    int recordingPosition = 0;

    // Recording status
    juce::Label recordingStatusLabel;

    // Sound list
    juce::ListBox soundList;

    // Sliders
    juce::Slider pitchSlider, lengthSlider, volumeSlider;
    juce::Label pitchLabel, lengthLabel, volumeLabel;

    // Cluster map placeholder
    juce::Component clusterMapPlaceholder;

    // File chooser
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Timer for updating playback position
    juce::TextButton pauseButton { "Pause" };
    juce::TextButton resumeButton { "Resume" };
    juce::Slider positionSlider;
    juce::Label positionLabel;
    bool isScrubbing { false };


    // Helpers
    void setupUI();
    void updateVisibility();
    bool userExists() const;
    void saveUserInfo(const juce::String&, const juce::String&, const juce::String&, const juce::String&);
    void loadUserInfo(juce::String&, juce::String&, juce::String&, juce::String&) const;
    void timerCallback() override;

    juce::String getCurrentUserRole() const
    {
        juce::String username, password, accountInfo, role;
        loadUserInfo(username, password, accountInfo, role);
        return role;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};