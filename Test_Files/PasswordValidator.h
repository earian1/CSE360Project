#pragma once
#include <string>
#include <cctype>

// Mock password validator — mirrors the login check logic in MainComponent
// without requiring JUCE.
//
// Password rules:
//   - Must not be empty
//   - Must contain at least one uppercase letter
//   - Must contain at least one number
//   - Must contain at least one special character (!@#$%^&*...)
class PasswordValidator
{
public:
    void setCredentials(const std::string& username, const std::string& password)
    {
        savedUsername = username;
        savedPassword = password;
    }

    // Returns true only if credentials match AND the password meets all rules
    bool validate(const std::string& username, const std::string& password) const
    {
        if (username.empty() || password.empty())
            return false;

        if (!meetsRequirements(password))
            return false;

        return username == savedUsername && password == savedPassword;
    }

    // Check password strength rules independently
    // (useful for the registration/setup screen)
    bool meetsRequirements(const std::string& password) const
    {
        if (password.empty()) return false;

        bool hasUpper   = false;
        bool hasDigit   = false;
        bool hasSpecial = false;

        for (char c : password)
        {
            if (std::isupper((unsigned char)c)) hasUpper   = true;
            if (std::isdigit((unsigned char)c)) hasDigit   = true;
            if (isSpecialChar(c))               hasSpecial = true;
        }

        return hasUpper && hasDigit && hasSpecial;
    }

private:
    std::string savedUsername;
    std::string savedPassword;

    bool isSpecialChar(char c) const
    {
        std::string specials = "!@#$%^&*()-_=+[]{}|;:',.<>?/`~";
        return specials.find(c) != std::string::npos;
    }
};