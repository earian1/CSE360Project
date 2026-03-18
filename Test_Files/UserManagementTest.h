#pragma once
#include <cppunit/extensions/HelperMacros.h>
#include <cstdio>  // for std::remove()

// Existing mocks (Sprint 1 & 2)
#include "MainComponentLogin.h"
#include "RecordingManager.h"

// New mocks (Sprint 3)
#include "PasswordValidator.h"
#include "SaveManager.h"
#include "PlaybackManager.h"
#include "MockSliderAndPermission.h"

class UserManagementTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(UserManagementTest);

    // --- Sprint 1 & 2 (kept from previous sprints) ---
    CPPUNIT_TEST(testUserLogin);
    CPPUNIT_TEST(testAccountInfo);
    CPPUNIT_TEST(testRecordingStatus);

    // --- Sprint 3 (new) ---
    CPPUNIT_TEST(testPasswordValidation);
    CPPUNIT_TEST(testSaveRecording);
    CPPUNIT_TEST(testPlaybackState);
    CPPUNIT_TEST(testVolumeSliderRange);

    CPPUNIT_TEST_SUITE_END();

private:
    // Sprint 1 & 2 mocks
    MainComponentLogin* mainComp       = new MainComponentLogin();
    RecordingManager*   recordingManager = new RecordingManager();

    // Sprint 3 mocks
    PasswordValidator*  passwordValidator = new PasswordValidator();
    SaveManager*        saveManager       = new SaveManager();
    PlaybackManager*    playbackManager   = new PlaybackManager();

