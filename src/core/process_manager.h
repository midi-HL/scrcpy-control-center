#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <memory>
#include <atomic>
#include <functional>
#include <mutex>
#include <deque>

// Manages scrcpy and adb child processes with pipe-based stdout capture.
// Handles CreateProcess, pipe redirection, and zombie process prevention.
class ProcessManager {
public:
    struct ProcessInfo {
        HANDLE hProcess = INVALID_HANDLE_VALUE;
        HANDLE hThread  = INVALID_HANDLE_VALUE;
        HANDLE hReadPipe = INVALID_HANDLE_VALUE;
        DWORD  processId = 0;
        std::string name;
        std::thread readerThread;
        std::atomic<bool> running{false};
    };

    static ProcessManager& instance();

    // Start a scrcpy instance with given arguments
    bool startScrcpy(const std::string& scrcpyPath,
                     const std::vector<std::string>& args,
                     const std::string& serial = "");

    // Stop a specific scrcpy instance by index
    void stopScrcpy(size_t index);

    // Stop all running scrcpy instances
    void stopAll();

    // Execute an adb command and capture output (blocking)
    std::string executeAdb(const std::string& adbPath,
                           const std::vector<std::string>& args,
                           int timeoutMs = 5000);

    // Get number of active processes
    size_t getActiveCount() const;

    // Check if a specific process is still running
    bool isRunning(size_t index) const;

    // Cleanup: called on app exit to kill all children
    void cleanup();

    // Callback for output lines
    using OutputCallback = std::function<void(const std::string&)>;
    void setOutputCallback(OutputCallback cb) { m_outputCallback = std::move(cb); }

private:
    ProcessManager() = default;
    ~ProcessManager();
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;

    // Create process with redirected stdout/stderr
    HANDLE createProcessWithPipe(const std::string& exePath,
                                 const std::vector<std::string>& args,
                                 HANDLE& outReadPipe,
                                 DWORD& outPid);

    void readerThreadFunc(size_t index);

    mutable std::mutex m_mutex;
    std::vector<std::unique_ptr<ProcessInfo>> m_processes;
    OutputCallback m_outputCallback;
};
