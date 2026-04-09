#include "MainComponent.h"
#include <string>
#include <iostream>
#include <cmath>
#include <climits>

using namespace std;

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



void MainComponent::applySlidersToBufferSource()
{
    if (!bufferSource) return;

    // Pitch
    float pitchValue = pitchSlider.getValue(); // 0-100
    float rate = (pitchValue < 50.0f)
                 ? juce::jmap(pitchValue, 0.0f, 50.0f, 0.5f, 1.0f)
                 : juce::jmap(pitchValue, 50.0f, 100.0f, 1.0f, 2.0f);
    bufferSource->setPlaybackRate(rate);

    // Volume (logarithmic)
    float volValue = volumeSlider.getValue(); // 0-100
    // map slider 0-100 to decibels from -60dB (almost silent) to 0dB (full)
    float dB = juce::jmap(volValue, 0.0f, 100.0f, -60.0f, 0.0f);
    float gain = juce::Decibels::decibelsToGain(dB); 
    bufferSource->setVolume(gain);

    // Length (trim)
    float lenValue = lengthSlider.getValue(); // 0-100
    int trimSamples = static_cast<int>(recordingBuffer.getNumSamples() * (lenValue / 100.0f));
    bufferSource->setTrimLength(trimSamples);
}

bool MainComponent::userExists()
{
    // Example: check if your user data file exists
    juce::File userFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                            .getChildFile("user_data.txt");
    return userFile.exists();
}

MainComponent::MainComponent(juce::ApplicationProperties& props)
{
    userStorage = props.getUserSettings();
    setSize(1000, 700);

    setupUI();

    currentState = AppState::LOGIN;

    refreshSoundListFromStorage();
    updateVisibility();
}

MainComponent::~MainComponent()
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    currentAudioFile.reset();
    deviceManager.removeAudioCallback(this);
}

void MainComponent::ensureDefaultAccounts()
{
    if (userStorage == nullptr)
        return;

    if (!userStorage->containsKey("roleChoiceUnlocked"))
        userStorage->setValue("roleChoiceUnlocked", false);

    userStorage->saveIfNeeded();
}

bool MainComponent::roleChoiceUnlocked() const
{
    if (userStorage == nullptr)
        return false;

    return userStorage->getBoolValue("roleChoiceUnlocked", false);
}

void MainComponent::setRoleChoiceUnlocked(bool unlocked)
{
    if (userStorage == nullptr)
        return;

    userStorage->setValue("roleChoiceUnlocked", unlocked);
    userStorage->saveIfNeeded();
}

void MainComponent::setupUI()
{

    
    // Login UI
    usernameLabel_setup.setText("Username:", juce::dontSendNotification);
    addAndMakeVisible(usernameLabel_setup);
    addAndMakeVisible(usernameField_setup);

    passwordLabel_setup.setText("Password:", juce::dontSendNotification);
    passwordField_setup.setPasswordCharacter('*');
    addAndMakeVisible(passwordLabel_setup);
    addAndMakeVisible(passwordField_setup);

    accountInfoLabel_setup.setText("Account Info:", juce::dontSendNotification);
    addAndMakeVisible(accountInfoLabel_setup);
    addAndMakeVisible(accountInfoField_setup);

    roleLabel_setup.setText("Role:", juce::dontSendNotification);
    addAndMakeVisible(roleLabel_setup);

    roleSelector_setup.addItem("Owner", 1);
    roleSelector_setup.setSelectedId(1);
    addAndMakeVisible(roleSelector_setup);

    submitButton.setButtonText("Submit"); // must be BEFORE addAndMakeVisible
    addAndMakeVisible(submitButton);


    usernameLabel_login.setText("Username:", juce::dontSendNotification);
    passwordLabel_login.setText("Password:", juce::dontSendNotification);
    loginRoleLabel.setText("Log In As:", juce::dontSendNotification);

    usernameLabel_login.setColour(juce::Label::textColourId, juce::Colours::white);
    passwordLabel_login.setColour(juce::Label::textColourId, juce::Colours::white);
    loginRoleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    passwordField_login.setPasswordCharacter('*');

    loginRoleSelector.addItem("Owner", 1);
    loginRoleSelector.addItem("Guest", 2);
    loginRoleSelector.setSelectedId(1);
    loginRoleSelector.setEnabled(roleChoiceUnlocked());

    addAndMakeVisible(usernameLabel_login);
    addAndMakeVisible(passwordLabel_login);
    addAndMakeVisible(loginRoleLabel);
    addAndMakeVisible(usernameField_login);
    addAndMakeVisible(passwordField_login);
    addAndMakeVisible(loginRoleSelector);
    addAndMakeVisible(loginButton);

    submitButton.onClick = [this]()
{
    if (usernameField_setup.getText().isEmpty() ||
        passwordField_setup.getText().isEmpty() ||
        accountInfoField_setup.getText().isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Error",
            "Fill all fields");
        return;
    }

    // ← ADD THIS
    if (!isValidPassword(passwordField_setup.getText()))
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Weak Password",
            "Password must contain:\n- At least one uppercase letter\n- At least one number\n- At least one special character");
        return;
    }

    juce::String role = roleSelector_setup.getSelectedId() == 1 ? "Owner" : "Guest";
    saveUserInfo(
        usernameField_setup.getText(),
        passwordField_setup.getText(),
        accountInfoField_setup.getText(),
        role);

    currentState = AppState::LOGIN;
    updateVisibility();
};

    loginButton.onClick = [this]()
{
    
    juce::String enteredUsername = usernameField_login.getText().trim();
    juce::String enteredPassword = passwordField_login.getText().trim();
    int selectedRole = loginRoleSelector.getSelectedId();

    if (enteredUsername.isEmpty() || enteredPassword.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Login Error",
            "Please enter both username and password.");
        return;
    }



    if (selectedRole == 1)
    {
        addAndMakeVisible(menuBar);
        menuBar.setModel(this);

        juce::String storedUsername, storedPassword, storedAccountInfo, storedRole;
        loadUserInfo(storedUsername, storedPassword, storedAccountInfo, storedRole);

        if (storedUsername.isEmpty())
        {

            if (!isValidPassword(enteredPassword))
        {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Weak Password",
            "Password must contain:\n- At least one uppercase letter\n- At least one number\n- At least one special character");
        return;
        }
            saveUserInfo(enteredUsername, enteredPassword, "User Account", "Owner");
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Account Created",
                "Your account has been created. Please log in again.");
            usernameField_login.clear();
            passwordField_login.clear();
            return;
        }
        else if (enteredUsername == storedUsername &&
                 enteredPassword == storedPassword &&
                 storedRole == "Owner")
        {
            loggedInAsGuest = false;
            currentState = AppState::MAIN_APP;

            if (!roleChoiceUnlocked())
            {
                setRoleChoiceUnlocked(true);
                loginRoleSelector.setEnabled(true);
            }

            updateVisibility();
            resized();
            repaint();

            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Login Successful",
                "Welcome, Owner!");
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Login Failed",
                "Invalid owner username or password.");
            passwordField_login.clear();
        }
    }
    else if (selectedRole == 2)  // ← ADD THIS ENTIRE BLOCK
    {
        juce::String guestUsername, guestPassword;
        loadGuestInfo(guestUsername, guestPassword);

        if (enteredUsername == guestUsername &&
            enteredPassword == guestPassword)
        {
            loggedInAsGuest = true;
            currentState = AppState::MAIN_APP;
            updateVisibility();
            resized();
            repaint();

            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Login Successful",
                "Welcome, Guest!");
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Login Failed",
                "Invalid guest username or password.");
            passwordField_login.clear();
        }
    }

    
};

