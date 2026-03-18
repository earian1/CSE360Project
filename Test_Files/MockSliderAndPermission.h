#pragma
#include <string>
#include <algorithm>

class MockSlider
{
  public:
    MockSlider(double min, double max, double initial)
        : minValue(min), maxValue(max), currentValue(initial) {}

    void setValue(double v)
    {
        currentValue = std::max(minValue, std::min(maxValue, v));
    }

    double getValue() const
    {
        return currentValue;
    }

    double getMinimum() const
    {
        return minValue;
    }

    double getMaximum() const
    {
        return maxValue;
    }

    bool isInRange(double v) const
    {
        return v >= minValue && v <= maxValue;
    }


  private:
    double minValue;
    double maxValue;
    double currentValue;
};

class PermissionGuard
{
  public:
    PermissionGuard(const std::string& role)
        : userRole(role) {}

    bool canRecord() const
    {
        return userRole == "Owner";
    }

    bool canSave() const
    {
        return userRole == "Owner";
    }

    bool canPlay() const
    {
        return true;
    }

    bool canStop() const
    {
        return true;
    }

    std::string getRole() const
    {
        return userRole;
    }

    private:
        std::string userRole;
};