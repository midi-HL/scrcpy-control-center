#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

// Manages Android device discovery via periodic adb polling.
// Detects USB/WLAN connections and device state changes.
class DeviceManager {
public:
    struct Device {
        std::string serial;
        std::string model;
        std::string state;   // "device", "offline", "unauthorized"
        bool isUsb = true;
        std::string ip;
    };

    using DeviceChangeCallback = std::function<void()>;

    static DeviceManager& instance();

    // Start/stop the background polling thread
    void startPolling(const std::string& adbPath, int intervalMs = 2000);
    void stopPolling();

    // Get current device list (thread-safe)
    std::vector<Device> getDevices() const;

    // Get the currently selected device serial
    const std::string& getSelectedSerial() const { return m_selectedSerial; }
    void setSelectedSerial(const std::string& serial) { m_selectedSerial = serial; }

    // Get the selected device (returns first if none selected)
    Device getSelectedDevice() const;

    // Auto-select first online device if current selection is invalid
    void ensureValidSelection();

    // Get device count
    size_t getDeviceCount() const;

    // Check if any device is connected
    bool hasDevice() const { return getDeviceCount() > 0; }

    // Check if multiple devices are connected (need -s serial)
    bool hasMultipleDevices() const { return getDeviceCount() > 1; }

    // Register callback for device list changes
    void setChangeCallback(DeviceChangeCallback cb) { m_changeCallback = std::move(cb); }

private:
    DeviceManager() = default;
    ~DeviceManager();
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    void pollThreadFunc();
    void refreshDeviceList();
    void ensureValidSelectionLocked(); // caller must hold m_mutex

    mutable std::mutex m_mutex;
    std::vector<Device> m_devices;
    std::string m_selectedSerial;
    std::string m_adbPath;
    int m_intervalMs = 2000;
    std::atomic<bool> m_running{false};
    std::thread m_pollThread;
    DeviceChangeCallback m_changeCallback;
};
