#include "MainComponent.h"
#include <string>
#include <iostream>
#include <cmath>
#include <climits>
using namespace std;

MainComponent::~MainComponent() = default;

class BufferAudioSource : public juce::PositionableAudioSource
{
public:
    BufferAudioSource(const juce::AudioBuffer<float>& buffer, int numSamples)
        : sourceBuffer(buffer), totalSamples(numSamples), position(0), playbackRate(1.0) {}

    void prepareToPlay(int, double) override { position = 0; }
    void releaseResources() override {}

    void setPlaybackRate(double rate) { playbackRate = rate; }
    void setVolume(float vol) {
        volume = vol;
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override
    {
        for (int i = 0; i < info.numSamples; ++i)
        {
            int srcPos = (int)(position + i * playbackRate);

            if (srcPos >= trimLength || srcPos >= totalSamples)
            {
                // silence the rest
                for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch)
                    info.buffer->setSample(ch, info.startSample + i, 0.0f);
            }
            else
            {
                for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch)
                {
                    const int srcCh = juce::jmin(ch, sourceBuffer.getNumChannels() - 1);
                    info.buffer->setSample(ch, info.startSample + i, sourceBuffer.getSample(srcCh, srcPos)* volume);
                }
            }
        }
        position += (int)(info.numSamples * playbackRate);
    }

    void setTrimLength(int samples) { trimLength = samples; }

    void setNextReadPosition(juce::int64 newPosition) override { position = (int)newPosition; }
    juce::int64 getNextReadPosition() const override { return position; }
    juce::int64 getTotalLength() const override { return totalSamples; }
    bool isLooping() const override { return false; }

private:
    const juce::AudioBuffer<float>& sourceBuffer;
    int totalSamples;
    int trimLength = INT_MAX;
    int position;
    double playbackRate { 1.0 };
    float volume { 1.0f };
};


MainComponent::MainComponent(juce::ApplicationProperties& props)
{
    userStorage = props.getUserSettings();
    setSize(600, 400);

    // Add menu bar
    addAndMakeVisible(menuBar);
    menuBar.setModel(this);

    // Setup UI components
    setupUI();

    // Check if a user exists
    currentState = userExists() ? AppState::LOGIN : AppState::FIRST_USER_SETUP;

    updateVisibility();
}
// ---------- UI SETUP ----------