forgotButton.setButtonText("Forgot credentials?");
addAndMakeVisible(forgotButton);

forgotButton.onClick = [this]()
{
    juce::AlertWindow::showOkCancelBox(
        juce::AlertWindow::QuestionIcon,
        "Forgot Credentials",
        "What would you like to do?",
        "Reset Account",   // OK = reset
        "Change Credentials", // Cancel = change
        nullptr,
        juce::ModalCallbackFunction::create([this](int result)
        {
            if (result == 1) // Reset Account
            {
                currentState = AppState::FIRST_USER_SETUP;
                updateVisibility();
                resized();
            }
            else // Change Credentials
            {
                auto* dialog = new juce::AlertWindow("Change Credentials", "", juce::AlertWindow::NoIcon);
                dialog->addTextEditor("username", "", "New Username:");
                dialog->addTextEditor("password", "", "New Password:");
                dialog->addButton("Save", 1);
                dialog->addButton("Cancel", 0);

                dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog](int r)
                {
                    if (r == 1)
                    {
                        juce::String newUser = dialog->getTextEditorContents("username").trim();
                        juce::String newPass = dialog->getTextEditorContents("password").trim();

                        if (newUser.isEmpty() || newPass.isEmpty())
                        {
                            juce::AlertWindow::showMessageBoxAsync(
                                juce::AlertWindow::WarningIcon,
                                "Error", "Fields cannot be empty.");
                            return;
                        }

                        if (!isValidPassword(newPass))
                        {
                            juce::AlertWindow::showMessageBoxAsync(
                                juce::AlertWindow::WarningIcon,
                                "Weak Password",
                                "Password must contain:\n- At least one uppercase letter\n- At least one number\n- At least one special character");
                            delete dialog;
                            return;
                        }

                        juce::String dummy, oldPass, info, role;
                        loadUserInfo(dummy, oldPass, info, role);
                        saveUserInfo(newUser, newPass, info, role);

                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::InfoIcon,
                            "Success", "Credentials updated! Please log in.");
                    }
                    delete dialog;
                }), true);
            }
        }));
};

    // Main app labels
    mainAppPlaceholder.setJustificationType(juce::Justification::centredLeft);
    mainAppPlaceholder.setColour(juce::Label::textColourId, juce::Colours::blue);
    mainAppPlaceholder.setText("Sound Effects Management System", juce::dontSendNotification);

    soundsListLabel.setText("Saved Sounds", juce::dontSendNotification);
    waveformLabel.setText("Waveform View", juce::dontSendNotification);
    clusterMapLabel.setText("2-D Cluster Map", juce::dontSendNotification);
    recordingStatusLabel.setText("Not recording", juce::dontSendNotification);

    soundsListLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    waveformLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    clusterMapLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    recordingStatusLabel.setColour(juce::Label::textColourId, juce::Colours::red);

    addAndMakeVisible(mainAppPlaceholder);
    addAndMakeVisible(soundsListLabel);
    addAndMakeVisible(waveformLabel);
    addAndMakeVisible(clusterMapLabel);
    addAndMakeVisible(recordingStatusLabel);

    // Sound list
    soundList.setModel(this);
    soundList.setRowHeight(15);
    addAndMakeVisible(soundList);

    // Sliders
    pitchLabel.setText("Pitch", juce::dontSendNotification);
    lengthLabel.setText("Length", juce::dontSendNotification);
    volumeLabel.setText("Volume", juce::dontSendNotification);

    pitchLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    lengthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    volumeLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    pitchSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    lengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);

    pitchSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    lengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);

    pitchSlider.setRange(0, 100, 1);
    lengthSlider.setRange(0, 100, 1);
    volumeSlider.setRange(0, 100, 1);

    pitchSlider.setValue(50);   // neutral
    lengthSlider.setValue(100); // full length
    volumeSlider.setValue(80);  // default volume

    volumeSlider.onValueChange = [this]()
    {
     if (bufferSource)
    {
        float volValue = volumeSlider.getValue();
        float dB = juce::jmap(volValue, 0.0f, 100.0f, -60.0f, 0.0f);
        float gain = juce::Decibels::decibelsToGain(dB);
        bufferSource->setVolume(gain);
        repaint();
    }
    };

    pitchSlider.onValueChange = [this]()
    {
    if (bufferSource)
    {
        float value = pitchSlider.getValue();
        float rate = (value < 50.0f)
                     ? juce::jmap(value, 0.0f, 50.0f, 0.5f, 1.0f)
                     : juce::jmap(value, 50.0f, 100.0f, 1.0f, 2.0f);
        bufferSource->setPlaybackRate(rate);
    }
    repaint();
    };

    lengthSlider.onValueChange = [this]()
    {
    if (bufferSource && recordingPosition > 0)
    {
        int trimSamples = (int)(
            (lengthSlider.getValue() / 100.0) * recordingPosition
        );

        bufferSource->setTrimLength(trimSamples);
    }
    repaint();
    };

    addAndMakeVisible(pitchLabel);
    addAndMakeVisible(lengthLabel);
    addAndMakeVisible(volumeLabel);
    addAndMakeVisible(pitchSlider);
    addAndMakeVisible(lengthSlider);
    addAndMakeVisible(volumeSlider);

    // Buttons
    addAndMakeVisible(recordButton);
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(createGuestButton);
    addAndMakeVisible(logoutButton);
    addAndMakeVisible(downloadButton);
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);

    recordButton.onClick = [this]()
{
    if (!isRecording)
    {
        isRecording = true;
        recordingPosition = 0;

        int totalSamples = (int)(currentSampleRate * 10.0);
        recordingBuffer.setSize(2, totalSamples);
        recordingBuffer.clear();

        blinkState = true;
        startTimer(500);

        recordingStatusLabel.setText("Recording started...", juce::dontSendNotification);
    }
    else
    {
        isRecording = false;
        blinkState = false;
        stopTimer();
        repaint();

        recordingStatusLabel.setText("Recording stopped", juce::dontSendNotification);

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Recording",
            "Recording stopped.");
    }
};


    playButton.onClick = [this]()
{
    transportSource.stop();
    transportSource.setSource(nullptr);

    if (selectedSoundRow >= 0 && selectedSoundRow < inMemorySounds.size())
    {
        auto* sound = inMemorySounds[selectedSoundRow];

        bufferSource.reset();
        playbackBuffer.makeCopyOf(sound->buffer);
        playbackBuffer.setSize(playbackBuffer.getNumChannels(), sound->numSamples, true, false, false);

        displayBuffer = &playbackBuffer;      
        displayNumSamples = sound->numSamples;

        bufferSource = std::make_unique<BufferAudioSource>(playbackBuffer, sound->numSamples);

        float sliderVal = pitchSlider.getValue();
        float rate = 0.5f * std::pow(4.0f, sliderVal / 100.0f);
        bufferSource->setPlaybackRate(rate);

        int trimSamples = static_cast<int>((lengthSlider.getValue() / 100.0) * sound->numSamples);
        bufferSource->setTrimLength(trimSamples);

        float dB = juce::jmap((float)volumeSlider.getValue(), 0.0f, 100.0f, -60.0f, 0.0f);
        bufferSource->setVolume(juce::Decibels::decibelsToGain(dB));

        transportSource.setSource(bufferSource.get(), 0, nullptr, sound->sampleRate);
        transportSource.start();
        repaint();
        return;
    }

    // Fall back to playing the current recording if nothing selected
    if (recordingPosition > 0){
        playRecordedAudio();
        repaint();
    }
    else{
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Playback Error",
            "No recording or sound selected!");
        }
};


    stopButton.onClick = [this]()
        {
            transportSource.stop();
        };

    saveButton.onClick = [this]()
{
    if (isRecording)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Save Error",
            "Please stop recording before saving.");
        return;
    }

    if (recordingPosition == 0)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Save Error",
            "No recording available to save!");
        return;
    }

    auto* nameDialog = new juce::AlertWindow("Save Sound", "Enter a name for this sound:", juce::AlertWindow::NoIcon);
    nameDialog->addTextEditor("soundName", "", "Sound Name:");
    nameDialog->addButton("Save", 1);
    nameDialog->addButton("Cancel", 0);

    nameDialog->enterModalState(true, juce::ModalCallbackFunction::create([this, nameDialog](int result)
    {
        if (result == 1)
        {
            juce::String soundName = nameDialog->getTextEditorContents("soundName").trim();
            if (soundName.isEmpty())
                soundName = "Untitled";

            // Save to a hidden app folder
            juce::File saveFolder = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                        .getChildFile("SoundEffectsApp")
                                        .getChildFile("Sounds");
            saveFolder.createDirectory();

            juce::File outputFile = saveFolder.getChildFile(soundName + ".wav");

            // Avoid overwriting
            int counter = 1;
            while (outputFile.existsAsFile())
                outputFile = saveFolder.getChildFile(soundName + "_" + juce::String(counter++) + ".wav");

            // Write WAV to disk
            juce::WavAudioFormat wavFormat;
            std::unique_ptr<juce::FileOutputStream> outputStream(outputFile.createOutputStream());

            if (!outputStream)
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon, "Save Error", "Could not create file.");
                delete nameDialog;
                return;
            }

            std::unique_ptr<juce::AudioFormatWriter> writer(
                wavFormat.createWriterFor(outputStream.release(),
                                          currentSampleRate,
                                          recordingBuffer.getNumChannels(),
                                          16, {}, 0));

            if (!writer)
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon, "Save Error", "Could not create WAV writer.");
                delete nameDialog;
                return;
            }

            writer->writeFromAudioSampleBuffer(recordingBuffer, 0, recordingPosition);

            // Also store in memory for this session
            auto* sound = new SavedSound();
            sound->name = soundName;
            sound->buffer.makeCopyOf(recordingBuffer);
            sound->numSamples = recordingPosition;
            sound->sampleRate = currentSampleRate;
            inMemorySounds.add(sound);

            // Add to list and persist
            addSavedSound(outputFile.getFileNameWithoutExtension(), outputFile.getFullPathName());
            soundList.updateContent();
            repaint();

            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Saved!",
                "Sound \"" + soundName + "\" saved!");
        }

        delete nameDialog;
    }), true);
};



    downloadButton.onClick = [this]()
{
    if (selectedSoundRow < 0 || selectedSoundRow >= inMemorySounds.size())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Download Error",
            "Please select a sound from the list first.");
        return;
    }

    juce::String selectedName = savedSoundNames[selectedSoundRow];

    // Step 1: Show price and terms
    juce::AlertWindow::showOkCancelBox(
        juce::AlertWindow::QuestionIcon,
        "Terms of Use",
        "Download \"" + selectedName + "\" for $2.99?\n\n"
        "By clicking OK you agree to the terms of use:\n"
        "- This sound is for personal use only.\n"
        "- Redistribution is not permitted.\n"
        "- No refunds after download.",
        "OK - I Agree",
        "Cancel",
        nullptr,
        juce::ModalCallbackFunction::create([this, selectedName](int result)
        {
            if (result != 1)
                return;

            // Step 2: Let user choose where to save it on their computer
            fileChooser = std::make_unique<juce::FileChooser>(
                "Save Sound To Your Computer",
                juce::File::getSpecialLocation(juce::File::userDesktopDirectory)
                    .getChildFile(selectedName + ".wav"),
                "*.wav");

            fileChooser->launchAsync(
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [this](const juce::FileChooser& chooser)
{
    juce::File destFile = chooser.getResult();
    if (destFile == juce::File{})
        return;

    auto* sound = inMemorySounds[selectedSoundRow];

    // Apply pitch by resampling the buffer
    float sliderVal = pitchSlider.getValue();
    float rate = 0.5f * std::pow(4.0f, sliderVal / 100.0f);

    int newNumSamples = (int)(sound->numSamples / rate);
    juce::AudioBuffer<float> pitchedBuffer(sound->buffer.getNumChannels(), newNumSamples);

    for (int ch = 0; ch < sound->buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < newNumSamples; ++i)
        {
            float srcPos = i * rate;
            int srcIndex = (int)srcPos;
            float frac = srcPos - srcIndex;

            int nextIndex = juce::jmin(srcIndex + 1, sound->numSamples - 1);
            srcIndex = juce::jmin(srcIndex, sound->numSamples - 1);

            // Linear interpolation between samples
            float sample = sound->buffer.getSample(ch, srcIndex) * (1.0f - frac)
                         + sound->buffer.getSample(ch, nextIndex) * frac;

            pitchedBuffer.setSample(ch, i, sample);
        }
    }

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::FileOutputStream> outputStream(destFile.createOutputStream());

    if (!outputStream)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Download Error",
            "Could not write to the selected location.");
        return;
    }

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(outputStream.release(),
                                  sound->sampleRate,
                                  pitchedBuffer.getNumChannels(),
                                  16, {}, 0));

    if (!writer)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Download Error",
            "Could not create WAV writer.");
        return;
    }

    writer->writeFromAudioSampleBuffer(pitchedBuffer, 0, newNumSamples);

    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::InfoIcon,
        "Download Complete",
        "\"" + savedSoundNames[selectedSoundRow] + "\" has been downloaded to:\n" +
        destFile.getFullPathName());
});
        }));
};

    // Audio
    formatManager.registerBasicFormats();


