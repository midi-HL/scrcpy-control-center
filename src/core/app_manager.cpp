#include "core/app_manager.h"
#include "core/adb_manager.h"
#include "core/process_manager.h"
#include "core/config_manager.h"
#include "utils/logger.h"
#include <sstream>
#include <algorithm>

AppManager& AppManager::instance() {
    static AppManager s_instance;
    return s_instance;
}

void AppManager::refreshAppList(const std::string& serial, bool systemApps) {
    if (serial.empty()) {
        LOG_WARN("Cannot refresh app list: no device selected");
        return;
    }

    m_refreshing = true;

    // Get enabled packages
    std::string enabledOutput = AdbManager::instance().execute(
        {"-s", serial, "shell", "pm", "list", "packages", "-e"}, 10000);

    // Get disabled (frozen) packages
    std::string disabledOutput = AdbManager::instance().execute(
        {"-s", serial, "shell", "pm", "list", "packages", "-d"}, 10000);

    std::vector<AppInfo> newApps;

    // Parse enabled packages: "package:com.example.app"
    std::istringstream iss(enabledOutput);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        size_t pos = line.find("package:");
        if (pos != std::string::npos) {
            AppInfo app;
            app.packageName = line.substr(pos + 8);
            // Trim whitespace
            while (!app.packageName.empty() && app.packageName.back() == '\r') {
                app.packageName.pop_back();
            }
            app.enabled = true;
            app.isSystem = (app.packageName.find("com.android.") == 0 ||
                           app.packageName.find("android.") == 0 ||
                           app.packageName.find("com.google.") == 0);
            newApps.push_back(app);
        }
    }

    // Parse disabled packages
    std::istringstream iss2(disabledOutput);
    while (std::getline(iss2, line)) {
        if (line.empty()) continue;
        size_t pos = line.find("package:");
        if (pos != std::string::npos) {
            AppInfo app;
            app.packageName = line.substr(pos + 8);
            while (!app.packageName.empty() && app.packageName.back() == '\r') {
                app.packageName.pop_back();
            }
            app.enabled = false;
            app.isSystem = (app.packageName.find("com.android.") == 0 ||
                           app.packageName.find("android.") == 0 ||
                           app.packageName.find("com.google.") == 0);
            newApps.push_back(app);
        }
    }

    // Sort: enabled first, then by package name
    std::sort(newApps.begin(), newApps.end(), [](const AppInfo& a, const AppInfo& b) {
        if (a.enabled != b.enabled) return a.enabled > b.enabled;
        return a.packageName < b.packageName;
    });

    size_t count = newApps.size();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_apps = std::move(newApps);
    }

    m_refreshing = false;

    LOG_INFO("App list refreshed: %zu packages found", count);

    if (m_callback) {
        m_callback();
    }
}

std::vector<AppManager::AppInfo> AppManager::getApps() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_apps;
}

std::vector<AppManager::AppInfo> AppManager::getEnabledApps() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AppInfo> result;
    for (const auto& app : m_apps) {
        if (app.enabled) result.push_back(app);
    }
    return result;
}

std::vector<AppManager::AppInfo> AppManager::getDisabledApps() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AppInfo> result;
    for (const auto& app : m_apps) {
        if (!app.enabled) result.push_back(app);
    }
    return result;
}

bool AppManager::freezeApp(const std::string& serial, const std::string& packageName) {
    std::string result = AdbManager::instance().execute(
        {"-s", serial, "shell", "pm", "disable-user", "--user", "0", packageName});
    bool ok = result.find("disabled") != std::string::npos ||
              result.find("already") != std::string::npos;
    if (ok) {
        LOG_INFO("Frozen: %s", packageName.c_str());
        // Update local state
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& app : m_apps) {
            if (app.packageName == packageName) {
                app.enabled = false;
                break;
            }
        }
    } else {
        LOG_ERROR("Failed to freeze %s: %s", packageName.c_str(), result.c_str());
    }
    return ok;
}

bool AppManager::unfreezeApp(const std::string& serial, const std::string& packageName) {
    std::string result = AdbManager::instance().execute(
        {"-s", serial, "shell", "pm", "enable", packageName});
    bool ok = result.find("enabled") != std::string::npos;
    if (ok) {
        LOG_INFO("Unfrozen: %s", packageName.c_str());
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& app : m_apps) {
            if (app.packageName == packageName) {
                app.enabled = true;
                break;
            }
        }
    } else {
        LOG_ERROR("Failed to unfreeze %s: %s", packageName.c_str(), result.c_str());
    }
    return ok;
}

bool AppManager::uninstallApp(const std::string& serial, const std::string& packageName, bool keepData) {
    std::vector<std::string> args = {"-s", serial, "shell", "pm", "uninstall"};
    if (keepData) args.push_back("-k");
    args.push_back(packageName);

    std::string result = AdbManager::instance().execute(args);
    bool ok = result.find("Success") != std::string::npos;
    if (ok) {
        LOG_INFO("Uninstalled: %s (keep_data=%d)", packageName.c_str(), keepData);
        // Remove from local list
        std::lock_guard<std::mutex> lock(m_mutex);
        m_apps.erase(std::remove_if(m_apps.begin(), m_apps.end(),
            [&](const AppInfo& a) { return a.packageName == packageName; }), m_apps.end());
    } else {
        LOG_ERROR("Failed to uninstall %s: %s", packageName.c_str(), result.c_str());
    }
    return ok;
}

bool AppManager::launchApp(const std::string& serial, const std::string& packageName) {
    std::string result = AdbManager::instance().execute(
        {"-s", serial, "shell", "monkey", "-p", packageName,
         "-c", "android.intent.category.LAUNCHER", "1"});
    bool ok = result.find("Events injected") != std::string::npos;
    if (ok) {
        LOG_INFO("Launched: %s", packageName.c_str());
    } else {
        LOG_ERROR("Failed to launch %s: %s", packageName.c_str(), result.c_str());
    }
    return ok;
}

bool AppManager::launchAppOnVirtualDisplay(const std::string& serial,
                                            const std::string& packageName,
                                            const std::string& displaySize) {
    auto& cfg = ConfigManager::instance().getSettings();
    std::vector<std::string> args = {
        "-s", serial,
        "--new-display=" + displaySize,
        "--start-app=" + packageName,
        "--flex-display",
        "--no-vd-system-decorations"
    };

    bool ok = ProcessManager::instance().startScrcpy(cfg.scrcpyPath, args, serial);
    if (ok) {
        LOG_INFO("Launched %s on virtual display %s", packageName.c_str(), displaySize.c_str());
    } else {
        LOG_ERROR("Failed to launch %s on virtual display", packageName.c_str());
    }
    return ok;
}

bool AppManager::isAppFrozen(const std::string& packageName) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& app : m_apps) {
        if (app.packageName == packageName) {
            return !app.enabled;
        }
    }
    return false;
}
