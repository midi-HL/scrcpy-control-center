#include "core/process_manager.h"
#include "utils/logger.h"
#include "utils/encoding.h"
#include <sstream>

ProcessManager::~ProcessManager() {
    cleanup();
}

ProcessManager& ProcessManager::instance() {
    static ProcessManager s_instance;
    return s_instance;
}

HANDLE ProcessManager::createProcessWithPipe(const std::string& exePath,
                                              const std::vector<std::string>& args,
                                              HANDLE& outReadPipe,
                                              DWORD& outPid) {
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hWritePipe = INVALID_HANDLE_VALUE;
    if (!CreatePipe(&outReadPipe, &hWritePipe, &sa, 0)) {
        LOG_ERROR("CreatePipe failed: %lu", GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    // Read end must NOT be inherited by the child
    SetHandleInformation(outReadPipe, HANDLE_FLAG_INHERIT, 0);

    // Build command line with UTF-8 codepage prefix
    // This forces child processes to output UTF-8 instead of GBK on Chinese Windows
    std::ostringstream innerCmd;
    innerCmd << "\"" << exePath << "\"";
    for (const auto& arg : args) {
        innerCmd << " " << arg;
    }

    // Wrap in cmd.exe with chcp 65001 to force UTF-8 output
    std::ostringstream cmdLine;
    cmdLine << "cmd.exe /c \"chcp 65001 >nul && " << innerCmd.str() << "\"";
    std::string cmd = cmdLine.str();

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.hStdOutput = hWritePipe;
    si.hStdError  = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};

    // Create the process
    BOOL ok = CreateProcessA(
        nullptr,
        const_cast<char*>(cmd.c_str()),
        nullptr, nullptr,
        TRUE,              // inherit handles
        CREATE_NO_WINDOW,  // no console window
        nullptr, nullptr,
        &si, &pi
    );

    // Close the write end in parent (child has its own copy)
    CloseHandle(hWritePipe);

    if (!ok) {
        LOG_ERROR("CreateProcess failed for '%s': %lu", exePath.c_str(), GetLastError());
        CloseHandle(outReadPipe);
        outReadPipe = INVALID_HANDLE_VALUE;
        return INVALID_HANDLE_VALUE;
    }

    outPid = pi.dwProcessId;

    // We don't need the thread handle
    CloseHandle(pi.hThread);

    return pi.hProcess;
}

bool ProcessManager::startScrcpy(const std::string& scrcpyPath,
                                  const std::vector<std::string>& args,
                                  const std::string& serial) {
    auto info = std::make_unique<ProcessInfo>();
    info->name = "scrcpy";

    // Build full args with serial
    std::vector<std::string> fullArgs = args;
    if (!serial.empty()) {
        fullArgs.insert(fullArgs.begin(), serial);
        fullArgs.insert(fullArgs.begin(), "-s");
    }

    HANDLE hReadPipe = INVALID_HANDLE_VALUE;
    DWORD pid = 0;
    info->hProcess = createProcessWithPipe(scrcpyPath, fullArgs, hReadPipe, pid);

    if (info->hProcess == INVALID_HANDLE_VALUE) {
        return false;
    }

    info->hReadPipe = hReadPipe;
    info->processId = pid;
    info->running = true;

    // Start reader thread (inside lock to prevent race with stopAll/cleanup)
    size_t index;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        index = m_processes.size();
        m_processes.push_back(std::move(info));
        m_processes[index]->readerThread = std::thread(&ProcessManager::readerThreadFunc, this, index);
    }

    LOG_INFO("Started scrcpy (PID: %lu) with %zu args", pid, fullArgs.size());
    return true;
}

void ProcessManager::readerThreadFunc(size_t index) {
    char buf[4096];
    DWORD bytesRead;

    HANDLE hPipe = INVALID_HANDLE_VALUE;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (index < m_processes.size()) {
            hPipe = m_processes[index]->hReadPipe;
        }
    }

    if (hPipe == INVALID_HANDLE_VALUE) return;

    while (true) {
        BOOL ok = ReadFile(hPipe, buf, sizeof(buf) - 1, &bytesRead, nullptr);
        if (!ok || bytesRead == 0) break;

        buf[bytesRead] = '\0';

        // Convert from system codepage (GBK on Chinese Windows) to UTF-8
        // This is critical for proper ImGui rendering of adb/scrcpy output
        std::string line(buf, bytesRead);
        if (Encoding::isDbcsCodepage()) {
            line = Encoding::systemToUtf8(line);
        }

        if (m_outputCallback) {
            m_outputCallback(line);
        }

        // Also log it
        LOG_INFO("[scrcpy] %s", line.c_str());
    }

    // Process ended
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (index < m_processes.size()) {
            m_processes[index]->running = false;
        }
    }
}

void ProcessManager::stopScrcpy(size_t index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index >= m_processes.size()) return;

    auto& info = m_processes[index];
    if (info->running) {
        TerminateProcess(info->hProcess, 1);
        info->running = false;
        LOG_INFO("Stopped scrcpy (PID: %lu)", info->processId);
    }
}

void ProcessManager::stopAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& info : m_processes) {
        if (info->running) {
            TerminateProcess(info->hProcess, 1);
            info->running = false;
        }
    }
    LOG_INFO("All scrcpy processes stopped");
}

size_t ProcessManager::getActiveCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& info : m_processes) {
        if (info->running) count++;
    }
    return count;
}

bool ProcessManager::isRunning(size_t index) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index >= m_processes.size()) return false;
    return m_processes[index]->running;
}

std::string ProcessManager::executeAdb(const std::string& adbPath,
                                        const std::vector<std::string>& args,
                                        int timeoutMs) {
    // Build command line with UTF-8 codepage prefix
    std::ostringstream innerCmd;
    innerCmd << "\"" << adbPath << "\"";
    for (const auto& arg : args) {
        innerCmd << " " << arg;
    }

    std::ostringstream cmdLine;
    cmdLine << "cmd.exe /c \"chcp 65001 >nul && " << innerCmd.str() << "\"";
    std::string cmd = cmdLine.str();

    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        LOG_ERROR("CreatePipe failed for adb: %lu", GetLastError());
        return "";
    }
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    BOOL ok = CreateProcessA(nullptr, const_cast<char*>(cmd.c_str()),
                             nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                             nullptr, nullptr, &si, &pi);

    CloseHandle(hWritePipe);

    if (!ok) {
        LOG_ERROR("CreateProcess failed for adb: %lu", GetLastError());
        CloseHandle(hReadPipe);
        return "";
    }

    // Read output
    std::string output;
    char buf[4096];
    DWORD bytesRead;

    // Use WaitForSingleObject with timeout to avoid hanging
    DWORD waitResult = WaitForSingleObject(pi.hProcess, timeoutMs);
    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        LOG_WARN("adb command timed out after %dms", timeoutMs);
    }

    while (ReadFile(hReadPipe, buf, sizeof(buf) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buf[bytesRead] = '\0';
        output += buf;
    }

    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Convert from system codepage to UTF-8 for proper display
    if (Encoding::isDbcsCodepage()) {
        output = Encoding::systemToUtf8(output);
    }

    return output;
}

void ProcessManager::cleanup() {
    stopAll();

    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& info : m_processes) {
        if (info->readerThread.joinable()) {
            info->readerThread.join();
        }
        if (info->hProcess != INVALID_HANDLE_VALUE) {
            CloseHandle(info->hProcess);
        }
        if (info->hReadPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(info->hReadPipe);
        }
    }
    m_processes.clear();
}
