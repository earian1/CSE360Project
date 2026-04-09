#pragma once
#include <string>
#include <algorithm>
#include <vector>

class MockSlider
{
public:
    MockSlider(double min, double max, double initial)
        : minValue(min), maxValue(max), currentValue(initial) {}

    void setValue(double v)
    {
        currentValue = std::max(minValue, std::min(maxValue, v));
    }

    double getValue() const { return currentValue; }
    double getMinimum() const { return minValue; }
    double getMaximum() const { return maxValue; }
    bool isInRange(double v) const { return v >= minValue && v <= maxValue; }

private:
    double minValue;
    double maxValue;
    double currentValue;
};

class PermissionGuard
{
public:
    PermissionGuard(const std::string& role) : userRole(role) {}

    bool canRecord()      const { return userRole == "Owner"; }
    bool canSave()        const { return userRole == "Owner"; }
    bool canDelete()      const { return userRole == "Owner"; }
    bool canCreateGuest() const { return userRole == "Owner"; }
    bool canDownload()    const { return userRole == "Guest" || userRole == "Owner"; }
    bool canPlay()        const { return userRole == "Owner" || userRole == "Guest"; }
    bool canStop()        const { return userRole == "Owner" || userRole == "Guest"; }

    std::string getRole() const { return userRole; }

private:
    std::string userRole;
};

class MockSoundList
{
public:
    MockSoundList() : selectedRow(-1)
    {
        sounds.push_back("TestSound");
    }

    void selectRow(int row)
    {
        if (row >= 0 && row < (int)sounds.size())
            selectedRow = row;
    }

    void deselectAll() { selectedRow = -1; }

    void addSound(const std::string& name)
    {
        sounds.push_back(name);
    }

    void deleteSound(int row)
    {
        if (row >= 0 && row < (int)sounds.size())
        {
            sounds.erase(sounds.begin() + row);
            selectedRow = -1;
        }
    }

    void clearAll()
    {
        sounds.clear();
        selectedRow = -1;
    }

    bool soundExists(const std::string& name) const
    {
        return std::find(sounds.begin(), sounds.end(), name) != sounds.end();
    }

    bool hasSelection() const
    {
        return selectedRow >= 0 && selectedRow < (int)sounds.size();
    }

    bool isEmpty() const { return sounds.empty(); }

    bool canDownload(const PermissionGuard& user) const
    {
        return user.canDownload() && hasSelection();
    }

    int size() const { return (int)sounds.size(); }

private:
    std::vector<std::string> sounds;
    int selectedRow;
};