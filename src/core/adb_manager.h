#pragma once
#include <string>
#include <vector>

// Thin wrapper around adb.exe for common operations.
// All operations are async-friendly but currently blocking for simplicity.
class AdbManager {
public:
    static AdbManager& instance();

    void setAdbPath(const std::string& path) { m_adbPath = path; }
    const std::string& getAdbPath() const { return m_adbPath; }

    // Execute raw adb command
    std::string execute(const std::vector<std::string>& args, int timeoutMs = 5000);

    // Common operations
    std::string getDevices();        // adb devices -l
    std::string shell(const std::string& cmd); // adb shell <cmd>

    // Device info queries
    std::string getDeviceModel(const std::string& serial);
    std::string getAndroidVersion(const std::string& serial);
    std::string getScreenResolution(const std::string& serial);

    // TCP/IP mode
    bool enableTcpip(const std::string& serial, int port = 5555);
    bool connect(const std::string& ip, int port = 5555);

    // Check if adb server is running
    bool startServer();

private:
    AdbManager() = default;
    ~AdbManager() = default;
    AdbManager(const AdbManager&) = delete;
    AdbManager& operator=(const AdbManager&) = delete;

    std::string m_adbPath = "adb"; // default: assume in PATH
};
