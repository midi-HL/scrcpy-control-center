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

    SetHandleInformation(outReadPipe, HANDLE_FLAG_INHERIT, 0);

    // Build command line with UTF-8 codepage prefix
    std::ostringstream innerCmd;
    innerCmd << "\"" << exePath << "\"";
    for (const auto& arg : args) {
        innerCmd << " " << arg;
    }

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

    BOOL ok = CreateProcessA(
        nullptr, const_cast<char*>(cmd.c_str()),
        nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
        nullptr, nullptr, &si, &pi
    );

    CloseHandle(hWritePipe);

    if (!ok) {
        LOG_ERROR("CreateProcess failed for '%s': %lu", exePath.c_str(), GetLastError());
        CloseHandle(outReadPipe);
        outReadPipe = INVALID_HANDLE_VALUE;
        return INVALID_HANDLE_VALUE;
    }

    outPid = pi.dwProcessId;
    CloseHandle(pi.hThread);
    return pi.hProcess;
}

bool ProcessManager::startScrcpy(const std::string& scrcpyPath,
                                  const std::vector<std::string>& args,
                                  const std::string& serial,
                                  const InstanceInfo& meta) {
    auto info = std::make_unique<ProcessInfo>();
    info->name = meta.name.empty() ? "scrcpy" : meta.name;
    info->meta = meta;
    info->meta.serial = serial;

    // Build full args — serial is injected by CommandBuilder, not here
    std::vector<std::string> fullArgs = args;

    HANDLE hReadPipe = INVALID_HANDLE_VALUE;
    DWORD pid = 0;
    info->hProcess = createProcessWithPipe(scrcpyPath, fullArgs, hReadPipe, pid);

    if (info->hProcess == INVALID_HANDLE_VALUE) {
        return false;
    }

    info->hReadPipe = hReadPipe;
    info->processId = pid;
    info->running = true;
    info->meta.processId = pid;
    info->meta.running = true;

    // Start reader thread inside lock to prevent race with stopAll/cleanup
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t index = m_processes.size();
    m_processes.push_back(std::move(info));
    m_processes[index]->readerThread = std::thread(&ProcessManager::readerThreadFunc, this, index);

    LOG_INFO("Started scrcpy '%s' (PID: %lu) with %zu args",
             meta.name.c_str(), pid, fullArgs.size());
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

        std::string line(buf, bytesRead);
        if (Encoding::isDbcsCodepage()) {
            line = Encoding::systemToUtf8(line);
        }

        if (m_outputCallback) {
            m_outputCallback(line);
        }

        LOG_INFO("[scrcpy] %s", line.c_str());
    }

    // Process ended
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (index < m_processes.size()) {
            m_processes[index]->running = false;
            m_processes[index]->meta.running = false;
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
        info->meta.running = false;
        LOG_INFO("Stopped scrcpy '%s' (PID: %lu)", info->name.c_str(), info->processId);
    }
}

void ProcessManager::stopAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& info : m_processes) {
        if (info->running) {
            TerminateProcess(info->hProcess, 1);
            info->running = false;
            info->meta.running = false;
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

std::vector<InstanceInfo> ProcessManager::getInstances() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<InstanceInfo> result;
    for (const auto& info : m_processes) {
        InstanceInfo meta = info->meta;
        meta.running = info->running;
        result.push_back(meta);
    }
    return result;
}

bool ProcessManager::isRunning(size_t index) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index >= m_processes.size()) return false;
    return m_processes[index]->running;
}

std::string ProcessManager::executeAdb(const std::string& adbPath,
                                        const std::vector<std::string>& args,
                                        int timeoutMs) {
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

    std::string output;
    char buf[4096];
    DWORD bytesRead;

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