void MainComponent::setupUI()
{   
    saveButton.onClick = [this]()
    {
        saveRecording();
    };

    playButton.onClick = [this]()
    {
        playRecordedAudio();
    };

    recordingStatusLabel.setText("Recording, 0 samples", juce::dontSendNotification);
    recordingStatusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    recordingStatusLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(recordingStatusLabel);


    // ---------- FIRST USER SETUP ----------
    usernameLabel_setup.setText("Username:", juce::dontSendNotification);
    usernameLabel_setup.setColour(juce::Label::textColourId, juce::Colours::white);
    usernameLabel_setup.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(usernameLabel_setup);

    passwordLabel_setup.setText("Password:", juce::dontSendNotification);
    passwordLabel_setup.setColour(juce::Label::textColourId, juce::Colours::white);
    passwordLabel_setup.setFont(juce::Font(16.0f, juce::Font::bold));
    passwordField_setup.setPasswordCharacter('*');
    addAndMakeVisible(passwordLabel_setup);
    addAndMakeVisible(passwordField_setup);

    accountInfoLabel_setup.setText("Account Info:", juce::dontSendNotification);
    accountInfoLabel_setup.setColour(juce::Label::textColourId, juce::Colours::white);
    accountInfoLabel_setup.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(accountInfoLabel_setup);
    addAndMakeVisible(accountInfoField_setup);

    roleLabel_setup.setText("Role:", juce::dontSendNotification);
    roleLabel_setup.setColour(juce::Label::textColourId, juce::Colours::white);
    roleLabel_setup.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(roleLabel_setup);


    usernameField_setup.setText("");
    passwordField_setup.setText("");
    accountInfoField_setup.setText("");

    addAndMakeVisible(usernameField_setup);
    addAndMakeVisible(passwordField_setup);
    addAndMakeVisible(accountInfoField_setup);

    submitButton.setButtonText("Submit");
    addAndMakeVisible(submitButton);
    submitButton.onClick = [this]()
    {
            string special_characters = "!@#$%^&*()_+-={}[]|\\\"\':;<>.?/";
            string cap_letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            string all_numbers = "0123456789";
            if (usernameField_setup.getText().isEmpty() ||
                passwordField_setup.getText().isEmpty() ||
                accountInfoField_setup.getText().isEmpty())
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Input Error",
                    "Please fill in all fields."
                );
                return;
            }
            else if ((passwordField_setup.getText()).indexOfAnyOf(special_characters) == -1 || (passwordField_setup.getText()).indexOfAnyOf(cap_letters) == -1 || (passwordField_setup.getText()).indexOfAnyOf(all_numbers) == -1){
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Input Error",
                "Please make sure password contains a special character, a capital letter, and a number"
            );
            return;
        }
        juce::String selectedRole = roleSelector_setup.getSelectedId() == 1 ? "Owner" : "Guest";
        saveUserInfo(usernameField_setup.getText(),
                     passwordField_setup.getText(),
                     accountInfoField_setup.getText(),
                     selectedRole);

        currentState = AppState::LOGIN;
        updateVisibility();
    };

    // ---------- LOGIN ----------
    usernameLabel_login.setText("Username:", juce::dontSendNotification);
    passwordLabel_login.setText("Password:", juce::dontSendNotification);
    addAndMakeVisible(usernameLabel_login);
    addAndMakeVisible(passwordLabel_login);

    usernameField_login.setText("");
    passwordField_login.setText("");
    passwordField_login.setPasswordCharacter('*');
    addAndMakeVisible(usernameField_login);
    addAndMakeVisible(passwordField_login);

    roleLabel_setup.setText("Role:", juce::dontSendNotification);
    roleLabel_setup.setColour(juce::Label::textColourId, juce::Colours::white);
    roleLabel_setup.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(roleLabel_setup);


    roleSelector_setup.addItem("Owner", 1);
    roleSelector_setup.addItem("Guest", 2);
    roleSelector_setup.setSelectedId(1);
    addAndMakeVisible(roleSelector_setup);

    loginButton.setButtonText("Login");
    addAndMakeVisible(loginButton);
    loginButton.onClick = [this]()
    {
        juce::String storedUsername, storedPassword, storedAccountInfo, storedRole;
        loadUserInfo(storedUsername, storedPassword, storedAccountInfo, storedRole);

        if (usernameField_login.getText() == storedUsername &&
            passwordField_login.getText() == storedPassword)
        {
            currentState = AppState::MAIN_APP;
            updateVisibility();
            resized();
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Login Successful",
                "Welcome back, " + storedUsername + "!\nAccount Info: " + storedAccountInfo + "\nRole: " + storedRole
            );
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Login Failed",
                "Invalid username or password."
            );
            passwordField_login.setText("");
        }
    };

    // ---------- BACK BUTTON ----------
    backButton.setButtonText("Back");
    addAndMakeVisible(backButton);
    backButton.onClick = [this]()
    {
        usernameField_login.setText("");
        passwordField_login.setText("");
        currentState = AppState::FIRST_USER_SETUP;

        usernameField_setup.setText("");
        passwordField_setup.setText("");
        accountInfoField_setup.setText("");
        roleSelector_setup.setSelectedId(1);

        updateVisibility();
        usernameField_setup.grabKeyboardFocus();
    };
    backButton.setVisible(false);

    // ---------- MAIN APP ----------
    mainAppPlaceholder.setJustificationType(juce::Justification::centred);
    mainAppPlaceholder.setColour(juce::Label::textColourId, juce::Colours::darkblue);
    mainAppPlaceholder.setFont(juce::Font(20.0f, juce::Font::bold));
    addAndMakeVisible(mainAppPlaceholder);
    mainAppPlaceholder.setVisible(false);

    // ---------- owner dashboard ----------
    recordButton.onClick = [this]()
    {
        if (!isRecording){
            isRecording = true;
            recordingPosition = 0;
            recordingBuffer.setSize(2, 44100 * 10); // 10 seconds buffer at 44.1kHz

            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Recording", "Recording started!");
        }
        else{
            isRecording = false;
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Recording", "Recording stopped!");

            juce::Logger::writeToLog("Recording stopped, samples recorded: " + juce::String(recordingPosition));

        }
    
    };


