#pragma once
#include <string>

// Manages application configuration persistence.
// Handles settings.json, devices.json, and profile files.
// All configs use JSON format with fault-tolerant parsing.
class ConfigManager {
public:
    static ConfigManager& instance();

    // Initialize config directory structure
    bool init(const std::string& configDir = "config");

    // Settings (global)
    struct Settings {
        std::string language = "en";
        std::string theme = "dark";
        std::string scrcpyPath = "scrcpy";  // will be set to app dir on init
        std::string adbPath = "adb";        // will be set to app dir on init
        bool fetchAppIcons = true;          // toggle icon extraction from APK
        int devicePollingInterval = 20;     // seconds between device polls
    };

    Settings& getSettings() { return m_settings; }
    const Settings& getSettings() const { return m_settings; }
    bool loadSettings();
    bool saveSettings();

    // Set default paths to the application's own directory
    void setDefaultPathsToAppDir();

    // Profile management
    bool loadProfile(const std::string& name);
    bool saveProfile(const std::string& name);
    std::string getProfilePath(const std::string& name) const;

    // Config directory path
    const std::string& getConfigDir() const { return m_configDir; }

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::string m_configDir = "config";
    Settings m_settings;

    void ensureDirectoryExists(const std::string& path);
};
