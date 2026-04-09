#pragma once

#include <JuceHeader.h>

class BufferAudioSource;

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

    void createGuestAccount();
    void saveGuestInfo(const juce::String& username, const juce::String& password);
    void showLoginScreen();

private:
    // ---------------- STATE ----------------
    enum class AppState
    {
        FIRST_USER_SETUP,
        LOGIN,
        MAIN_APP
    };

    struct SavedSound
{
    juce::String name;
    juce::AudioBuffer<float> buffer;
    int numSamples;
    double sampleRate;
};

struct ClusterDot { int x, y, soundIndex; };
juce::Array<ClusterDot> clusterDots;
void mouseDown(const juce::MouseEvent& e) override;

const juce::AudioBuffer<float>* displayBuffer = nullptr;
int displayNumSamples = 0;


juce::OwnedArray<SavedSound> inMemorySounds;

    AppState currentState{ AppState::LOGIN };
    bool loggedInAsGuest{ false };
    juce::PropertiesFile* userStorage{ nullptr };
    juce::Random random;

    bool userExists();
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

    void loadGuestInfo(juce::String& username,
                       juce::String& password) const;

    void playRecordedAudio();
    void loadAudioFileForPlayback(const juce::File& file);
    void refreshSoundListFromStorage();
    void saveSoundListToStorage();
    void addSavedSound(const juce::String& name, const juce::String& path);
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;

    bool isOwnerLoggedIn() const;
    bool isGuestLoggedIn() const;

    void drawWaveform(juce::Graphics& g);
    void drawClusterMap(juce::Graphics& g);
    void applySlidersToBufferSource();

    void ensureDefaultAccounts();
    void setupUI();
    void updateVisibility();

    // ---------------- FIRST USER SETUP / CREATE ACCOUNT ----------------
    juce::Label usernameLabel_setup;
    juce::Label passwordLabel_setup;
    juce::Label accountInfoLabel_setup;
    juce::Label roleLabel_setup;

    juce::TextEditor usernameField_setup;
    juce::TextEditor passwordField_setup;
    juce::TextEditor accountInfoField_setup;

    juce::ComboBox roleSelector_setup;
    juce::TextButton submitButton { "Submit" };

    // ---------------- LOGIN UI ----------------
    juce::Label usernameLabel_login;
    juce::Label passwordLabel_login;
    juce::Label loginRoleLabel;

    juce::TextEditor usernameField_login;
    juce::TextEditor passwordField_login;
    juce::ComboBox loginRoleSelector;
    juce::TextButton loginButton { "Login" };
    juce::TextButton forgotButton;


    // ---------------- MAIN APP UI ----------------
    juce::Label mainAppPlaceholder;
    juce::Label soundsListLabel;
    juce::Label waveformLabel;
    juce::Label clusterMapLabel;
    juce::Label recordingStatusLabel;

    juce::TextButton recordButton { "Record" };
    juce::TextButton playButton { "Play" };
    juce::TextButton stopButton { "Stop" };
    juce::TextButton saveButton { "Save" };
    juce::TextButton createGuestButton { "Create Guest Account" };
    juce::TextButton logoutButton { "Logout" };
    juce::TextButton downloadButton { "Download Sound" };

    // ---------------- SLIDERS ----------------
    juce::Slider pitchSlider;
    juce::Slider lengthSlider;
    juce::Slider volumeSlider;

    juce::Label pitchLabel;
    juce::Label lengthLabel;
    juce::Label volumeLabel;

    // ---------------- SOUND LIST ----------------
    juce::ListBox soundList;
    juce::StringArray savedSoundNames;
    juce::StringArray savedSoundPaths;
    int selectedSoundRow{ -1 };
std::unique_ptr<juce::FileChooser> fileChooser;
    // ---------------- AUDIO ----------------
    juce::AudioFormatManager formatManager;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> currentAudioFile;
    juce::AudioDeviceManager deviceManager;
    std::unique_ptr<BufferAudioSource> bufferSource;
    juce::AudioBuffer<float> playbackBuffer;

    juce::AudioBuffer<float> recordingBuffer;
    bool isRecording{ false };
    int recordingPosition{ 0 };
    double currentSampleRate{ 44100.0 };

    // ---------------- DRAW AREAS ----------------
    juce::Rectangle<int> waveformArea;
    juce::Rectangle<int> clusterArea;

    // ---------------- MENU ----------------
    juce::MenuBarComponent menuBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};