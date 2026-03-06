#pragma once

#include <JuceHeader.h>

class MainComponent final
    : public juce::Component,
      public juce::MenuBarModel,
      public juce::AudioIODeviceCallback
{
public:
    explicit MainComponent(juce::ApplicationProperties& props);

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

    //owner dashboard
    juce::TextButton recordButton { "Record" };
    juce::TextButton playButton { "Play" };
    juce::TextButton stopButton { "Stop" };
    juce::TextButton saveButton { "Save" };
    juce::TextButton createGuestButton { "Create Guest Account" };

    //audio
    juce::AudioFormatManager formatManager; // read/write audio files
    juce::AudioTransportSource transportSource; // manage playback
    std::unique_ptr<juce::AudioFormatReaderSource> currentAudioFile; // current audio file source
    juce::AudioDeviceManager deviceManager; 

    //Recording
    juce::AudioBuffer<float> recordingBuffer;
    bool isRecording { false }; 
    int recordingPosition = 0;

    // Recording status
    juce::Label recordingStatusLabel;
    
    // Sound list
    juce::ListBox soundList;

    //sliders for sound manipulation
    juce::Slider pitchSlider, lengthSlider, volumeSlider;
    juce::Label pitchLabel, lengthLabel, volumeLabel;

    //placeholder for 2-D cluster map
    juce::Component clusterMapPlaceholder;


    // Helpers
    void setupUI();
    void updateVisibility();
    bool userExists() const;
    void saveUserInfo(const juce::String&, const juce::String&, const juce::String&, const juce::String&);
    void loadUserInfo(juce::String&, juce::String&, juce::String&, juce::String&) const;
    void playRecordAudio();

    juce::String getCurrentUserRole() const
    {
        juce::String username, password, accountInfo, role;
        loadUserInfo(username, password, accountInfo, role);
        return role;
    }


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