//deviceManager.initialiseWithDefaultDevices

    createGuestButton.onClick = []() { juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Create Guest Account", "Create Guest Account button clicked!"); };

    pitchLabel.setText("Pitch:", juce::dontSendNotification);
    lengthLabel.setText("Length:", juce::dontSendNotification);
    volumeLabel.setText("Volume:", juce::dontSendNotification);

    addAndMakeVisible(recordButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(createGuestButton);
    addAndMakeVisible(pitchLabel);
    addAndMakeVisible(lengthLabel);
    addAndMakeVisible(pitchSlider);
    addAndMakeVisible(lengthSlider);
    addAndMakeVisible(volumeLabel);
    addAndMakeVisible(volumeSlider);
    addAndMakeVisible(clusterMapPlaceholder);

    //hide owner dashboard by default, will be shown based on role after login
    recordButton.setVisible(false);
    playButton.setVisible(false);
    stopButton.setVisible(false);
    saveButton.setVisible(false);
    createGuestButton.setVisible(false);
    pitchLabel.setVisible(false);
    lengthLabel.setVisible(false);
    pitchSlider.setVisible(false);
    lengthSlider.setVisible(false);
    volumeLabel.setVisible(false);
    volumeSlider.setVisible(false);
    clusterMapPlaceholder.setVisible(false);

    formatManager.registerBasicFormats(); // Register WAV, AIFF, MP3, etc.
    deviceManager.initialiseWithDefaultDevices(2, 2); // 2 input channels, 2 output channels

    // Initialize audio device and register callback
    deviceManager.addAudioCallback(this);

    auto devices = deviceManager.getCurrentAudioDevice();
    if (devices != nullptr)
    {
        juce::Logger::writeToLog("Input device: " + devices->getName());
        juce::Logger::writeToLog("Input channels: " + juce::String(devices->getActiveInputChannels().countNumberOfSetBits()));
    }
    
    
juce::StringArray inputDevices;

juce::OwnedArray<juce::AudioIODeviceType> types;
deviceManager.createAudioDeviceTypes(types);

for (auto* type : types)
{
    type->scanForDevices();
    auto names = type->getDeviceNames(true);
    inputDevices.addArray(names);
}

// Log
juce::Logger::writeToLog("Available input devices: " + inputDevices.joinIntoString(", "));

if (inputDevices.isEmpty())
{
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                           "No Input Devices",
                                           "No microphone or input devices were found!");
}
else
{
    // pick first device safely
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);

    setup.inputDeviceName = inputDevices[0]; // safe now
    setup.inputChannels = juce::BigInteger();
    setup.inputChannels.setRange(0, 2, true); // enable first 2 input channels
    setup.outputDeviceName = deviceManager.getCurrentAudioDevice() ? deviceManager.getCurrentAudioDevice()->getName() : "";
    setup.outputChannels = deviceManager.getCurrentAudioDevice() ? deviceManager.getCurrentAudioDevice()->getActiveOutputChannels() : juce::BigInteger(0x3);

    deviceManager.setAudioDeviceSetup(setup, true);
}

auto* device = deviceManager.getCurrentAudioDevice();
if (!device || device->getActiveInputChannels().countNumberOfSetBits() == 0)
{
    recordButton.setEnabled(false);
    saveButton.setEnabled(false);
    juce::Logger::writeToLog("No microphone detected — recording disabled.");
}

pitchSlider.setRange(-12.0, 12.0, 0.1);
pitchSlider.setValue(0.0);
pitchSlider.onValueChange = [this]()
{
    if (bufferSource != nullptr)
    {
        double semitoneRatio = pitchSlider.getValue();
        double rate = pow(2.0, semitoneRatio / 12.0);
        bufferSource->setPlaybackRate(rate);
}
};

lengthSlider.setRange(0.0, 1.0, 0.01);
lengthSlider.setValue(1.0);
lengthSlider.onValueChange = [this]()
{
    if (bufferSource != nullptr)
    {
        int trimSamples = (int)(lengthSlider.getValue() * recordingPosition);
        bufferSource->setTrimLength(trimSamples);
    }
};

volumeSlider.setRange(0.0, 1.0, 0.01);
volumeSlider.setValue(1.0);
volumeSlider.onValueChange = [this]()
{
    if (bufferSource != nullptr)
        bufferSource->setVolume((float)volumeSlider.getValue());
};

   pauseButton.onClick = [this]()
{
    if (transportSource.isPlaying())
        transportSource.stop();
    else
        transportSource.start();
};

positionSlider.setRange(0.0, 1.0, 0.001);
positionSlider.setValue(0.0);
positionSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

positionSlider.onDragStart = [this]() { isScrubbing = true; };

positionSlider.onDragEnd = [this]()
{
    if (bufferSource != nullptr)
    {
        int newPos = (int)(positionSlider.getValue() * recordingPosition);
        bufferSource->setNextReadPosition(newPos);
    }
    isScrubbing = false;
};

