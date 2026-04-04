#include "MainComponent.h"

MainComponent::MainComponent(juce::ApplicationProperties& props)
{
    userStorage = props.getUserSettings();
    setSize(1000, 700);

    addAndMakeVisible(menuBar);
    menuBar.setModel(this);

    ensureDefaultAccounts();
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

    if (userStorage->getValue("username", "").isEmpty())
    {
        userStorage->setValue("username", "guest");
        userStorage->setValue("password", "guest");
        userStorage->setValue("accountInfo", "Default Owner Account");
        userStorage->setValue("role", "Owner");
    }

    if (userStorage->getValue("guestUsername", "").isEmpty())
    {
        userStorage->setValue("guestUsername", "guest");
        userStorage->setValue("guestPassword", "guest");
    }

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

            if (!roleChoiceUnlocked())
                selectedRole = 1;

            if (selectedRole == 1)
            {
                juce::String storedUsername, storedPassword, storedAccountInfo, storedRole;
                loadUserInfo(storedUsername, storedPassword, storedAccountInfo, storedRole);

                if (enteredUsername == storedUsername &&
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
            else if (selectedRole == 2)
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

    pitchSlider.setRange(0.5, 2.0, 0.1);
    lengthSlider.setRange(0.5, 2.0, 0.1);
    volumeSlider.setRange(0.0, 1.0, 0.05);

    pitchSlider.setValue(1.0);
    lengthSlider.setValue(1.0);
    volumeSlider.setValue(0.8);

    volumeSlider.onValueChange = [this]()
        {
            transportSource.setGain((float)volumeSlider.getValue());
            repaint();
        };

    pitchSlider.onValueChange = [this]() { repaint(); };
    lengthSlider.onValueChange = [this]() { repaint(); };

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

    recordButton.onClick = [this]()
        {
            if (!isRecording)
            {
                isRecording = true;
                recordingPosition = 0;

                int totalSamples = (int)(currentSampleRate * 10.0);
                recordingBuffer.setSize(2, totalSamples);
                recordingBuffer.clear();

                recordingStatusLabel.setText("Recording started...", juce::dontSendNotification);
            }
            else
            {
                isRecording = false;
                recordingStatusLabel.setText("Recording stopped", juce::dontSendNotification);

                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::InfoIcon,
                    "Recording",
                    "Recording stopped.");
            }
        };

    playButton.onClick = [this]()
        {
            if (selectedSoundRow >= 0 && selectedSoundRow < savedSoundPaths.size())
            {
                juce::File selectedFile(savedSoundPaths[selectedSoundRow]);

                if (selectedFile.existsAsFile())
                {
                    loadAudioFileForPlayback(selectedFile);
                    transportSource.setGain((float)volumeSlider.getValue());
                    transportSource.start();
                    return;
                }
            }

            playRecordedAudio();
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
                    "No recording available to save.");
                return;
            }

            juce::FileChooser fileChooser("Save Recording", juce::File{}, "*.wav");

            fileChooser.launchAsync(
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [this](const juce::FileChooser& chooser)
                {
                    auto file = chooser.getResult();

                    if (file == juce::File{})
                        return;

                    if (file.getFileExtension() != ".wav")
                        file = file.withFileExtension(".wav");

                    juce::WavAudioFormat wavFormat;
                    std::unique_ptr<juce::FileOutputStream> outputStream(file.createOutputStream());

                    if (outputStream == nullptr)
                    {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon,
                            "Save Error",
                            "Could not create output stream.");
                        return;
                    }

                    std::unique_ptr<juce::AudioFormatWriter> writer(
                        wavFormat.createWriterFor(outputStream.release(),
                            currentSampleRate,
                            recordingBuffer.getNumChannels(),
                            16,
                            {},
                            0));

                    if (writer == nullptr)
                    {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::AlertWindow::WarningIcon,
                            "Save Error",
                            "Failed to create WAV writer.");
                        return;
                    }

                    writer->writeFromAudioSampleBuffer(recordingBuffer, 0, recordingPosition);

                    addSavedSound(file.getFileName(), file.getFullPathName());
                    refreshSoundListFromStorage();
                    soundList.updateContent();
                    repaint();

                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::InfoIcon,
                        "Save Successful",
                        "Recording saved successfully.");
                });
        };

    createGuestButton.onClick = [this]()
        {
            juce::String guestUser = "guest";
            juce::String guestPass = "guest";

            saveGuestInfo(guestUser, guestPass);

            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Guest Account Created",
                "Guest account created successfully.\n\nUsername: " + guestUser +
                "\nPassword: " + guestPass);
        };

    logoutButton.onClick = [this]()
        {
            transportSource.stop();
            usernameField_login.clear();
            passwordField_login.clear();
            currentState = AppState::LOGIN;
            loginRoleSelector.setEnabled(roleChoiceUnlocked());
            loginRoleSelector.setSelectedId(1);
            updateVisibility();
            resized();
        };

    downloadButton.onClick = [this]()
        {
            if (selectedSoundRow < 0 || selectedSoundRow >= savedSoundPaths.size())
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Download Error",
                    "Please select a saved sound first.");
                return;
            }

            juce::NativeMessageBox::showOkCancelBox(
                juce::MessageBoxIconType::QuestionIcon,
                "Terms of Use",
                "Download this sound for $2.99?\nBy clicking OK, you accept the terms of use.",
                this,
                juce::ModalCallbackFunction::create([this](int result)
                    {
                        if (result != 0)
                        {
                            juce::AlertWindow::showMessageBoxAsync(
                                juce::AlertWindow::InfoIcon,
                                "Download Approved",
                                "Sound download approved.");
                        }
                    }));
        };

    // Audio
    formatManager.registerBasicFormats();
    deviceManager.initialiseWithDefaultDevices(2, 2);
    deviceManager.addAudioCallback(this);

    if (auto* device = deviceManager.getCurrentAudioDevice())
        currentSampleRate = device->getCurrentSampleRate();

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        if (device->getActiveInputChannels().countNumberOfSetBits() == 0)
            recordButton.setEnabled(false);
    }
}

