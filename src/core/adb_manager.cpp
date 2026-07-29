#include "core/adb_manager.h"
#include "core/process_manager.h"
#include "utils/logger.h"
#include <sstream>

AdbManager& AdbManager::instance() {
    static AdbManager s_instance;
    return s_instance;
}

std::string AdbManager::execute(const std::vector<std::string>& args, int timeoutMs) {
    return ProcessManager::instance().executeAdb(m_adbPath, args, timeoutMs);
}

std::string AdbManager::getDevices() {
    return execute({"devices", "-l"});
}

std::string AdbManager::shell(const std::string& cmd) {
    return execute({"shell", cmd});
}

std::string AdbManager::getDeviceModel(const std::string& serial) {
    std::string result = execute({"-s", serial, "shell", "getprop", "ro.product.model"});
    // Trim whitespace
    size_t start = result.find_first_not_of(" \t\r\n");
    size_t end = result.find_last_not_of(" \t\r\n");
    if (start != std::string::npos && end != std::string::npos) {
        return result.substr(start, end - start + 1);
    }
    return "Unknown";
}

std::string AdbManager::getAndroidVersion(const std::string& serial) {
    std::string result = execute({"-s", serial, "shell", "getprop", "ro.build.version.release"});
    size_t start = result.find_first_not_of(" \t\r\n");
    size_t end = result.find_last_not_of(" \t\r\n");
    if (start != std::string::npos && end != std::string::npos) {
        return result.substr(start, end - start + 1);
    }
    return "?";
}

std::string AdbManager::getScreenResolution(const std::string& serial) {
    std::string result = execute({"-s", serial, "shell", "wm", "size"});
    // Output: "Physical size: 1080x2400"
    size_t pos = result.find(": ");
    if (pos != std::string::npos) {
        std::string size = result.substr(pos + 2);
        size_t end = size.find_first_of("\r\n");
        if (end != std::string::npos) size = size.substr(0, end);
        return size;
    }
    return "Unknown";
}

bool AdbManager::enableTcpip(const std::string& serial, int port) {
    std::string result = execute({"-s", serial, "tcpip", std::to_string(port)});
    bool ok = result.find("restarting") != std::string::npos ||
              result.find("already") != std::string::npos;
    if (ok) {
        LOG_INFO("TCP/IP mode enabled on %s:%d", serial.c_str(), port);
    } else {
        LOG_ERROR("Failed to enable TCP/IP on %s: %s", serial.c_str(), result.c_str());
    }
    return ok;
}

bool AdbManager::connect(const std::string& ip, int port) {
    std::string addr = ip + ":" + std::to_string(port);
    std::string result = execute({"connect", addr});
    bool ok = result.find("connected") != std::string::npos;
    if (ok) {
        LOG_INFO("Connected to %s", addr.c_str());
    } else {
        LOG_ERROR("Failed to connect to %s: %s", addr.c_str(), result.c_str());
    }
    return ok;
}

bool AdbManager::startServer() {
    std::string result = execute({"start-server"});
    bool ok = result.find("daemon started") != std::string::npos ||
              result.find("already running") != std::string::npos;
    if (ok) {
        LOG_INFO("ADB server started");
    } else {
        LOG_WARN("ADB server start result: %s", result.c_str());
    }
    return ok;
}
