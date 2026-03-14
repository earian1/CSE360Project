#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/XmlOutputter.h>
#include <fstream>
#include "UserManagementTest.h"

int main()
{
    CppUnit::TextUi::TestRunner runner;

    // Add your test suite
    runner.addTest(UserManagementTest::suite());

    // Run tests
    bool wasSuccessful = runner.run("", false);

    // Generate XML report
    std::ofstream xmlFile("cppunit_results.xml");
    CppUnit::XmlOutputter xmlOutputter(&runner.result(), xmlFile);
    xmlOutputter.write();

    return wasSuccessful ? 0 : 1;
}