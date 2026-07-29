#include "core/config_manager.h"
#include "core/state_manager.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <direct.h>

using json = nlohmann::json;

ConfigManager& ConfigManager::instance() {
    static ConfigManager s_instance;
    return s_instance;
}

void ConfigManager::ensureDirectoryExists(const std::string& path) {
    // Create directory recursively (simple single-level for now)
#ifdef _WIN32
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0755);
#endif
}

bool ConfigManager::init(const std::string& configDir) {
    m_configDir = configDir;
    ensureDirectoryExists(m_configDir);
    ensureDirectoryExists(m_configDir + "/profiles");

    // Load settings with fault tolerance
    if (!loadSettings()) {
        LOG_WARN("Settings file not found or corrupted, using defaults");
        saveSettings(); // write defaults
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
        {"language",    m_settings.language},
        {"theme",       m_settings.theme},
        {"scrcpy_path", m_settings.scrcpyPath},
        {"adb_path",    m_settings.adbPath}
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
