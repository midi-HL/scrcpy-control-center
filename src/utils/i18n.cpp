#include "utils/i18n.h"
#include <cstring>

I18n& I18n::instance() {
    static I18n s_instance;
    return s_instance;
}

I18n::I18n() {
    initTranslations();
}

void I18n::setLanguage(Language lang) {
    m_language = lang;
}

void I18n::toggleLanguage() {
    m_language = (m_language == Language::English) ? Language::Chinese : Language::English;
}

const char* I18n::tr(const char* key) const {
    const auto& map = (m_language == Language::Chinese) ? m_zh : m_en;
    auto it = map.find(key);
    if (it != map.end()) {
        return it->second.c_str();
    }
    return key; // fallback to key itself
}

void I18n::initTranslations() {
    // ── Sidebar Menu Items ───────────────────────────────────────────────────
    m_en["menu_device"]           = "Device";
    m_zh["menu_device"]           = "设备";

    m_en["menu_connection"]       = "Connection";
    m_zh["menu_connection"]       = "连接";

    m_en["menu_display"]          = "Display";
    m_zh["menu_display"]          = "显示";

    m_en["menu_audio"]            = "Audio";
    m_zh["menu_audio"]            = "音频";

    m_en["menu_control"]          = "Control";
    m_zh["menu_control"]          = "控制";

    m_en["menu_recording"]        = "Recording";
    m_zh["menu_recording"]        = "录制";

    m_en["menu_virtual_display"]  = "Virtual Display";
    m_zh["menu_virtual_display"]  = "虚拟显示";

    m_en["menu_app_manager"]      = "App Manager";
    m_zh["menu_app_manager"]      = "应用管理";

    m_en["menu_camera"]           = "Camera";
    m_zh["menu_camera"]           = "相机";

    m_en["menu_gaming"]           = "Gaming";
    m_zh["menu_gaming"]           = "游戏";

    m_en["menu_advanced"]         = "Advanced";
    m_zh["menu_advanced"]         = "高级";

    m_en["menu_logs"]             = "Logs";
    m_zh["menu_logs"]             = "日志";

    m_en["menu_settings"]         = "Settings";
    m_zh["menu_settings"]         = "设置";

    // ── Common UI ────────────────────────────────────────────────────────────
    m_en["btn_start_scrcpy"]      = "Start scrcpy";
    m_zh["btn_start_scrcpy"]      = "启动 scrcpy";

    m_en["btn_stop_scrcpy"]       = "Stop scrcpy";
    m_zh["btn_stop_scrcpy"]       = "停止 scrcpy";

    m_en["btn_apply_profile"]     = "Apply Profile";
    m_zh["btn_apply_profile"]     = "应用配置";

    m_en["cmd_preview"]           = "Command Preview";
    m_zh["cmd_preview"]           = "命令预览";

    m_en["no_device"]             = "No device connected";
    m_zh["no_device"]             = "未连接设备";

    m_en["device_online"]         = "Online";
    m_zh["device_online"]         = "在线";

    m_en["device_offline"]        = "Offline";
    m_zh["device_offline"]        = "离线";

    m_en["device_unauthorized"]   = "Unauthorized";
    m_zh["device_unauthorized"]   = "未授权";

    // ── Settings ─────────────────────────────────────────────────────────────
    m_en["settings_language"]     = "Language";
    m_zh["settings_language"]     = "语言";

    m_en["settings_theme"]        = "Theme";
    m_zh["settings_theme"]        = "主题";

    m_en["settings_scrcpy_path"]  = "Scrcpy Path";
    m_zh["settings_scrcpy_path"]  = "Scrcpy 路径";

    m_en["settings_adb_path"]     = "ADB Path";
    m_zh["settings_adb_path"]     = "ADB 路径";

    m_en["theme_dark"]            = "Dark";
    m_zh["theme_dark"]            = "深色";

    m_en["theme_light"]           = "Light";
    m_zh["theme_light"]           = "浅色";
}
