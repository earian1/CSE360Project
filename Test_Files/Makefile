CXX = g++
CXXFLAGS = -std=c++17 -I/usr/include/cppunit
LDFLAGS = -lcppunit -pthread

TARGET = UnitTests
SRC = main.cpp

# All headers the test suite depends on
HEADERS = UserManagementTest.h \
          MainComponentLogin.h \
          RecordingManager.h \
          PasswordValidator.h \
          SaveManager.h \
          PlaybackManager.h \
          MockSliderAndPermission.h

# Target for the test runner
all: $(TARGET) GenerateHTML

$(TARGET): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

# Target for HTML generator
GenerateHTML: GenerateHTML.cpp
	$(CXX) $(CXXFLAGS) GenerateHTML.cpp -o GenerateHTML

# Run tests and generate HTML report
report: all
	./$(TARGET)
	./GenerateHTML

clean:
	rm -f $(TARGET) GenerateHTML *.o cppunit_results.xml TestReport.html test_output.wav