deviceManager.initialise(2, 2, nullptr, true);
deviceManager.addAudioCallback(this);


juce::StringArray inputDevices;

juce::OwnedArray<juce::AudioIODeviceType> types;
deviceManager.createAudioDeviceTypes(types);

for (auto* type : types)
{
    type->scanForDevices();
    auto names = type->getDeviceNames(true);
    inputDevices.addArray(names);
}

if (!inputDevices.isEmpty())
{
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);

    setup.inputDeviceName = inputDevices[0];

    setup.inputChannels = juce::BigInteger();
    setup.inputChannels.setRange(0, 2, true);

    setup.outputDeviceName = deviceManager.getCurrentAudioDevice()
        ? deviceManager.getCurrentAudioDevice()->getName()
        : "";

    setup.outputChannels = deviceManager.getCurrentAudioDevice()
        ? deviceManager.getCurrentAudioDevice()->getActiveOutputChannels()
        : juce::BigInteger(0x3);

    deviceManager.setAudioDeviceSetup(setup, true);
}
logoutButton.setButtonText("Logout");

logoutButton.onClick = [this]()
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    bufferSource.reset();

    loggedInAsGuest = false;
    usernameField_login.clear();
    passwordField_login.clear();
    loginRoleSelector.setSelectedId(1);
    loginRoleSelector.setEnabled(roleChoiceUnlocked());

    displayBuffer = nullptr;      
    displayNumSamples = 0;        

    currentState = AppState::LOGIN;
    updateVisibility();
    resized();
};

}

