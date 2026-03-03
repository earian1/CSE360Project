#include "MainComponent.h"

MainComponent::MainComponent(juce::ApplicationProperties& props)
{
    userStorage = props.getUserSettings();
    setSize(700, 500);

    // Low-saturation indigo theme, slightly darkened overall while keeping textboxes lighter
    {
        auto& lf = getLookAndFeel();
        // darker muted indigo background
        lf.setColour(juce::ResizableWindow::backgroundColourId, juce::Colour::fromRGB(80, 70, 110));
        // general label text color (slightly less bright)
        lf.setColour(juce::Label::textColourId, juce::Colour::fromRGB(230, 230, 240));
        // buttons: darker muted indigo/gray
        lf.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(110, 95, 130));
        lf.setColour(juce::TextButton::textColourOnId, juce::Colour::fromRGB(240, 240, 245));
        lf.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(240, 240, 245));
        // sliders: slightly darker/desaturated
        lf.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(150, 140, 160));
        lf.setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(100, 95, 115));

        // Text editors (textboxes): keep slightly lighter than background but darker than before
        lf.setColour(juce::TextEditor::backgroundColourId, juce::Colour::fromRGB(125, 115, 150));
        lf.setColour(juce::TextEditor::outlineColourId, juce::Colour::fromRGB(105, 95, 125));
        lf.setColour(juce::TextEditor::textColourId, juce::Colour::fromRGB(25, 20, 35));

        // ComboBox / Popups / Lists: match textbox tone (darker)
        lf.setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(125, 115, 150));
        lf.setColour(juce::PopupMenu::backgroundColourId, juce::Colour::fromRGB(135, 125, 160));
        lf.setColour(juce::ListBox::backgroundColourId, juce::Colour::fromRGB(125, 115, 150));
        lf.setColour(juce::ListBox::textColourId, juce::Colour::fromRGB(25, 20, 35));
    }

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


    // ---------- FIRST USER SETUP ----------
    titleLabel.setText("Sound System Login", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, getLookAndFeel().findColour(juce::Label::textColourId));
    addAndMakeVisible(titleLabel);

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
    // make the submit button ocean blue
    submitButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(0, 123, 167));
    submitButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    submitButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(submitButton);
    submitButton.onClick = [this]()
    {
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
            recordingBuffer.setSize(2, 44100 * 10);
            recordingBuffer.clear();

            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Recording", "Recording started!");
        }
        else{
            isRecording = false;
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Recording", "Recording stopped!");
        }

    };

    playButton.onClick = [this]()
    {
        if (isRecording)
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon, 
                "Playback Error", 
                "Cannot play while recording is in progress!");
            return;
        }

        if (recordingPosition == 0)
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon, 
                "Playback Error", 
                "No recording available. Please record first!");
            return;
        }

        playRecordAudio();
    };

    stopButton.onClick = [this]()
    {
        if (isRecording)
        {
            isRecording = false;
        }
        transportSource.stop();
    };

    saveButton.onClick = [this]() 
{
    if (isRecording)
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Save Error", "Please stop recording before saving.");
        return;
    }

    juce::FileChooser fileChooser("Save Recording",
                               juce::File{},
                               "*.wav");

