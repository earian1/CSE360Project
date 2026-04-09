#pragma once
#include <cppunit/extensions/HelperMacros.h>
#include <cstdio>  // for std::remove()
#include <cmath>    // for std::pow()

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
    CPPUNIT_TEST(testGuestPermissions);
    CPPUNIT_TEST(testGuestCannotDelete);
    CPPUNIT_TEST(testGuestDownloadRequiresSelection);
    CPPUNIT_TEST(testGuestCannotCreateGuest);
    CPPUNIT_TEST(testMaxRecordingDuration);
    CPPUNIT_TEST(testDeleteSound);
    CPPUNIT_TEST(testSoundListEmpty);
    CPPUNIT_TEST(testPitchAffectsPlaybackRate);
    CPPUNIT_TEST(testVolumeZero);
    CPPUNIT_TEST(testLogout);
    CPPUNIT_TEST(testRecordingOverwrite);
    CPPUNIT_TEST(testSoundListSize);
    CPPUNIT_TEST(testSessionPersistence);

    CPPUNIT_TEST_SUITE_END();

private:
    // Sprint 1 & 2 mocks
    MainComponentLogin* mainComp       = new MainComponentLogin();
    RecordingManager*   recordingManager = new RecordingManager();

    // Sprint 3 mocks
    PasswordValidator*  passwordValidator = new PasswordValidator();
    SaveManager*        saveManager       = new SaveManager();
    PlaybackManager*    playbackManager   = new PlaybackManager();

    private:
    // helper to calculate playback rate from pitch slider value
    double calculatePlaybackRate(double sliderVal)
    {
        double minRate = 0.5;
        double maxRate = 2.0;
        double normalized = sliderVal / 100.0;
        return minRate * std::pow(maxRate / minRate, normalized);
    }

    // helper to calculate gain from volume slider value
    double calculateGain(double sliderVal)
    {
        if (sliderVal <= 0.0) return 0.0;
        double dB = -60.0 + (sliderVal / 100.0) * 60.0;
        return std::pow(10.0, dB / 20.0);
    }

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

     void testGuestCannotDelete()
    {
        PermissionGuard guest("Guest");
        PermissionGuard owner("Owner");

        CPPUNIT_ASSERT(!guest.canDelete());
        CPPUNIT_ASSERT(owner.canDelete());
    }

    // ----------------------------------------------------------------
    // testGuestDownloadRequiresSelection
    // ----------------------------------------------------------------
    void testGuestDownloadRequiresSelection()
    {
        PermissionGuard guest("Guest");

        CPPUNIT_ASSERT(guest.canDownload());

        MockSoundList soundList;
        CPPUNIT_ASSERT(!soundList.hasSelection());
        CPPUNIT_ASSERT(!soundList.canDownload(guest));

        soundList.selectRow(0);
        CPPUNIT_ASSERT(soundList.hasSelection());
        CPPUNIT_ASSERT(soundList.canDownload(guest));
    }

    // ----------------------------------------------------------------
    // testGuestCannotCreateGuest
    // ----------------------------------------------------------------
    void testGuestCannotCreateGuest()
    {
        PermissionGuard guest("Guest");
        PermissionGuard owner("Owner");

        CPPUNIT_ASSERT(!guest.canCreateGuest());
        CPPUNIT_ASSERT(owner.canCreateGuest());
    }

    // ----------------------------------------------------------------
// testMaxRecordingDuration
// Verifies that recording stops automatically after max samples
// are reached (simulating 10 second limit).
// ----------------------------------------------------------------
void testMaxRecordingDuration()
{
    recordingManager->startRecording();
    CPPUNIT_ASSERT(recordingManager->getIsRecording());

    // Simulate reaching max duration
    recordingManager->simulateMaxDuration();

    // Should have stopped automatically
    CPPUNIT_ASSERT(!recordingManager->getIsRecording());
    CPPUNIT_ASSERT(recordingManager->hasReachedMaxDuration());
}

// ----------------------------------------------------------------
// testDeleteSound
// Verifies that after deleting a sound it no longer appears
// in the list.
// ----------------------------------------------------------------
void testDeleteSound()
{
    MockSoundList soundList;

    // Should start with 1 sound
    CPPUNIT_ASSERT(soundList.size() == 1);

    // Add another sound
    soundList.addSound("NewSound");
    CPPUNIT_ASSERT(soundList.size() == 2);

    // Delete first sound
    soundList.deleteSound(0);
    CPPUNIT_ASSERT(soundList.size() == 1);

    // Deleted sound should not exist
    CPPUNIT_ASSERT(!soundList.soundExists("TestSound"));

    // Remaining sound should still exist
    CPPUNIT_ASSERT(soundList.soundExists("NewSound"));
}

// ----------------------------------------------------------------
// testSoundListEmpty
// Verifies that clearing the list removes all entries.
// ----------------------------------------------------------------
void testSoundListEmpty()
{
    MockSoundList soundList;

    // Add a few sounds
    soundList.addSound("Sound1");
    soundList.addSound("Sound2");
    CPPUNIT_ASSERT(soundList.size() == 3);

    // Clear the list
    soundList.clearAll();
    CPPUNIT_ASSERT(soundList.size() == 0);
    CPPUNIT_ASSERT(soundList.isEmpty());
}