void MainComponent::updateVisibility()
{
    bool loginVisible = (currentState == AppState::LOGIN);
    bool mainVisible = (currentState == AppState::MAIN_APP);

    bool isOwner = mainVisible && isOwnerLoggedIn();
    bool isGuest = mainVisible && isGuestLoggedIn();

    usernameLabel_login.setVisible(loginVisible);
    usernameField_login.setVisible(loginVisible);
    passwordLabel_login.setVisible(loginVisible);
    passwordField_login.setVisible(loginVisible);
    loginRoleLabel.setVisible(loginVisible);
    loginRoleSelector.setVisible(loginVisible);
    loginButton.setVisible(loginVisible);

    mainAppPlaceholder.setVisible(mainVisible);
    soundsListLabel.setVisible(mainVisible);
    soundList.setVisible(mainVisible);
    waveformLabel.setVisible(mainVisible);
    clusterMapLabel.setVisible(mainVisible);
    recordingStatusLabel.setVisible(mainVisible);

    recordButton.setVisible(isOwner);
    saveButton.setVisible(isOwner);
    createGuestButton.setVisible(true);

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

        g.setColour(juce::Colours::darkslategrey);
        g.fillRoundedRectangle(clusterArea.toFloat(), 8.0f);

        drawWaveform(g);
        drawClusterMap(g);
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

        auto buttonsArea = controlsSection.removeFromLeft(220);
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
        downloadButton.setBounds(buttonsArea.removeFromTop(buttonHeight));
        buttonsArea.removeFromTop(8);
        logoutButton.setBounds(buttonsArea.removeFromTop(buttonHeight));

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

void MainComponent::saveGuestInfo(juce::String& username, juce::String& password)
{
    if (&userStorage == NULL)
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

    if (userStorage == nullptr)
        return;

    juce::String namesJoined = userStorage->getValue("savedSoundNames", "");
    juce::String pathsJoined = userStorage->getValue("savedSoundPaths", "");

    if (!namesJoined.isEmpty())
        savedSoundNames.addTokens(namesJoined, "||", "");

    if (!pathsJoined.isEmpty())
        savedSoundPaths.addTokens(pathsJoined, "||", "");

    if (savedSoundNames.size() != savedSoundPaths.size())
    {
        int minSize = juce::jmin(savedSoundNames.size(), savedSoundPaths.size());
        savedSoundNames.removeRange(minSize, savedSoundNames.size() - minSize);
        savedSoundPaths.removeRange(minSize, savedSoundPaths.size() - minSize);
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
            "No recording available to play.");
        return;
    }

    transportSource.stop();
    transportSource.setSource(nullptr);
    currentAudioFile.reset();

    juce::MemoryOutputStream memStream;
    juce::WavAudioFormat wavFormat;

    {
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(&memStream,
                currentSampleRate,
                recordingBuffer.getNumChannels(),
                16,
                {},
                0));

        if (writer != nullptr)
            writer->writeFromAudioSampleBuffer(recordingBuffer, 0, recordingPosition);
    }

    auto memInputStream = std::make_unique<juce::MemoryInputStream>(
        memStream.getData(), memStream.getDataSize(), false);

    std::unique_ptr<juce::AudioFormatReader> reader(
        wavFormat.createReaderFor(memInputStream.release(), true));

    if (reader == nullptr)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Playback Error",
            "Failed to create audio reader.");
        return;
    }

    currentAudioFile.reset(new juce::AudioFormatReaderSource(reader.release(), true));
    transportSource.setSource(currentAudioFile.get(), 0, nullptr, currentSampleRate);
    transportSource.setGain((float)volumeSlider.getValue());
    transportSource.start();
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
        g.drawText(savedSoundNames[rowNumber], 8, 0, width - 8, height, juce::Justification::centredLeft);
}

