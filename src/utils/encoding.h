#pragma once
#include <string>
#include <windows.h>

// Windows codepage conversion utilities.
// Critical for proper display of adb/scrcpy output on Chinese Windows (GBK/CP936).
// ImGui expects UTF-8, but adb.exe may output GBK on Chinese systems.
namespace Encoding {

    // Convert from system codepage (GBK on Chinese Windows) to UTF-8
    inline std::string systemToUtf8(const std::string& input) {
        if (input.empty()) return "";

        // Step 1: System codepage → Wide char (UTF-16)
        int wideLen = MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, nullptr, 0);
        if (wideLen <= 0) return input; // fallback: return as-is

        std::wstring wideStr(wideLen - 1, L'\0');
        MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, &wideStr[0], wideLen);

        // Step 2: Wide char (UTF-16) → UTF-8
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Len <= 0) return input;

        std::string utf8Str(utf8Len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Len, nullptr, nullptr);

        return utf8Str;
    }

    // Convert from UTF-8 to system codepage (for CreateProcess command lines)
    inline std::string utf8ToSystem(const std::string& input) {
        if (input.empty()) return "";

        int wideLen = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
        if (wideLen <= 0) return input;

        std::wstring wideStr(wideLen - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, &wideStr[0], wideLen);

        int sysLen = WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (sysLen <= 0) return input;

        std::string sysStr(sysLen - 1, '\0');
        WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, &sysStr[0], sysLen, nullptr, nullptr);

        return sysStr;
    }

    // Get current console codepage
    inline UINT getConsoleCodepage() {
        return GetConsoleOutputCP();
    }

    // Check if current system is using a DBCS (double-byte) codepage like GBK
    inline bool isDbcsCodepage() {
        UINT cp = getConsoleCodepage();
        return (cp == 936 || cp == 950 || cp == 932 || cp == 949); // GBK, Big5, Shift-JIS, EUC-KR
    }

} // namespace Encoding
