#include "core/device_manager.h"
#include "core/adb_manager.h"
#include "utils/logger.h"
#include <sstream>
#include <algorithm>
#include <chrono>

DeviceManager::~DeviceManager() {
    stopPolling();
}

DeviceManager& DeviceManager::instance() {
    static DeviceManager s_instance;
    return s_instance;
}

void DeviceManager::startPolling(const std::string& adbPath, int intervalMs) {
    m_adbPath = adbPath;
    m_intervalMs = intervalMs;
    AdbManager::instance().setAdbPath(adbPath);

    // Start adb server first
    AdbManager::instance().startServer();

    if (!m_running) {
        m_running = true;
        m_pollThread = std::thread(&DeviceManager::pollThreadFunc, this);
        LOG_INFO("Device polling started (interval: %dms)", intervalMs);
    }
}

void DeviceManager::stopPolling() {
    m_running = false;
    if (m_pollThread.joinable()) {
        m_pollThread.join();
    }
}

void DeviceManager::pollThreadFunc() {
    while (m_running) {
        refreshDeviceList();
        std::this_thread::sleep_for(std::chrono::milliseconds(m_intervalMs));
    }
}

void DeviceManager::refreshDeviceList() {
    std::string output = AdbManager::instance().getDevices();

    std::vector<Device> newDevices;
    std::istringstream iss(output);
    std::string line;

    // Skip first line ("List of devices attached")
    std::getline(iss, line);

    while (std::getline(iss, line)) {
        if (line.empty()) continue;

        Device dev;

        // Parse: SERIAL  STATE  [key:value ...]
        std::istringstream lineStream(line);
        lineStream >> dev.serial >> dev.state;

        // Parse extra fields (model:, device:, transport_id:)
        std::string extra;
        while (lineStream >> extra) {
            if (extra.find("model:") == 0) {
                dev.model = extra.substr(6);
            }
        }

        // Detect USB vs WLAN: WLAN serials contain ':' (e.g., 192.168.1.100:5555)
        dev.isUsb = (dev.serial.find(':') == std::string::npos);

        // Extract IP for WLAN devices
        if (!dev.isUsb) {
            size_t colonPos = dev.serial.find(':');
            if (colonPos != std::string::npos) {
                dev.ip = dev.serial.substr(0, colonPos);
            }
        }

        if (!dev.serial.empty()) {
            newDevices.push_back(dev);
        }
    }

    // Check for changes
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (newDevices.size() != m_devices.size()) {
            changed = true;
        } else {
            for (size_t i = 0; i < newDevices.size(); ++i) {
                if (newDevices[i].serial != m_devices[i].serial ||
                    newDevices[i].state != m_devices[i].state) {
                    changed = true;
                    break;
                }
            }
        }

        if (changed) {
            m_devices = std::move(newDevices);

            // Auto-select first device if none selected or selection is gone
            ensureValidSelectionLocked();
        }
    }

    if (changed) {
        if (m_changeCallback) {
            m_changeCallback();
        }
        LOG_INFO("Device list updated: %zu device(s)", newDevices.size());
    }
}

DeviceManager::Device DeviceManager::getSelectedDevice() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& dev : m_devices) {
        if (dev.serial == m_selectedSerial) {
            return dev;
        }
    }
    // Return first device if selection not found
    if (!m_devices.empty()) {
        return m_devices[0];
    }
    return {};
}

void DeviceManager::ensureValidSelection() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ensureValidSelectionLocked();
}

void DeviceManager::ensureValidSelectionLocked() {
    // Caller must hold m_mutex
    if (m_devices.empty()) {
        m_selectedSerial.clear();
        return;
    }

    // Check if current selection is still valid
    bool found = false;
    for (const auto& dev : m_devices) {
        if (dev.serial == m_selectedSerial && dev.state == "device") {
            found = true;
            break;
        }
    }

    // Auto-select first online device if selection is invalid
    if (!found) {
        for (const auto& dev : m_devices) {
            if (dev.state == "device") {
                m_selectedSerial = dev.serial;
                LOG_INFO("Auto-selected device: %s", dev.serial.c_str());
                return;
            }
        }
        // No online device, select first anyway
        m_selectedSerial = m_devices[0].serial;
    }
}

std::vector<DeviceManager::Device> DeviceManager::getDevices() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_devices;
}

size_t DeviceManager::getDeviceCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_devices.size();
}