void MainComponent::createGuestAccount() {
    juce::String guestUser = "guest" + juce::String(random.nextInt(1000));
    juce::String guestPass = "guest";
    saveGuestInfo(guestUser, guestPass);

    // Transition back to login screen
    currentState = AppState::LOGIN;
    usernameField_login.clear();
    passwordField_login.clear();
    loginRoleSelector.setEnabled(roleChoiceUnlocked());
    loginRoleSelector.setSelectedId(2); // pre-select Guest
    updateVisibility();
    repaint();
    resized();

    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::InfoIcon,
        "Guest Account Created",
        "Guest account created successfully.\n\nUsername: " + guestUser + "\nPassword: " + guestPass
    );
}

bool MainComponent::isValidPassword(const juce::String& password) const
{
    bool hasUpper = false;
    bool hasNumber = false;
    bool hasSpecial = false;

    for (int i = 0; i < password.length(); ++i)
    {
        juce::juce_wchar c = password[i];

        if (juce::CharacterFunctions::isUpperCase(c)) hasUpper = true;
        if (juce::CharacterFunctions::isDigit(c))     hasNumber = true;
        if (!juce::CharacterFunctions::isLetterOrDigit(c)) hasSpecial = true;
    }

    return hasUpper && hasNumber && hasSpecial;
}

void MainComponent::updateVisibility()
{
    bool setupVisible = (currentState == AppState::FIRST_USER_SETUP);
    bool loginVisible = (currentState == AppState::LOGIN);
    bool mainVisible  = (currentState == AppState::MAIN_APP);

    bool isOwner = mainVisible && isOwnerLoggedIn();
    bool isGuest = mainVisible && isGuestLoggedIn();

    // SHOW CREATE ACCOUNT
    usernameLabel_setup.setVisible(setupVisible);
    usernameField_setup.setVisible(setupVisible);
    passwordLabel_setup.setVisible(setupVisible);
    passwordField_setup.setVisible(setupVisible);
    accountInfoLabel_setup.setVisible(setupVisible);
    accountInfoField_setup.setVisible(setupVisible);
    roleLabel_setup.setVisible(setupVisible);
    roleSelector_setup.setVisible(setupVisible);
    submitButton.setVisible(setupVisible);

    usernameLabel_login.setVisible(loginVisible);
    usernameField_login.setVisible(loginVisible);
    passwordLabel_login.setVisible(loginVisible);
    passwordField_login.setVisible(loginVisible);
    loginRoleLabel.setVisible(loginVisible);
    loginRoleSelector.setVisible(loginVisible);
    loginButton.setVisible(loginVisible);
    forgotButton.setVisible(loginVisible);


    mainAppPlaceholder.setVisible(mainVisible);
    soundsListLabel.setVisible(mainVisible);
    soundList.setVisible(mainVisible);
    waveformLabel.setVisible(mainVisible);
    clusterMapLabel.setVisible(mainVisible);
    recordingStatusLabel.setVisible(mainVisible);

    recordButton.setVisible(isOwner);
    saveButton.setVisible(isOwner);
    createGuestButton.setVisible(isOwner);

    playButton.setVisible(isOwner || isGuest);
    stopButton.setVisible(isOwner || isGuest);
    logoutButton.setVisible(isOwner || isGuest);
    pitchLabel.setVisible(isOwner || isGuest);
    pitchSlider.setVisible(isOwner || isGuest);
    lengthLabel.setVisible(isOwner || isGuest);
    lengthSlider.setVisible(isOwner || isGuest);
    volumeLabel.setVisible(isOwner || isGuest);
    volumeSlider.setVisible(isOwner || isGuest);
    downloadButton.setVisible(isGuest);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e2f));

    if (currentState == AppState::MAIN_APP)
    {
        g.setColour(juce::Colours::darkslategrey);
        g.fillRoundedRectangle(waveformArea.toFloat(), 8.0f);
        g.fillRoundedRectangle(clusterArea.toFloat(), 8.0f);

        drawWaveform(g);
        drawClusterMap(g);

        // Draw blinking recording dot
        if (isRecording && blinkState)
        {
            auto recBounds = recordButton.getBounds();
            int dotX = recBounds.getX() - 18;
            int dotY = recBounds.getCentreY() - 6;
            g.setColour(juce::Colours::red);
            g.fillEllipse((float)dotX, (float)dotY, 12.0f, 12.0f);
        }
    }
}

