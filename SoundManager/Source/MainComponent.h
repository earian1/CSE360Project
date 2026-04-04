#pragma once

#include <JuceHeader.h>

class MainComponent final
    : public juce::Component,
    public juce::MenuBarModel,
    public juce::AudioIODeviceCallback,
    public juce::ListBoxModel
{
public:
    explicit MainComponent(juce::ApplicationProperties& props);
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int, const juce::String&) override;
    void menuItemSelected(int, int) override;

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber,
        juce::Graphics& g,
        int width,
        int height,
        bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;

private:
    enum class AppState
    {
        LOGIN,
        MAIN_APP
    };

    juce::PropertiesFile* userStorage{ nullptr };
    AppState currentState{ AppState::LOGIN };
    bool loggedInAsGuest{ false };

    // Login UI
    juce::Label usernameLabel_login, passwordLabel_login, loginRoleLabel;
    juce::TextEditor usernameField_login, passwordField_login;
    juce::ComboBox loginRoleSelector;
    juce::TextButton loginButton{ "Login" };

    // Main app labels
    juce::Label mainAppPlaceholder;
    juce::Label soundsListLabel;
    juce::Label waveformLabel;
    juce::Label clusterMapLabel;
    juce::Label recordingStatusLabel;

    // Menu bar
    juce::MenuBarComponent menuBar;

    // Buttons
    juce::TextButton recordButton{ "Record" };
    juce::TextButton playButton{ "Play" };
    juce::TextButton stopButton{ "Stop" };
    juce::TextButton saveButton{ "Save" };
    juce::TextButton createGuestButton{ "Create Guest Account" };
    juce::TextButton logoutButton{ "Logout" };
    juce::TextButton downloadButton{ "Download Sound" };

    // Sound list
    juce::ListBox soundList;
    juce::StringArray savedSoundNames;
    juce::StringArray savedSoundPaths;
    int selectedSoundRow = -1;

    // Audio
    juce::AudioFormatManager formatManager;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> currentAudioFile;
    juce::AudioDeviceManager deviceManager;

    // Recording
    juce::AudioBuffer<float> recordingBuffer;
    bool isRecording{ false };
    int recordingPosition = 0;
    double currentSampleRate = 44100.0;

    // Sliders
    juce::Slider pitchSlider, lengthSlider, volumeSlider;
    juce::Label pitchLabel, lengthLabel, volumeLabel;

    // Drawing areas
    juce::Rectangle<int> waveformArea;
    juce::Rectangle<int> clusterArea;

    void setupUI();
    void updateVisibility();

    void ensureDefaultAccounts();
    bool roleChoiceUnlocked() const;
    void setRoleChoiceUnlocked(bool unlocked);

    void saveUserInfo(const juce::String& username,
        const juce::String& password,
        const juce::String& accountInfo,
        const juce::String& role);

    void loadUserInfo(juce::String& username,
        juce::String& password,
        juce::String& accountInfo,
        juce::String& role) const;

    void saveGuestInfo(juce::String& username,
        juce::String& password);

    void loadGuestInfo(juce::String& username,
        juce::String& password) const;

    void playRecordedAudio();
    void loadAudioFileForPlayback(const juce::File& file);
    void refreshSoundListFromStorage();
    void saveSoundListToStorage();
    void addSavedSound(const juce::String& name, const juce::String& path);

    bool isOwnerLoggedIn() const;
    bool isGuestLoggedIn() const;

    void drawWaveform(juce::Graphics& g);
    void drawClusterMap(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};