// ----------------------------------------------------------------
// testPitchAffectsPlaybackRate
// Verifies that pitch slider value correctly maps to playback rate.
// ----------------------------------------------------------------
void testPitchAffectsPlaybackRate()
{
    MockSlider pitchSlider(0.0, 100.0, 50.0);

    // At 50 (neutral) rate should be 1.0
    double rate = calculatePlaybackRate(pitchSlider.getValue());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, rate, 0.01);

    // At 0 (lowest) rate should be 0.5 (one octave down)
    pitchSlider.setValue(0.0);
    rate = calculatePlaybackRate(pitchSlider.getValue());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, rate, 0.01);

    // At 100 (highest) rate should be 2.0 (one octave up)
    pitchSlider.setValue(100.0);
    rate = calculatePlaybackRate(pitchSlider.getValue());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, rate, 0.01);
}

// ----------------------------------------------------------------
// testVolumeZero
// Verifies that setting volume to 0 produces silence (gain of 0).
// ----------------------------------------------------------------
void testVolumeZero()
{
    MockSlider volumeSlider(0.0, 100.0, 80.0);

    // At 0 volume gain should be effectively 0 (silence)
    volumeSlider.setValue(0.0);
    double gain = calculateGain(volumeSlider.getValue());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, gain, 0.0001);

    // At 100 volume gain should be 1.0 (full)
    volumeSlider.setValue(100.0);
    gain = calculateGain(volumeSlider.getValue());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, gain, 0.01);

    // At 50 gain should be between 0 and 1
    volumeSlider.setValue(50.0);
    gain = calculateGain(volumeSlider.getValue());
    CPPUNIT_ASSERT(gain > 0.0 && gain < 1.0);
}

// ----------------------------------------------------------------
// testLogout
// Verifies that after logout, owner functions are blocked.
// ----------------------------------------------------------------
void testLogout()
{
    // Start logged in as owner
    PermissionGuard owner("Owner");
    CPPUNIT_ASSERT(owner.canRecord());
    CPPUNIT_ASSERT(owner.canSave());

    // Simulate logout by switching to no role
    PermissionGuard loggedOut("None");
    CPPUNIT_ASSERT(!loggedOut.canRecord());
    CPPUNIT_ASSERT(!loggedOut.canSave());
    CPPUNIT_ASSERT(!loggedOut.canPlay());
    CPPUNIT_ASSERT(!loggedOut.canStop());
    CPPUNIT_ASSERT(!loggedOut.canDownload());
    CPPUNIT_ASSERT(!loggedOut.canDelete());
    CPPUNIT_ASSERT(!loggedOut.canCreateGuest());
}

// ----------------------------------------------------------------
// testRecordingOverwrite
// Verifies that starting a new recording clears the previous one.
// ----------------------------------------------------------------
void testRecordingOverwrite()
{
    // Start first recording
    recordingManager->startRecording();
    CPPUNIT_ASSERT(recordingManager->getIsRecording());

    // Stop first recording
    recordingManager->stopRecording();
    CPPUNIT_ASSERT(!recordingManager->getIsRecording());

    // Record some samples
    recordingManager->addSamples(100);
    CPPUNIT_ASSERT(recordingManager->getSampleCount() == 100);

    // Start a new recording — should clear previous samples
    recordingManager->startRecording();
    CPPUNIT_ASSERT(recordingManager->getSampleCount() == 0);

    recordingManager->stopRecording();
}

// ----------------------------------------------------------------
// testSoundListSize
// Verifies that adding and removing sounds changes list size correctly.
// ----------------------------------------------------------------
void testSoundListSize()
{
    MockSoundList soundList;

    // Starts with 1 sound
    CPPUNIT_ASSERT(soundList.size() == 1);

    // Add sounds one by one and check size
    soundList.addSound("Sound1");
    CPPUNIT_ASSERT(soundList.size() == 2);

    soundList.addSound("Sound2");
    CPPUNIT_ASSERT(soundList.size() == 3);

    soundList.addSound("Sound3");
    CPPUNIT_ASSERT(soundList.size() == 4);

    // Delete one and check size decreases
    soundList.deleteSound(0);
    CPPUNIT_ASSERT(soundList.size() == 3);

    // Clear all and check size is 0
    soundList.clearAll();
    CPPUNIT_ASSERT(soundList.size() == 0);
    CPPUNIT_ASSERT(soundList.isEmpty());
}

// ----------------------------------------------------------------
// testSessionPersistence
// Verifies that a logged in user stays logged in until
// logout is explicitly called.
// ----------------------------------------------------------------
void testSessionPersistence()
{
    // Log in
    CPPUNIT_ASSERT(mainComp->checkLogin("Johnny", "Jgarciga1!"));
    mainComp->setLoggedIn(true);

    // Session should be active
    CPPUNIT_ASSERT(mainComp->isLoggedIn());

    // Simulate some actions — session should persist
    mainComp->isLoggedIn();
    mainComp->isLoggedIn();
    mainComp->isLoggedIn();
    CPPUNIT_ASSERT(mainComp->isLoggedIn());

    // Logout — session should end
    mainComp->setLoggedIn(false);
    CPPUNIT_ASSERT(!mainComp->isLoggedIn());
}
};