void MainComponent::resized()
{
    menuBar.setBounds(0, 0, getWidth(), 24);

    auto area = getLocalBounds().reduced(16);
    area.removeFromTop(24);

    const int labelHeight = 24;
    const int fieldHeight = 32;
    const int buttonHeight = 34;
    const int spacing = 10;

    if (currentState == AppState::FIRST_USER_SETUP)
    {
    usernameLabel_setup.setBounds(50, 50, 200, 25);
    usernameField_setup.setBounds(50, 80, 300, 30);

    passwordLabel_setup.setBounds(50, 120, 200, 25);
    passwordField_setup.setBounds(50, 150, 300, 30);

    accountInfoLabel_setup.setBounds(50, 190, 200, 25);
    accountInfoField_setup.setBounds(50, 220, 300, 30);

    roleLabel_setup.setBounds(50, 260, 200, 25);
    roleSelector_setup.setBounds(50, 290, 300, 30);

    submitButton.setBounds(50, 340, 150, 40);
    }

    if (currentState == AppState::LOGIN)
    {
        auto form = area.removeFromTop(300);
        int centreX = getWidth() / 2 - 180;

        usernameLabel_login.setBounds(centreX, form.getY(), 360, labelHeight);
        form.removeFromTop(labelHeight);
        usernameField_login.setBounds(centreX, form.getY(), 360, fieldHeight);
        form.removeFromTop(fieldHeight + spacing);

        passwordLabel_login.setBounds(centreX, form.getY(), 360, labelHeight);
        form.removeFromTop(labelHeight);
        passwordField_login.setBounds(centreX, form.getY(), 360, fieldHeight);
        form.removeFromTop(fieldHeight + spacing);

        loginRoleLabel.setBounds(centreX, form.getY(), 360, labelHeight);
        form.removeFromTop(labelHeight);
        loginRoleSelector.setBounds(centreX, form.getY(), 360, fieldHeight);
        form.removeFromTop(fieldHeight + spacing + 8);

        loginButton.setBounds(centreX + 110, form.getY(), 140, buttonHeight);
        forgotButton.setBounds(centreX, loginButton.getBottom() + 8, 360, buttonHeight);

    }
    else if (currentState == AppState::MAIN_APP)
{
    mainAppPlaceholder.setBounds(area.removeFromTop(36));

    auto topSection = area.removeFromTop(250);
    auto leftTop = topSection.removeFromLeft(240);
    topSection.removeFromLeft(20);
    auto rightTop = topSection;

    soundsListLabel.setBounds(leftTop.removeFromTop(24));
    soundList.setBounds(leftTop);

    auto waveformHeader = rightTop.removeFromTop(24);
    waveformLabel.setBounds(waveformHeader.removeFromLeft(200));
    recordingStatusLabel.setBounds(waveformHeader.removeFromRight(220));
    waveformArea = rightTop.reduced(0, 4);

    area.removeFromTop(12);

    auto controlsSection = area.removeFromTop(170);
    auto buttonsArea = controlsSection.removeFromLeft(220); // ← declared here

    if (isOwnerLoggedIn())
    {
        recordButton.setBounds(buttonsArea.removeFromTop(buttonHeight));
        buttonsArea.removeFromTop(8);
        playButton.setBounds(buttonsArea.removeFromTop(buttonHeight));
        buttonsArea.removeFromTop(8);
        stopButton.setBounds(buttonsArea.removeFromTop(buttonHeight));
        buttonsArea.removeFromTop(8);
        saveButton.setBounds(buttonsArea.removeFromTop(buttonHeight));
        buttonsArea.removeFromTop(8);
        createGuestButton.setBounds(buttonsArea.removeFromTop(buttonHeight));
        buttonsArea.removeFromTop(8);
        logoutButton.setBounds(buttonsArea.removeFromTop(buttonHeight));
    }
    else if (isGuestLoggedIn())
    {
        playButton.setBounds(buttonsArea.removeFromTop(buttonHeight));
        buttonsArea.removeFromTop(8);
        stopButton.setBounds(buttonsArea.removeFromTop(buttonHeight));
        buttonsArea.removeFromTop(8);
        downloadButton.setBounds(buttonsArea.removeFromTop(buttonHeight));
        buttonsArea.removeFromTop(8);
        logoutButton.setBounds(buttonsArea.removeFromTop(buttonHeight));
    }

    controlsSection.removeFromLeft(20);

    auto slidersArea = controlsSection;
    pitchLabel.setBounds(slidersArea.removeFromTop(22));
    pitchSlider.setBounds(slidersArea.removeFromTop(32));
    slidersArea.removeFromTop(8);

    lengthLabel.setBounds(slidersArea.removeFromTop(22));
    lengthSlider.setBounds(slidersArea.removeFromTop(32));
    slidersArea.removeFromTop(8);

    volumeLabel.setBounds(slidersArea.removeFromTop(22));
    volumeSlider.setBounds(slidersArea.removeFromTop(32));

    area.removeFromTop(12);

    clusterMapLabel.setBounds(area.removeFromTop(24));
    clusterArea = area;
}
}

