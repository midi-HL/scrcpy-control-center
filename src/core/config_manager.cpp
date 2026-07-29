#include "core/config_manager.h"
#include "core/state_manager.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <direct.h>
#include <windows.h>

using json = nlohmann::json;

ConfigManager& ConfigManager::instance() {
    static ConfigManager s_instance;
    return s_instance;
}

void ConfigManager::ensureDirectoryExists(const std::string& path) {
    _mkdir(path.c_str());
}

void ConfigManager::setDefaultPathsToAppDir() {
    // Get the directory where the executable is located
    char exePath[MAX_PATH] = "";
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);

    // Extract directory
    std::string exeDir(exePath);
    size_t lastSlash = exeDir.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        exeDir = exeDir.substr(0, lastSlash);
    }

    // Set default paths to exe directory
    // Only override if still at default values
    if (m_settings.scrcpyPath == "scrcpy") {
        m_settings.scrcpyPath = exeDir + "\\scrcpy.exe";
    }
    if (m_settings.adbPath == "adb") {
        m_settings.adbPath = exeDir + "\\adb.exe";
    }

    LOG_INFO("Default paths set to app directory: %s", exeDir.c_str());
}

bool ConfigManager::init(const std::string& configDir) {
    m_configDir = configDir;
    ensureDirectoryExists(m_configDir);
    ensureDirectoryExists(m_configDir + "/profiles");

    // Load settings with fault tolerance
    if (!loadSettings()) {
        LOG_WARN("Settings file not found or corrupted, using defaults");
        setDefaultPathsToAppDir();
        saveSettings();
    } else {
        // If settings loaded but paths are still default, try app dir
        setDefaultPathsToAppDir();
    }

    return true;
}

bool ConfigManager::loadSettings() {
    std::string path = m_configDir + "/settings.json";
    std::ifstream file(path);
    if (!file.is_open()) return false;

    try {
        json root = json::parse(file);
        m_settings.language    = root.value("language", "en");
        m_settings.theme       = root.value("theme", "dark");
        m_settings.scrcpyPath  = root.value("scrcpy_path", "scrcpy");
        m_settings.adbPath     = root.value("adb_path", "adb");
        m_settings.fetchAppIcons = root.value("fetch_app_icons", true);
        m_settings.devicePollingInterval = root.value("device_polling_interval", 20);
        LOG_INFO("Settings loaded from %s", path.c_str());
        return true;
    } catch (const json::exception& e) {
        LOG_ERROR("Failed to parse settings.json: %s", e.what());
        return false;
    }
}

bool ConfigManager::saveSettings() {
    std::string path = m_configDir + "/settings.json";
    json root = {
        {"language",                 m_settings.language},
        {"theme",                    m_settings.theme},
        {"scrcpy_path",              m_settings.scrcpyPath},
        {"adb_path",                 m_settings.adbPath},
        {"fetch_app_icons",          m_settings.fetchAppIcons},
        {"device_polling_interval",  m_settings.devicePollingInterval}
    };

    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to write settings.json");
        return false;
    }
    file << root.dump(2);
    LOG_INFO("Settings saved to %s", path.c_str());
    return true;
}

std::string ConfigManager::getProfilePath(const std::string& name) const {
    return m_configDir + "/profiles/" + name + ".json";
}

bool ConfigManager::loadProfile(const std::string& name) {
    std::string path = getProfilePath(name);
    return StateManager::instance().loadFromJson(path);
}

bool ConfigManager::saveProfile(const std::string& name) {
    std::string path = getProfilePath(name);
    return StateManager::instance().saveToJson(path);
}