void MainComponent::selectedRowsChanged(int lastRowSelected)
{
    selectedSoundRow = lastRowSelected;
}

void MainComponent::drawWaveform(juce::Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.drawRect(waveformArea);

    if (recordingPosition <= 1)
    {
        g.setColour(juce::Colours::lightgrey);
        g.drawText("No waveform yet", waveformArea, juce::Justification::centred);
        return;
    }

    g.setColour(juce::Colours::cyan);

    int midY = waveformArea.getCentreY();
    int width = waveformArea.getWidth();

    juce::Path path;
    path.startNewSubPath((float)waveformArea.getX(), (float)midY);

    const float* samples = recordingBuffer.getReadPointer(0);
    int samplesPerPixel = juce::jmax(1, recordingPosition / juce::jmax(1, width));

    for (int x = 0; x < width; ++x)
    {
        int sampleIndex = juce::jmin(recordingPosition - 1, x * samplesPerPixel);
        float sample = samples[sampleIndex];
        float y = juce::jmap(sample, -1.0f, 1.0f,
            (float)waveformArea.getBottom(),
            (float)waveformArea.getY());
        path.lineTo((float)(waveformArea.getX() + x), y);
    }

    g.strokePath(path, juce::PathStrokeType(2.0f));

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

    for (int i = 0; i < savedSoundNames.size(); ++i)
    {
        int clusterIndex = i % 3;
        int baseX = clusterArea.getX() + 80 + clusterIndex * (usableWidth / 3);
        int baseY = clusterArea.getY() + 60 + clusterIndex * 20;

        int x = baseX + (i % 4) * 20;
        int y = baseY + (i % 5) * 18;

        g.setColour(colours[i % colours.size()]);
        g.fillEllipse((float)x, (float)y, 12.0f, 12.0f);
    }
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File", "Edit" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int menuIndex, const juce::String& menuName)
{
    juce::ignoreUnused(menuName);

    juce::PopupMenu menu;

    if (menuIndex == 0)
        menu.addItem(1, "Exit");
    else if (menuIndex == 1)
        menu.addItem(2, "Clear Sound Selection");

    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    juce::ignoreUnused(topLevelMenuIndex);

    if (menuItemID == 1)
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
    else if (menuItemID == 2)
    {
        selectedSoundRow = -1;
        soundList.deselectAllRows();
    }
}