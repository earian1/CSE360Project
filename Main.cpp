#include <JuceHeader.h>
#include "MainComponent.h"

//==============================================================================
class GuiAppApplication final : public juce::JUCEApplication
{
public:
    //==============================================================================
    GuiAppApplication() {}

    juce::ApplicationProperties appProperties;

    const juce::String getApplicationName() override       { return "My JUCE App"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    //==============================================================================
    void initialise (const juce::String& commandLine) override
    {
        juce::ignoreUnused (commandLine);

        // --- Setup storage options FIRST ---
        juce::PropertiesFile::Options options;
        options.applicationName = "SoundManagerApp";
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        options.folderName = "SoundManagerAppData";
        options.storageFormat = juce::PropertiesFile::StorageFormat::storeAsXML;

        appProperties.setStorageParameters(options);

        // --- Then create main window ---
        mainWindow.reset(new MainWindow(getApplicationName(), appProperties));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
        juce::ignoreUnused (commandLine);
    }

    //==============================================================================
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (juce::String name, juce::ApplicationProperties& props)
            : DocumentWindow (name,
                              juce::Desktop::getInstance().getDefaultLookAndFeel()
                                  .findColour (juce::ResizableWindow::backgroundColourId),
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);

            // Pass the already-initialized appProperties to MainComponent
            setContentOwned (new MainComponent(props), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
           #else
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
           #endif

            setVisible(true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
         
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION (GuiAppApplication)