positionLabel.setText("0:00 / 0:00", juce::dontSendNotification);
positionLabel.setColour(juce::Label::textColourId, juce::Colours::white);
positionLabel.setJustificationType(juce::Justification::centred);

addAndMakeVisible(pauseButton);
addAndMakeVisible(positionSlider);
addAndMakeVisible(positionLabel);

pauseButton.setVisible(false);
positionSlider.setVisible(false);
positionLabel.setVisible(false);

startTimer(50); // update every 50ms


}

// ---------- SAVE BUTTON ----------
void MainComponent::saveRecording()
{
    if (isRecording)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Save Error",
            "Please stop recording before saving."
        );
        return;
    }

    if (recordingPosition == 0)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Save Error",
            "No recording available to save!"
        );
        return;
    }

    // Keep FileChooser alive
    fileChooser = std::make_unique<juce::FileChooser>("Save Recording", juce::File{}, "*.wav");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (file == juce::File{}) return; // cancelled

            auto outputStream = file.createOutputStream();
            if (!outputStream)
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Save Error",
                    "Failed to create file stream."
                );
                return;
            }

            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::AudioFormatWriter> writer(
                wavFormat.createWriterFor(outputStream.release(),
                                          44100,
                                          recordingBuffer.getNumChannels(),
                                          16,
                                          {},
                                          0));

            if (!writer)
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Save Error",
                    "Failed to create WAV writer."
                );
                return;
            }

            writer->writeFromAudioSampleBuffer(recordingBuffer, 0, recordingPosition);

            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Save Successful",
                "Recording saved to: " + file.getFullPathName()
            );
        });
}


// ---------- PLAY BUTTON ----------
void MainComponent::playRecordedAudio()
{
    if (recordingPosition == 0)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon, "Playback Error", "No recording to play!");
        return;
    }

    // Stop transport FIRST and wait — this blocks until the audio thread is done
    transportSource.stop();
    transportSource.setSource(nullptr);  // audio thread will no longer call into bufferSource

    // NOW safe to reset
    bufferSource.reset();

    // Make a copy of the recorded data for playback
    playbackBuffer.makeCopyOf(recordingBuffer);
    // Trim to actual recorded length
    playbackBuffer.setSize(playbackBuffer.getNumChannels(), recordingPosition, true, false, false);

    bufferSource = std::make_unique<BufferAudioSource>(playbackBuffer, recordingPosition);


    double semitoneRatio = pitchSlider.getValue();
    double rate = pow(2.0, semitoneRatio / 12.0);
    bufferSource->setPlaybackRate(rate);

    int trimSamples = (int)(lengthSlider.getValue() * recordingPosition);
    bufferSource->setTrimLength(trimSamples);

    auto* device = deviceManager.getCurrentAudioDevice();
    double sampleRate = device ? device->getCurrentSampleRate() : 44100.0;

    bufferSource->setVolume((float)volumeSlider.getValue());
    positionSlider.setValue(0.0, juce::dontSendNotification);

    transportSource.setSource(bufferSource.get(), 0, nullptr, sampleRate);
    transportSource.start();
}


void MainComponent::updateVisibility()
{
    bool setupVisible = (currentState == AppState::FIRST_USER_SETUP);
    bool loginVisible = (currentState == AppState::LOGIN);
    bool mainVisible = (currentState == AppState::MAIN_APP);
    juce::String role = getCurrentUserRole();
    bool isOwner = mainVisible && role == "Owner";
    bool isGuest = mainVisible && role == "Guest";

    // FIRST USER SETUP
    usernameLabel_setup.setVisible(setupVisible);
    usernameField_setup.setVisible(setupVisible);
    passwordLabel_setup.setVisible(setupVisible);
    passwordField_setup.setVisible(setupVisible);
    accountInfoLabel_setup.setVisible(setupVisible);
    accountInfoField_setup.setVisible(setupVisible);
    roleLabel_setup.setVisible(setupVisible);
    roleSelector_setup.setVisible(setupVisible);
    submitButton.setVisible(setupVisible);

    // LOGIN
    usernameLabel_login.setVisible(loginVisible);
    usernameField_login.setVisible(loginVisible);
    passwordLabel_login.setVisible(loginVisible);
    passwordField_login.setVisible(loginVisible);
    loginButton.setVisible(loginVisible);
    backButton.setVisible(loginVisible);

    // MAIN APP
    mainAppPlaceholder.setVisible(mainVisible); // always show greeting

    // OWNER DASHBOARD
    recordButton.setVisible(isOwner);
    playButton.setVisible(isOwner);
    stopButton.setVisible(isOwner);
    saveButton.setVisible(isOwner);
    createGuestButton.setVisible(isOwner);
    pitchLabel.setVisible(isOwner);
    pitchSlider.setVisible(isOwner);
    lengthLabel.setVisible(isOwner);
    lengthSlider.setVisible(isOwner);
    volumeLabel.setVisible(isOwner);
    volumeSlider.setVisible(isOwner);
    clusterMapPlaceholder.setVisible(isOwner);

    pauseButton.setVisible(isOwner);
    positionSlider.setVisible(isOwner);
    positionLabel.setVisible(isOwner);
}


