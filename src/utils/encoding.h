#pragma once
#include <string>
#include <windows.h>

// Configurable encoding system for adb/scrcpy output conversion.
// Supports auto-detection and manual override for different locales.
namespace Encoding {

    // Encoding mode
    enum class EncodingMode : int {
        Auto = 0,     // Auto-detect from system codepage
        UTF8,         // Force UTF-8 (no conversion needed)
        GBK,          // Chinese Simplified (CP936)
        Big5,         // Chinese Traditional (CP950)
        ShiftJIS,     // Japanese (CP932)
        EUC_KR,       // Korean (CP949)
        Latin1        // Western European (CP1252)
    };

    // Current encoding mode (configurable via Settings)
    inline EncodingMode g_encodingMode = EncodingMode::Auto;

    // Get the codepage for a given encoding mode
    inline UINT getCodepage(EncodingMode mode) {
        switch (mode) {
            case EncodingMode::UTF8:     return CP_UTF8;
            case EncodingMode::GBK:      return 936;
            case EncodingMode::Big5:     return 950;
            case EncodingMode::ShiftJIS: return 932;
            case EncodingMode::EUC_KR:   return 949;
            case EncodingMode::Latin1:   return 1252;
            default: return GetConsoleOutputCP(); // Auto
        }
    }

    // Get the name of an encoding mode
    inline const char* getEncodingName(EncodingMode mode) {
        switch (mode) {
            case EncodingMode::Auto:     return "Auto (System Default)";
            case EncodingMode::UTF8:     return "UTF-8";
            case EncodingMode::GBK:      return "GBK (Chinese Simplified)";
            case EncodingMode::Big5:     return "Big5 (Chinese Traditional)";
            case EncodingMode::ShiftJIS: return "Shift-JIS (Japanese)";
            case EncodingMode::EUC_KR:   return "EUC-KR (Korean)";
            case EncodingMode::Latin1:   return "Latin-1 (Western)";
            default: return "Unknown";
        }
    }

    // Convert from current source encoding to UTF-8
    inline std::string toUtf8(const std::string& input) {
        if (input.empty()) return "";

        UINT srcCP = getCodepage(g_encodingMode);

        // If source is already UTF-8, no conversion needed
        if (srcCP == CP_UTF8) return input;

        // Step 1: Source codepage → Wide char
        int wideLen = MultiByteToWideChar(srcCP, 0, input.c_str(), -1, nullptr, 0);
        if (wideLen <= 0) return input;

        std::wstring wideStr(wideLen - 1, L'\0');
        MultiByteToWideChar(srcCP, 0, input.c_str(), -1, &wideStr[0], wideLen);

        // Step 2: Wide char → UTF-8
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Len <= 0) return input;

        std::string utf8Str(utf8Len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Len, nullptr, nullptr);

        return utf8Str;
    }

    // Convert from UTF-8 to current source encoding (for CreateProcess)
    inline std::string fromUtf8(const std::string& input) {
        if (input.empty()) return "";

        UINT dstCP = getCodepage(g_encodingMode);
        if (dstCP == CP_UTF8) return input;

        int wideLen = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
        if (wideLen <= 0) return input;

        std::wstring wideStr(wideLen - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, &wideStr[0], wideLen);

        int dstLen = WideCharToMultiByte(dstCP, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (dstLen <= 0) return input;

        std::string dstStr(dstLen - 1, '\0');
        WideCharToMultiByte(dstCP, 0, wideStr.c_str(), -1, &dstStr[0], dstLen, nullptr, nullptr);

        return dstStr;
    }

    // Legacy compatibility
    inline std::string systemToUtf8(const std::string& input) { return toUtf8(input); }
    inline bool isDbcsCodepage() {
        UINT cp = getCodepage(g_encodingMode);
        return (cp != CP_UTF8 && cp != 1252);
    }

} // namespace Encoding