public:

    // ----------------------------------------------------------------
    // setUp — runs before every test
    // ----------------------------------------------------------------
    void setUp() override
    {
        // Seed login data used by Sprint 1/2 tests and new password tests
        mainComp->saveUserInfo("Johnny", "Jgarciga1!", "IDK");
        passwordValidator->setCredentials("Johnny", "Jgarciga1!");

        // Reset playback state before each test
        playbackManager->stop();
        playbackManager->loadAudio(false);

        // Reset recording state
        if (recordingManager->getIsRecording())
            recordingManager->stopRecording();
    }

    void tearDown() override {}


    // ================================================================
    //  SPRINT 1 & 2 TESTS  (unchanged)
    // ================================================================

    // ----------------------------------------------------------------
    // testUserLogin
    // Verifies that valid credentials allow login and invalid ones do not.
    // ----------------------------------------------------------------
    void testUserLogin()
    {
        // Correct credentials — should succeed
        CPPUNIT_ASSERT(mainComp->checkLogin("Johnny", "Jgarciga1!"));

        // Wrong password — should fail
        CPPUNIT_ASSERT(!mainComp->checkLogin("johnny", "wrongpass"));
    }

    // ----------------------------------------------------------------
    // testAccountInfo
    // Verifies that account info is returned for the correct user
    // and empty string is returned for an unknown user.
    // ----------------------------------------------------------------
    void testAccountInfo()
    {
        // Correct username — should return saved account info
        CPPUNIT_ASSERT(mainComp->getAccountInfo("Johnny") == "IDK");

        // Wrong username — should return empty
        CPPUNIT_ASSERT(mainComp->getAccountInfo("johnny") == "");
    }

    // ----------------------------------------------------------------
    // testRecordingStatus
    // Verifies that the recording flag toggles correctly on
    // startRecording() and stopRecording().
    // ----------------------------------------------------------------
    void testRecordingStatus()
    {
        // Initially not recording
        CPPUNIT_ASSERT(!recordingManager->getIsRecording());

        // Start recording
        recordingManager->startRecording();
        CPPUNIT_ASSERT(recordingManager->getIsRecording());

        // Stop recording
        recordingManager->stopRecording();
        CPPUNIT_ASSERT(!recordingManager->getIsRecording());
    }


    // ================================================================
    //  SPRINT 3 TESTS  (new)
    // ================================================================

    // ----------------------------------------------------------------
    // testPasswordValidation
    // Verifies that password matching is case-sensitive and that
    // empty usernames / passwords are always rejected.
    // ----------------------------------------------------------------
     void testPasswordValidation()
    {
        // Valid password meeting all rules — should pass
        // "Jgarciga1!" has uppercase (J), number (1), special char (!)
        CPPUNIT_ASSERT(passwordValidator->validate("Johnny", "Jgarciga1!"));
 
        // --- Rule: must have uppercase ---
        // All lowercase, has number and special — should fail
        CPPUNIT_ASSERT(!passwordValidator->meetsRequirements("jgarciga1!"));
 
        // --- Rule: must have a number ---
        // Has uppercase and special but no number — should fail
        CPPUNIT_ASSERT(!passwordValidator->meetsRequirements("Jgarciga!"));
 
        // --- Rule: must have a special character ---
        // Has uppercase and number but no special — should fail
        CPPUNIT_ASSERT(!passwordValidator->meetsRequirements("Jgarciga1"));
 
        // --- Case-sensitivity: wrong case on username --- should fail
        CPPUNIT_ASSERT(!passwordValidator->validate("johnny", "Jgarciga1!"));
 
        // --- Case-sensitivity: wrong case on password --- should fail
        CPPUNIT_ASSERT(!passwordValidator->validate("Johnny", "jgarciga1!"));
 
        // --- Empty username --- should fail
        CPPUNIT_ASSERT(!passwordValidator->validate("", "Jgarciga1!"));
 
        // --- Empty password --- should fail
        CPPUNIT_ASSERT(!passwordValidator->validate("Johnny", ""));
 
        // --- Both empty --- should fail
        CPPUNIT_ASSERT(!passwordValidator->validate("", ""));
    }

    // ----------------------------------------------------------------
    // testSaveRecording
    // Verifies that saveRecording() writes a .wav file to disk and
    // that the file actually exists afterward.
    // ----------------------------------------------------------------
    void testSaveRecording()
    {
        const std::string testFilePath = "test_output.wav";

        // Clean up any leftover file from a previous run
        std::remove(testFilePath.c_str());

        // File should not exist before saving
        CPPUNIT_ASSERT(!saveManager->fileExists(testFilePath));

        // Perform save
        bool saved = saveManager->saveToFile(testFilePath);
        CPPUNIT_ASSERT(saved);

        // File must exist on disk after save
        CPPUNIT_ASSERT(saveManager->fileExists(testFilePath));

        // Saved path should match what we requested
        CPPUNIT_ASSERT(saveManager->getLastSavedPath() == testFilePath);

        // Clean up the test file
        std::remove(testFilePath.c_str());
    }

    // ----------------------------------------------------------------
    // testPlaybackState
    // Verifies the play -> pause -> resume -> stop state machine.
    // Also checks that play is blocked when no audio is loaded.
    // ----------------------------------------------------------------
    void testPlaybackState()
    {
        // Should start in Stopped state
        CPPUNIT_ASSERT(playbackManager->isStopped());

        // Play should be blocked when no audio is loaded
        playbackManager->play();
        CPPUNIT_ASSERT(playbackManager->isStopped());

        // Load audio and try again — should now play
        playbackManager->loadAudio(true);
        playbackManager->play();
        CPPUNIT_ASSERT(playbackManager->isPlaying());

        // Pause — should enter Paused state
        playbackManager->pause();
        CPPUNIT_ASSERT(playbackManager->isPaused());

        // Resume — should go back to Playing
        playbackManager->resume();
        CPPUNIT_ASSERT(playbackManager->isPlaying());

        // Stop — should return to Stopped
        playbackManager->stop();
        CPPUNIT_ASSERT(playbackManager->isStopped());
    }

    // ----------------------------------------------------------------
    // testVolumeSliderRange
    // Verifies that the volume, pitch, and length sliders clamp
    // values within their defined min/max bounds.
    // ----------------------------------------------------------------
    void testVolumeSliderRange()
    {
        // Volume: 0.0 to 1.0  (matches JUCE slider range in MainComponent)
        MockSlider volumeSlider(0.0, 1.0, 0.0);

        // Normal value — should stay as-is
        volumeSlider.setValue(0.5);
        CPPUNIT_ASSERT(volumeSlider.getValue() == 0.5);

        // Over max — should clamp to 1.0
        volumeSlider.setValue(2.0);
        CPPUNIT_ASSERT(volumeSlider.getValue() == 1.0);

        // Under min — should clamp to 0.0
        volumeSlider.setValue(-1.0);
        CPPUNIT_ASSERT(volumeSlider.getValue() == 0.0);

        // Boundary values — should be accepted exactly
        volumeSlider.setValue(0.0);
        CPPUNIT_ASSERT(volumeSlider.isInRange(volumeSlider.getValue()));
        volumeSlider.setValue(1.0);
        CPPUNIT_ASSERT(volumeSlider.isInRange(volumeSlider.getValue()));

        // Pitch: 0.0 to 2.0
        MockSlider pitchSlider(0.0, 2.0, 1.0);
        pitchSlider.setValue(3.5);
        CPPUNIT_ASSERT(pitchSlider.getValue() == 2.0);
        pitchSlider.setValue(-0.5);
        CPPUNIT_ASSERT(pitchSlider.getValue() == 0.0);

        // Length: 0.0 to 10.0
        MockSlider lengthSlider(0.0, 10.0, 0.0);
        lengthSlider.setValue(11.0);
        CPPUNIT_ASSERT(lengthSlider.getValue() == 10.0);
        lengthSlider.setValue(-1.0);
        CPPUNIT_ASSERT(lengthSlider.getValue() == 0.0);
    }

    // ----------------------------------------------------------------
    // testGuestPermissions
    // Verifies that a Guest user cannot record or save, but can
    // play and stop. Owner should have access to all actions.
    // ----------------------------------------------------------------
    void testGuestPermissions()
    {
        // --- Guest role ---
        PermissionGuard guest("Guest");

        // Guest cannot record or save
        CPPUNIT_ASSERT(!guest.canRecord());
        CPPUNIT_ASSERT(!guest.canSave());

        // Guest CAN play and stop
        CPPUNIT_ASSERT(guest.canPlay());
        CPPUNIT_ASSERT(guest.canStop());

        // --- Owner role ---
        PermissionGuard owner("Owner");

        // Owner has full access
        CPPUNIT_ASSERT(owner.canRecord());
        CPPUNIT_ASSERT(owner.canSave());
        CPPUNIT_ASSERT(owner.canPlay());
        CPPUNIT_ASSERT(owner.canStop());

        // --- Role string check ---
        CPPUNIT_ASSERT(guest.getRole() == "Guest");
        CPPUNIT_ASSERT(owner.getRole() == "Owner");
    }
};