void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}
// ---------- LAYOUT ----------
void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(20);

    const int labelHeight = 25;
    const int fieldHeight = 50;
    const int buttonHeight = 40;
    const int buttonWidth = 120;
    const int sliderHeight = 40;
    const int spacing = 10;

    if (currentState == AppState::FIRST_USER_SETUP)
    {
        const int narrowWidth = 400;

        usernameLabel_setup.setBounds(area.removeFromTop(labelHeight));
        usernameField_setup.setBounds(area.removeFromTop(fieldHeight).reduced(0, spacing).withWidth(narrowWidth));
        passwordLabel_setup.setBounds(area.removeFromTop(labelHeight));
        passwordField_setup.setBounds(area.removeFromTop(fieldHeight).reduced(0, spacing).withWidth(narrowWidth));
        accountInfoLabel_setup.setBounds(area.removeFromTop(labelHeight));
        accountInfoField_setup.setBounds(area.removeFromTop(fieldHeight).reduced(0, spacing).withWidth(narrowWidth));
        roleLabel_setup.setBounds(area.removeFromTop(labelHeight));
        roleSelector_setup.setBounds(area.removeFromTop(fieldHeight).reduced(0, spacing).withWidth(narrowWidth));
        area.removeFromTop(20);
        submitButton.setBounds(area.removeFromTop(buttonHeight).withWidth(buttonWidth).withX(getWidth()/2 - narrowWidth/2 + 50));
    }

    else if (currentState == AppState::LOGIN)
{
    int fieldWidth = 350;
    int fieldHeight = 30;
    int labelHeight = 20;
    int buttonHeight = 40;
    int spacing = 10;

    int extraSpacing = 30;

    // Calculate total height of the form
    int formHeight = labelHeight + fieldHeight + spacing + 
                     labelHeight + fieldHeight + spacing +
                     buttonHeight + spacing +
                     buttonHeight;

    // Start Y coordinate to center the form vertically
    int startY = getHeight() / 2 - formHeight / 2 - 50;
    int centerX = getWidth() / 2 - fieldWidth / 2;

    // Username
    usernameLabel_login.setBounds(centerX, startY, fieldWidth, labelHeight);
    startY += labelHeight + spacing;
    usernameField_login.setBounds(centerX, startY, fieldWidth, fieldHeight);
    startY += fieldHeight + spacing;

    // Password
    passwordLabel_login.setBounds(centerX, startY, fieldWidth, labelHeight);
    startY += labelHeight + spacing;
    passwordField_login.setBounds(centerX, startY, fieldWidth, fieldHeight);
    startY += fieldHeight + spacing;

    startY += extraSpacing;

    // Login button
    loginButton.setBounds(centerX, startY, fieldWidth, buttonHeight);
    startY += buttonHeight + spacing;

    // Back button
    backButton.setBounds(centerX, startY, fieldWidth, buttonHeight);
}

    else if (currentState == AppState::MAIN_APP)
    {
        // Guest view
        if (getCurrentUserRole() == "Guest")
        {
            mainAppPlaceholder.setBounds(area);
        }
        // Owner view
        else if (getCurrentUserRole() == "Owner")
        {
            recordButton.setBounds(area.removeFromTop(buttonHeight));
            playButton.setBounds(area.removeFromTop(buttonHeight));
            pauseButton.setBounds(area.removeFromTop(buttonHeight));
            saveButton.setBounds(area.removeFromTop(buttonHeight));
            createGuestButton.setBounds(area.removeFromTop(buttonHeight));

            pitchLabel.setBounds(area.removeFromTop(sliderHeight / 2));
            pitchSlider.setBounds(area.removeFromTop(sliderHeight));

            lengthLabel.setBounds(area.removeFromTop(sliderHeight / 2));
            lengthSlider.setBounds(area.removeFromTop(sliderHeight));

            volumeLabel.setBounds(area.removeFromTop(sliderHeight / 2));
            volumeSlider.setBounds(area.removeFromTop(sliderHeight));

            soundList.setBounds(area.removeFromTop(150));
            clusterMapPlaceholder.setBounds(area);

            recordingStatusLabel.setBounds(area.removeFromTop(25));

            positionSlider.setBounds(area.removeFromTop(20));
            positionLabel.setBounds(area.removeFromTop(20));
        }
    }
}


