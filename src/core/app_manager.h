#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>

// Manages Android apps via ADB: list, freeze, unfreeze, launch.
class AppManager {
public:
    struct AppInfo {
        std::string packageName;
        std::string label;
        bool enabled = true;
        bool isSystem = false;
    };

    using AppListCallback = std::function<void()>;

    static AppManager& instance();

    void refreshAppList(const std::string& serial, bool systemApps = false);
    std::vector<AppInfo> getApps() const;
    std::vector<AppInfo> getEnabledApps() const;
    std::vector<AppInfo> getDisabledApps() const;

    bool freezeApp(const std::string& serial, const std::string& packageName);
    bool unfreezeApp(const std::string& serial, const std::string& packageName);
    bool uninstallApp(const std::string& serial, const std::string& packageName, bool keepData = true);
    bool launchApp(const std::string& serial, const std::string& packageName);
    bool launchAppOnVirtualDisplay(const std::string& serial,
                                   const std::string& packageName,
                                   const std::string& displaySize = "1920x1080");

    bool isAppFrozen(const std::string& packageName) const;
    void setCallback(AppListCallback cb) { m_callback = std::move(cb); }
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
