#pragma once
#include <cppunit/extensions/HelperMacros.h>
#include "MainComponentLogin.h"
#include "RecordingManager.h"

class UserManagementTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(UserManagementTest);
    CPPUNIT_TEST(testUserLogin);
    CPPUNIT_TEST(testAccountInfo);
    CPPUNIT_TEST(testRecordingStatus);
    CPPUNIT_TEST_SUITE_END();

private:
    MainComponentLogin* mainComp = new MainComponentLogin();
    RecordingManager* recordingManager = new RecordingManager();

public:

    void setUp() override
    {
        mainComp->saveUserInfo("Johnny", "jgarciga", "IDK");
    }

    void tearDown() override
    {
        // Cleanup can be used to reset any state after each test
    }

    void testUserLogin()
    {
       
        // Correct login
        CPPUNIT_ASSERT(mainComp->checkLogin("Johnny", "jgarciga"));
        // Incorrect login
        CPPUNIT_ASSERT(!mainComp->checkLogin("johnny", "wrongpass"));
    }

    void testAccountInfo()
    {   
       
        // Account info retrieval
        CPPUNIT_ASSERT(mainComp->getAccountInfo("Johnny") == "IDK");

        // Wrong username returns empty
        CPPUNIT_ASSERT(mainComp->getAccountInfo("johnny") == "");
    }

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
};