// ---------- USER STORAGE ----------
bool MainComponent::userExists() const
{
    return userStorage != nullptr &&
           !userStorage->getValue("username", "").isEmpty() &&
           !userStorage->getValue("password", "").isEmpty();
}

void MainComponent::saveUserInfo(const juce::String& username, const juce::String& password, const juce::String& accountInfo, const juce::String& role)
{
    userStorage->setValue("username", username);
    userStorage->setValue("password", password);
    userStorage->setValue("accountInfo", accountInfo);
    userStorage->setValue("role", role);
    userStorage->saveIfNeeded();
}

void MainComponent::loadUserInfo(juce::String& username, juce::String& password, juce::String& accountInfo, juce::String& role) const
{
    username = userStorage->getValue("username", "");
    password = userStorage->getValue("password", "");
    accountInfo = userStorage->getValue("accountInfo", "");
    role = userStorage->getValue("role", "");
}


void MainComponent::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device!= nullptr)
    transportSource.prepareToPlay(device->getCurrentBufferSizeSamples(), device->getCurrentSampleRate());
}

void MainComponent::audioDeviceStopped()
{
    transportSource.stop();
}

void MainComponent::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext&)
{
    // --- RECORDING ---
    if (isRecording && inputChannelData != nullptr)
{
    const int numSamplesToCopy = juce::jmin(numSamples, recordingBuffer.getNumSamples() - recordingPosition);
    for (int ch = 0; ch < juce::jmin(numInputChannels, recordingBuffer.getNumChannels()); ++ch)
    {
        recordingBuffer.copyFrom(ch, recordingPosition, inputChannelData[ch], numSamplesToCopy);
    }
    recordingPosition += numSamplesToCopy;

    // Update the label safely in the message thread
    juce::MessageManager::callAsync([this]()
    {
        recordingStatusLabel.setText("Recording: " + juce::String(recordingPosition) + " samples", juce::dontSendNotification);
    });

    // Optional: auto-stop if buffer full
    if (recordingPosition >= recordingBuffer.getNumSamples())
    {
        isRecording = false;
        juce::MessageManager::callAsync([this]()
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                   "Recording",
                                                   "Maximum recording time reached (10 seconds). Recording stopped!");
        });
    }
}


    // --- PLAYBACK ---
    // Clear outputs first
    juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);
    juce::AudioSourceChannelInfo info(&buffer, 0, numSamples);

    transportSource.getNextAudioBlock(info);

    // If using transportSource for playback, it will automatically fill output buffers
}

void MainComponent::timerCallback()
{
    if (bufferSource == nullptr || isScrubbing)
        return;

    double sampleRate = 44100.0;
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device) sampleRate = device->getCurrentSampleRate();

    int currentPos = (int)bufferSource->getNextReadPosition();
    double currentSecs = currentPos / sampleRate;
    double totalSecs = recordingPosition / sampleRate;

    if (totalSecs > 0)
        positionSlider.setValue(currentPos / (double)recordingPosition, juce::dontSendNotification);

    auto toTimeString = [](double secs) -> juce::String
    {
        int m = (int)(secs / 60);
        int s = (int)(secs) % 60;
        return juce::String(m) + ":" + (s < 10 ? "0" : "") + juce::String(s);
    };

    positionLabel.setText(toTimeString(currentSecs) + " / " + toTimeString(totalSecs),
                          juce::dontSendNotification);
}




//==============================================================================

// ---------- MENU BAR ----------
juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File", "Edit" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int menuIndex, const juce::String& menuName)
{
    juce::PopupMenu menu;
    if (menuIndex == 0) menu.addItem(1, "Exit");
    if (menuIndex == 1) menu.addItem(2, "Settings");
    return menu;
}
//fileChooser.browseForFileToSave
void MainComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    if (menuItemID == 1)
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
}