void MainComponent::saveUserInfo(const juce::String& username,
    const juce::String& password,
    const juce::String& accountInfo,
    const juce::String& role)
{
    if (userStorage == nullptr)
        return;

    userStorage->setValue("username", username);
    userStorage->setValue("password", password);
    userStorage->setValue("accountInfo", accountInfo);
    userStorage->setValue("role", role);
    userStorage->saveIfNeeded();
}

void MainComponent::loadUserInfo(juce::String& username,
    juce::String& password,
    juce::String& accountInfo,
    juce::String& role) const
{
    if (userStorage == nullptr)
    {
        username.clear();
        password.clear();
        accountInfo.clear();
        role.clear();
        return;
    }

    username = userStorage->getValue("username", "");
    password = userStorage->getValue("password", "");
    accountInfo = userStorage->getValue("accountInfo", "");
    role = userStorage->getValue("role", "");
}

void MainComponent::saveGuestInfo(const juce::String& username, const juce::String& password)
{
    if (userStorage == NULL)
        return;

    userStorage->setValue("guestUsername", username);
    userStorage->setValue("guestPassword", password);
    userStorage->saveIfNeeded();
}

void MainComponent::loadGuestInfo(juce::String& username, juce::String& password) const
{
    if (userStorage == nullptr)
    {
        username.clear();
        password.clear();
        return;
    }

    username = userStorage->getValue("guestUsername", "");
    password = userStorage->getValue("guestPassword", "");
}

void MainComponent::refreshSoundListFromStorage()
{
    savedSoundNames.clear();
    savedSoundPaths.clear();
    inMemorySounds.clear();

    if (userStorage == nullptr)
        return;

    juce::String namesJoined = userStorage->getValue("savedSoundNames", "");
    juce::String pathsJoined = userStorage->getValue("savedSoundPaths", "");

    if (!namesJoined.isEmpty())
        savedSoundNames = juce::StringArray::fromTokens(namesJoined, "|", "");
    if (!pathsJoined.isEmpty())
        savedSoundPaths = juce::StringArray::fromTokens(pathsJoined, "|", "");

    savedSoundNames.removeEmptyStrings();
    savedSoundPaths.removeEmptyStrings();

    if (savedSoundNames.size() != savedSoundPaths.size())
    {
        int minSize = juce::jmin(savedSoundNames.size(), savedSoundPaths.size());
        savedSoundNames.removeRange(minSize, savedSoundNames.size() - minSize);
        savedSoundPaths.removeRange(minSize, savedSoundPaths.size() - minSize);
    }

    // Reload each file back into inMemorySounds
    for (int i = 0; i < savedSoundPaths.size(); ++i)
    {
        juce::File f(savedSoundPaths[i]);
        if (!f.existsAsFile()) continue;

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(f));
        if (reader == nullptr) continue;

        auto* sound = new SavedSound();
        sound->name = savedSoundNames[i];
        sound->sampleRate = reader->sampleRate;
        sound->numSamples = (int)reader->lengthInSamples;
        sound->buffer.setSize((int)reader->numChannels, sound->numSamples);
        reader->read(&sound->buffer, 0, sound->numSamples, 0, true, true);
        inMemorySounds.add(sound);
    }

    soundList.updateContent();
}

void MainComponent::saveSoundListToStorage()
{
    if (userStorage == nullptr)
        return;

    userStorage->setValue("savedSoundNames", savedSoundNames.joinIntoString("||"));
    userStorage->setValue("savedSoundPaths", savedSoundPaths.joinIntoString("||"));
    userStorage->saveIfNeeded();
}

void MainComponent::addSavedSound(const juce::String& name, const juce::String& path)
{
    savedSoundNames.add(name);
    savedSoundPaths.add(path);
    saveSoundListToStorage();
}

bool MainComponent::isOwnerLoggedIn() const
{
    return currentState == AppState::MAIN_APP && !loggedInAsGuest;
}

bool MainComponent::isGuestLoggedIn() const
{
    return currentState == AppState::MAIN_APP && loggedInAsGuest;
}


void MainComponent::playRecordedAudio()
{
    if (recordingPosition == 0)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Playback Error",
            "No recording to play!");
        return;
    }

    transportSource.stop();
    transportSource.setSource(nullptr);
    bufferSource.reset();

    playbackBuffer.makeCopyOf(recordingBuffer);
    playbackBuffer.setSize(playbackBuffer.getNumChannels(), recordingPosition, true, false, false);

    displayBuffer = &playbackBuffer;       
    displayNumSamples = recordingPosition;

    bufferSource = std::make_unique<BufferAudioSource>(playbackBuffer, recordingPosition);

    // --- FIXED pitch mapping ---
    float sliderVal = pitchSlider.getValue(); // 0-100
    float minRate = 0.5f;  // one octave down
    float maxRate = 2.0f;  // one octave up
    float normalized = sliderVal / 100.0f;
    float rate = minRate * std::pow(maxRate/minRate, normalized);

    bufferSource->setPlaybackRate(rate);

    // Trim
    int trimSamples = static_cast<int>((lengthSlider.getValue() / 100.0) * recordingPosition);
    bufferSource->setTrimLength(trimSamples);

    // Volume
    float volValue = volumeSlider.getValue();
    float dB = juce::jmap(volValue, 0.0f, 100.0f, -60.0f, 0.0f);
    bufferSource->setVolume(juce::Decibels::decibelsToGain(dB));

    // Connect to transport
    auto* device = deviceManager.getCurrentAudioDevice();
    double sampleRate = device ? device->getCurrentSampleRate() : 44100.0;

    transportSource.setSource(bufferSource.get(), 0, nullptr, sampleRate);
    transportSource.start();
    repaint();
}

void MainComponent::loadAudioFileForPlayback(const juce::File& file)
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    currentAudioFile.reset();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Audio Error",
            "Could not open selected audio file.");
        return;
    }

    currentAudioFile.reset(new juce::AudioFormatReaderSource(reader.release(), true));
    transportSource.setSource(currentAudioFile.get(), 0, nullptr, currentSampleRate);
}

