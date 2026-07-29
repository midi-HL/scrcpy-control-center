#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>

// Manages Android apps via ADB: list, freeze, unfreeze, launch.
// Data is obtained ONLY through `adb shell pm list packages` (no APK parsing).
class AppManager {
public:
    struct AppInfo {
        std::string packageName;
        std::string label;        // human-readable name (may be empty initially)
        bool enabled = true;      // false = frozen/disabled
        bool isSystem = false;    // system app vs third-party
    };

    using AppListCallback = std::function<void()>;

    static AppManager& instance();

    // Refresh app list from device (blocking, runs in background)
    void refreshAppList(const std::string& serial, bool systemApps = false);

    // Get cached app list (thread-safe)
    std::vector<AppInfo> getApps() const;

    // Filter apps by state
    std::vector<AppInfo> getEnabledApps() const;
    std::vector<AppInfo> getDisabledApps() const;

    // App control operations
    bool freezeApp(const std::string& serial, const std::string& packageName);
    bool unfreezeApp(const std::string& serial, const std::string& packageName);
    bool uninstallApp(const std::string& serial, const std::string& packageName, bool keepData = true);
    bool launchApp(const std::string& serial, const std::string& packageName);

    // Launch app on virtual display
    bool launchAppOnVirtualDisplay(const std::string& serial,
                                   const std::string& packageName,
                                   const std::string& displaySize = "1920x1080");

    // Check if a package is frozen
    bool isAppFrozen(const std::string& packageName) const;

    // Register callback for when app list is updated
    void setCallback(AppListCallback cb) { m_callback = std::move(cb); }

    // Is refresh in progress?
    bool isRefreshing() const { return m_refreshing; }

private:
    AppManager() = default;
    ~AppManager() = default;
    AppManager(const AppManager&) = delete;
    AppManager& operator=(const AppManager&) = delete;

    mutable std::mutex m_mutex;
    std::vector<AppInfo> m_apps;
    AppListCallback m_callback;
    std::atomic<bool> m_refreshing{false};
};
