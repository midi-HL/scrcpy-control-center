#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

// Simple internationalization system for Chinese/English support
class I18n {
public:
    enum class Language : uint8_t { English, Chinese };

    static I18n& instance();

    void setLanguage(Language lang);
    Language getLanguage() const { return m_language; }

    // Get translated string by key
    const char* tr(const char* key) const;

    // Toggle between English and Chinese
    void toggleLanguage();

private:
    I18n();
    ~I18n() = default;
    I18n(const I18n&) = delete;
    I18n& operator=(const I18n&) = delete;

    Language m_language = Language::English;
    std::unordered_map<std::string, std::string> m_en;
    std::unordered_map<std::string, std::string> m_zh;

    void initTranslations();
};

// Convenience macro
#define TR(key) I18n::instance().tr(key)