void MainComponent::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device != nullptr)
    {
        currentSampleRate = device->getCurrentSampleRate();
        transportSource.prepareToPlay(device->getCurrentBufferSizeSamples(), currentSampleRate);
    }
}

void MainComponent::audioDeviceStopped()
{
    transportSource.releaseResources();
}

void MainComponent::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext&)

{
    if (isRecording && inputChannelData != nullptr)
    {
        int remaining = recordingBuffer.getNumSamples() - recordingPosition;
        int numSamplesToCopy = juce::jmin(numSamples, remaining);

        for (int ch = 0; ch < juce::jmin(numInputChannels, recordingBuffer.getNumChannels()); ++ch)
        {
            if (inputChannelData[ch] != nullptr && numSamplesToCopy > 0)
                recordingBuffer.copyFrom(ch, recordingPosition, inputChannelData[ch], numSamplesToCopy);
        }

        recordingPosition += numSamplesToCopy;

        juce::MessageManager::callAsync([this]()
        {   
    recordingStatusLabel.setText("Recording: " + juce::String(recordingPosition) + " samples",
        juce::dontSendNotification);
        });

        if (recordingPosition >= recordingBuffer.getNumSamples())
        {
            isRecording = false;

            juce::MessageManager::callAsync([this]()
                {
                    recordingStatusLabel.setText("Max recording time reached", juce::dontSendNotification);
                    repaint();

                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::InfoIcon,
                        "Recording Stopped",
                        "Maximum recording time reached (10 seconds).");
                });
        }
    }

    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        if (outputChannelData[channel] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
    }

    juce::AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels, numSamples);
    juce::AudioSourceChannelInfo info(&outputBuffer, 0, numSamples);
    transportSource.getNextAudioBlock(info);
}

int MainComponent::getNumRows()
{
    DBG("savedSoundNames: " + juce::String(savedSoundNames.size()));
    return savedSoundNames.size();
}

void MainComponent::paintListBoxItem(int rowNumber,
    juce::Graphics& g,
    int width,
    int height,
    bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(juce::Colours::steelblue);

    g.setColour(juce::Colours::white);

    if (rowNumber >= 0 && rowNumber < savedSoundNames.size())
        g.drawText(savedSoundNames[rowNumber], 8, 0, width - 8, 15, juce::Justification::centredLeft);
}

void MainComponent::selectedRowsChanged(int lastRowSelected)
{
    selectedSoundRow = lastRowSelected;
}

void MainComponent::drawWaveform(juce::Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.drawRect(waveformArea);

    if (displayBuffer == nullptr || displayNumSamples <= 1)
    {
        g.setColour(juce::Colours::lightgrey);
        g.drawText("No waveform yet", waveformArea, juce::Justification::centred);
        return;
    }

    const float* samples = displayBuffer->getReadPointer(0);

    // Find the actual peak amplitude
    float peak = 0.0f;
    for (int i = 0; i < displayNumSamples; ++i)
        peak = juce::jmax(peak, std::abs(samples[i]));

    if (peak < 0.0001f)
        peak = 1.0f;

    g.setColour(juce::Colours::cyan);

    int midY = waveformArea.getCentreY();
    int width = waveformArea.getWidth();

    juce::Path path;
    path.startNewSubPath((float)waveformArea.getX(), (float)midY);

    int samplesPerPixel = juce::jmax(1, displayNumSamples / juce::jmax(1, width));

    for (int x = 0; x < width; ++x)
    {
        int sampleIndex = juce::jmin(displayNumSamples - 1, x * samplesPerPixel);

        float maxSample = 0.0f;
        for (int s = sampleIndex; s < juce::jmin(sampleIndex + samplesPerPixel, displayNumSamples); ++s)
            maxSample = juce::jmax(maxSample, std::abs(samples[s]));

        float signedSample = samples[sampleIndex] >= 0 ? maxSample : -maxSample;

        float y = juce::jmap(signedSample / peak, -1.0f, 1.0f,
            (float)waveformArea.getBottom() - 4,
            (float)waveformArea.getY() + 4);

        path.lineTo((float)(waveformArea.getX() + x), y);
    }

    g.strokePath(path, juce::PathStrokeType(2.0f));

    g.setColour(juce::Colours::grey);
    g.drawHorizontalLine(midY, (float)waveformArea.getX(), (float)waveformArea.getRight());

    g.setColour(juce::Colours::orange);
    g.drawText("Pitch: " + juce::String(pitchSlider.getValue(), 1) +
        "   Length: " + juce::String(lengthSlider.getValue(), 1) +
        "   Volume: " + juce::String(volumeSlider.getValue(), 2),
        waveformArea.reduced(8), juce::Justification::topRight);
}

void MainComponent::drawClusterMap(juce::Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.drawRect(clusterArea);

    if (savedSoundNames.isEmpty())
    {
        g.setColour(juce::Colours::lightgrey);
        g.drawText("No saved sounds to cluster yet", clusterArea, juce::Justification::centred);
        return;
    }

    g.setColour(juce::Colours::grey);
    g.drawLine((float)clusterArea.getX() + 30.0f,
        (float)clusterArea.getBottom() - 30.0f,
        (float)clusterArea.getRight() - 10.0f,
        (float)clusterArea.getBottom() - 30.0f, 1.5f);

    g.drawLine((float)clusterArea.getX() + 30.0f,
        (float)clusterArea.getBottom() - 30.0f,
        (float)clusterArea.getX() + 30.0f,
        (float)clusterArea.getY() + 10.0f, 1.5f);

    g.setColour(juce::Colours::white);
    g.drawText("Similarity / Length", clusterArea.getX() + 40, clusterArea.getBottom() - 25, 180, 20, juce::Justification::left);
    g.drawText("Similarity", clusterArea.getX() + 2, clusterArea.getY() + 10, 60, 20, juce::Justification::left);

    juce::Array<juce::Colour> colours;
    colours.add(juce::Colours::hotpink);
    colours.add(juce::Colours::yellow);
    colours.add(juce::Colours::limegreen);
    colours.add(juce::Colours::orange);
    colours.add(juce::Colours::aqua);

    int usableWidth = clusterArea.getWidth() - 70;
    clusterDots.clear(); // rebuild dot positions every paint

    for (int i = 0; i < savedSoundNames.size(); ++i)
    {
        int clusterIndex = i % 3;
        int baseX = clusterArea.getX() + 80 + clusterIndex * (usableWidth / 3);
        int baseY = clusterArea.getY() + 60 + clusterIndex * 20;

        int x = baseX + (i % 4) * 20;
        int y = baseY + (i % 5) * 18;

        // Store dot position
        clusterDots.add({ x, y, i });

        // Highlight if selected
        if (i == selectedSoundRow)
        {
            g.setColour(juce::Colours::white);
            g.drawEllipse((float)x - 2, (float)y - 2, 16.0f, 16.0f, 2.0f);
        }

        g.setColour(colours[i % colours.size()]);
        g.fillEllipse((float)x, (float)y, 12.0f, 12.0f);

        // Draw sound name next to dot
        g.setColour(juce::Colours::white);
        g.setFont(11.0f);
        g.drawText(savedSoundNames[i], x + 15, y, 80, 12, juce::Justification::centredLeft);
    }
}

