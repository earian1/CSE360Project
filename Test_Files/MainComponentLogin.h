#pragma once
#include <string>

class MainComponentLogin
{
public:
    // Save all user info
    void saveUserInfo(const std::string& username,
                      const std::string& password,
                      const std::string& accountInfo)
    {
        savedUsername = username;
        savedPassword = password;
        savedAccountInfo = accountInfo;
    }

    // Check login (username + password)
    bool checkLogin(const std::string& username, const std::string& password) const
    {
        return username == savedUsername && password == savedPassword;
    }

    // Retrieve account info
    std::string getAccountInfo(const std::string& username) const
    {
        if (username == savedUsername)
            return savedAccountInfo;
        return "";
    }

private:
    std::string savedUsername;
    std::string savedPassword;
    std::string savedAccountInfo;
};