fileChooser.launchAsync(
    juce::FileBrowserComponent::saveMode |
    juce::FileBrowserComponent::canSelectFiles,
    [this](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();

        if (file != juce::File{})
        {
            juce::WavAudioFormat wavFormat;

            std::unique_ptr<juce::AudioFormatWriter> writer(
                wavFormat.createWriterFor(
                    file.createOutputStream().release(),
                    44100,
                    recordingBuffer.getNumChannels(),
                    16,
                    {},
                    0));

            if (writer != nullptr)
                writer->writeFromAudioSampleBuffer(recordingBuffer, 0, recordingPosition);
        }
    });

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
        titleLabel.setBounds(area.removeFromTop(40));
        area.removeFromTop(10);

        usernameLabel_setup.setBounds(area.removeFromTop(labelHeight));
        usernameField_setup.setBounds(area.removeFromTop(fieldHeight).reduced(0, spacing));
        passwordLabel_setup.setBounds(area.removeFromTop(labelHeight));
        passwordField_setup.setBounds(area.removeFromTop(fieldHeight).reduced(0, spacing));
        accountInfoLabel_setup.setBounds(area.removeFromTop(labelHeight));
        accountInfoField_setup.setBounds(area.removeFromTop(fieldHeight).reduced(0, spacing));
        roleLabel_setup.setBounds(area.removeFromTop(labelHeight));
        roleSelector_setup.setBounds(area.removeFromTop(fieldHeight).reduced(0, spacing));
        submitButton.setBounds(area.removeFromTop(buttonHeight).withWidth(buttonWidth).withX(getWidth()/2 - buttonWidth/2));
    }
    else if (currentState == AppState::LOGIN)
    {
        usernameLabel_login.setBounds(area.removeFromTop(labelHeight));
        usernameField_login.setBounds(area.removeFromTop(fieldHeight).reduced(0, spacing));
        passwordLabel_login.setBounds(area.removeFromTop(labelHeight));
        passwordField_login.setBounds(area.removeFromTop(fieldHeight).reduced(0, spacing));
        loginButton.setBounds(area.removeFromTop(buttonHeight).withWidth(buttonWidth).withX(getWidth()/2 - buttonWidth/2));

        area.removeFromTop(spacing);
        backButton.setBounds(area.removeFromTop(buttonHeight).withWidth(buttonWidth).withX(getWidth()/2 - buttonWidth/2));
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
            stopButton.setBounds(area.removeFromTop(buttonHeight));
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

void MainComponent::audioDeviceIOCallbackWithContext (
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext&)
{
    if (isRecording && recordingPosition < recordingBuffer.getNumSamples())
    {
        int samplesToRecord = juce::jmin(numSamples, recordingBuffer.getNumSamples() - recordingPosition);

        for (int channel = 0; channel < juce::jmin(numInputChannels, recordingBuffer.getNumChannels()); ++channel)
        {
            if (inputChannelData[channel] != nullptr)
            {
                auto* bufferChannel = recordingBuffer.getWritePointer(channel, recordingPosition);

                for (int i = 0; i < samplesToRecord; ++i)
                    bufferChannel[i] = inputChannelData[channel][i];
            }
        }
        recordingPosition += samplesToRecord;

        if (recordingPosition >= recordingBuffer.getNumSamples())
        {
            isRecording = false;
        }
    }

    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        if (outputChannelData[channel] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);
    }

    juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);
    juce::AudioSourceChannelInfo channelInfo(&buffer, 0, numSamples);

    transportSource.getNextAudioBlock(channelInfo);
}



void MainComponent::playRecordAudio()
{
    if (recordingPosition == 0 || recordingBuffer.getNumChannels() == 0)
        return;

    transportSource.stop();
    transportSource.setSource(nullptr);

    lastRecordingData.reset();
    auto* memStream = new juce::MemoryOutputStream(lastRecordingData, false);
    juce::WavAudioFormat wavFormat;

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(memStream, 44100, recordingBuffer.getNumChannels(), 16, {}, 0)
    );

    if (writer == nullptr)
        return;

    writer->writeFromAudioSampleBuffer(recordingBuffer, 0, recordingPosition);
    writer.reset();

    if (lastRecordingData.getSize() == 0)
        return;

    auto memInputStream = std::make_unique<juce::MemoryInputStream>(
        lastRecordingData, false
    );

    juce::WavAudioFormat wavFormat2;
    std::unique_ptr<juce::AudioFormatReader> reader(
        wavFormat2.createReaderFor(memInputStream.release(), true)
    );

    if (reader != nullptr)
    {
        auto sampleRate = reader->sampleRate;
        currentAudioFile.reset(new juce::AudioFormatReaderSource(reader.release(), true));
        transportSource.setSource(currentAudioFile.get(), 0, nullptr, sampleRate);
        transportSource.start();
    }
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