void MainComponent::mouseDown(const juce::MouseEvent& e)
{
    if (currentState != AppState::MAIN_APP)
        return;

    // Check if click is inside cluster area
    if (!clusterArea.contains(e.getPosition()))
        return;

    const int hitRadius = 10;

    for (auto& dot : clusterDots)
    {
        int cx = dot.x + 6; // centre of the 12px ellipse
        int cy = dot.y + 6;

        if (e.getPosition().getDistanceFrom(juce::Point<int>(cx, cy)) <= hitRadius)
        {
            // Select this sound in the list
            selectedSoundRow = dot.soundIndex;
            soundList.selectRow(selectedSoundRow);

            // If double clicked, play it
            if (e.getNumberOfClicks() == 2 && dot.soundIndex < inMemorySounds.size())
            {
                auto* sound = inMemorySounds[dot.soundIndex];
                transportSource.stop();
                transportSource.setSource(nullptr);
                bufferSource.reset();

                playbackBuffer.makeCopyOf(sound->buffer);
                playbackBuffer.setSize(playbackBuffer.getNumChannels(), sound->numSamples, true, false, false);

                bufferSource = std::make_unique<BufferAudioSource>(playbackBuffer, sound->numSamples);

                float sliderVal = pitchSlider.getValue();
                float rate = 0.5f * std::pow(4.0f, sliderVal / 100.0f);
                bufferSource->setPlaybackRate(rate);

                int trimSamples = static_cast<int>((lengthSlider.getValue() / 100.0) * sound->numSamples);
                bufferSource->setTrimLength(trimSamples);

                float dB = juce::jmap((float)volumeSlider.getValue(), 0.0f, 100.0f, -60.0f, 0.0f);
                bufferSource->setVolume(juce::Decibels::decibelsToGain(dB));

                transportSource.setSource(bufferSource.get(), 0, nullptr, sound->sampleRate);
                transportSource.start();
                repaint();
            }

            repaint();
            return;
        }
    }
}

void MainComponent::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    selectedSoundRow = row;

    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Delete \"" + savedSoundNames[row] + "\"");

        menu.showMenuAsync(juce::PopupMenu::Options(), [this, row](int result)
        {
            if (result == 1)
            {
                juce::AlertWindow::showOkCancelBox(
                    juce::AlertWindow::WarningIcon,
                    "Delete Sound",
                    "Are you sure you want to delete \"" + savedSoundNames[row] + "\"?",
                    "Delete",
                    "Cancel",
                    nullptr,
                    juce::ModalCallbackFunction::create([this, row](int confirmed)
                    {
                        if (confirmed == 1)
                        {
                            // Delete the file from disk too
                            juce::File f(savedSoundPaths[row]);
                            if (f.existsAsFile())
                                f.deleteFile();

                            inMemorySounds.remove(row);
                            savedSoundNames.remove(row);
                            savedSoundPaths.remove(row);
                            selectedSoundRow = -1;

                            // Persist the updated list
                            saveSoundListToStorage();
                            soundList.deselectAllRows();
                            soundList.updateContent();
                            repaint();
                        }
                    }));
            }
        });
    }
}

void MainComponent::timerCallback()
{
    blinkState = !blinkState;
    repaint();
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return {"File", "Edit"};
}

juce::PopupMenu MainComponent::getMenuForIndex(int menuIndex, const juce::String& menuName)
{
    juce::ignoreUnused(menuName);

    juce::PopupMenu menu;

    if (menuIndex == 0) // File menu
    {
        if (isOwnerLoggedIn())
        {
            menu.addItem(1, "Load");
            menu.addItem(2, "Create Guest Account");
        }
        menu.addItem(3, "Exit");
    }
    else if (menuIndex == 1) // Edit menu
    {
        if (isOwnerLoggedIn())
        {
            menu.addItem(1, "Clear Sound Selection");
            menu.addItem(2, "Clear Sound List");
        }
    }

    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    if (topLevelMenuIndex == 0) { // File menu
        switch (menuItemID)
        {
            case 1: { // Load
                auto chooser = std::make_shared<juce::FileChooser>("Load a sound file", juce::File{}, "*.wav;*.mp3;*.aiff");
                chooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, chooser](const juce::FileChooser& fc) {
                        juce::File f = fc.getResult();
                        if (f.existsAsFile()) {
                            addSavedSound(f.getFileName(), f.getFullPathName());
                            refreshSoundListFromStorage();
                            soundList.updateContent();
                            repaint();
                        }
                    });
                break;
            }
            case 2: // Create Guest Account
                createGuestAccount();
                break;
            case 3: // Exit
                juce::JUCEApplication::getInstance()->systemRequestedQuit();
                break;
        }
    }
    else if (topLevelMenuIndex == 1)
{
    switch (menuItemID)
    {
        case 1: // Clear Recording
            transportSource.stop();
            transportSource.setSource(nullptr);
            bufferSource.reset();
            recordingPosition = 0;
            recordingBuffer.clear();
            displayBuffer = nullptr;
            displayNumSamples = 0;
            recordingStatusLabel.setText("Not recording", juce::dontSendNotification);
            repaint();
            break;

        case 2: // Clear Sound List
            savedSoundNames.clear();
            savedSoundPaths.clear();
            inMemorySounds.clear();
            selectedSoundRow = -1;
            if (userStorage) {
                userStorage->setValue("savedSoundNames", "");
                userStorage->setValue("savedSoundPaths", "");
                userStorage->saveIfNeeded();
            }
            soundList.updateContent();
            soundList.deselectAllRows();
            repaint();
            break;
    }
}
}