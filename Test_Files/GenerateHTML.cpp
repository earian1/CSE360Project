#include <map>
#include <fstream>
#include <string>
#include <iostream>

int main() {
    std::ifstream xml("cppunit_results.xml");
    if (!xml.is_open()) {
        std::cerr << "Cannot open cppunit_results.xml\n";
        return 1;
    }

    std::ofstream html("TestReport.html");
    html << "<html><head><title>CppUnit Test Report</title></head><body>\n";
    html << "<h1>CppUnit Test Report</h1>\n";

    // --- Test descriptions for all 8 tests ---
    std::map<std::string, std::string> descriptions;

    // Sprint 1 & 2
    descriptions["UserManagementTest::testUserLogin"] =
        "Verifies that valid credentials allow login and invalid credentials are rejected";
    descriptions["UserManagementTest::testAccountInfo"] =
        "Verifies that account info is returned for the correct user and empty for an unknown user";
    descriptions["UserManagementTest::testRecordingStatus"] =
        "Verifies that the recording flag toggles correctly on startRecording() and stopRecording()";

    // Sprint 3
    descriptions["UserManagementTest::testPasswordValidation"] =
        "Verifies password requires at least one uppercase letter, one number, and one special character; also checks case-sensitivity and empty inputs are rejected";
    descriptions["UserManagementTest::testSaveRecording"] =
        "Verifies that saveRecording() writes a .wav file to disk and the file exists afterward";
    descriptions["UserManagementTest::testPlaybackState"] =
        "Verifies the play/pause/resume/stop state machine and that play is blocked when no audio is loaded";
    descriptions["UserManagementTest::testVolumeSliderRange"] =
        "Verifies that volume, pitch, and length sliders clamp values within their defined min/max bounds";
    descriptions["UserManagementTest::testGuestPermissions"] =
        "Verifies that Guest users cannot record or save, while Owner users have full access";

    int totalTests = 0;
    int totalFailures = 0;
    int totalErrors = 0;

    html << "<h2>Tests</h2>\n<ul>\n";

    std::string line;
    bool inFailedTests = false;

    while (std::getline(xml, line)) {
        // Track whether we are inside <FailedTests> to colour failures red
        if (line.find("<FailedTests>") != std::string::npos)  inFailedTests = true;
        if (line.find("</FailedTests>") != std::string::npos) inFailedTests = false;

        if (line.find("<Name>") != std::string::npos) {
            totalTests++;
            size_t start = line.find("<Name>") + 6;
            size_t end   = line.find("</Name>");
            std::string testName = line.substr(start, end - start);

            std::string desc = descriptions.count(testName) ? descriptions[testName] : "No description";
            std::string status = inFailedTests
                ? "<span style='color:red'>FAIL</span>"
                : "<span style='color:green'>PASS</span>";

            html << "<li><strong>" << testName << "</strong> &ndash; "
                 << desc << " &ndash; " << status << "</li>\n";
        }

        // Pick up failure/error counts from the XML statistics block
        if (line.find("<FailuresTotal>") != std::string::npos) {
            size_t s = line.find("<FailuresTotal>") + 15;
            size_t e = line.find("</FailuresTotal>");
            totalFailures = std::stoi(line.substr(s, e - s));
        }
        if (line.find("<Errors>") != std::string::npos) {
            size_t s = line.find("<Errors>") + 8;
            size_t e = line.find("</Errors>");
            totalErrors = std::stoi(line.substr(s, e - s));
        }
    }

    html << "</ul>\n";

    // Summary section
    html << "<h2>Summary</h2>\n<ul>\n";
    html << "<li>Total Tests: " << totalTests << "</li>\n";
    html << "<li>Failures: "    << totalFailures << "</li>\n";
    html << "<li>Errors: "      << totalErrors   << "</li>\n";
    html << "</ul>\n";

    html << "</body></html>\n";

    std::cout << "HTML report generated: TestReport.html\n";
    return 0;
}