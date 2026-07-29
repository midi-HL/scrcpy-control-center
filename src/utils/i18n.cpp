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
    m_language = (m_language == Language::Chinese) ? Language::Chinese : Language::English;
}

const char* I18n::tr(const char* key) const {
    const auto& map = (m_language == Language::Chinese) ? m_zh : m_en;
    auto it = map.find(key);
    if (it != map.end()) return it->second.c_str();
    return key;
}

void I18n::initTranslations() {
    // ═══════════════════════════════════════════════════════════════════════════
    // SIDEBAR MENU
    // ═══════════════════════════════════════════════════════════════════════════
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

    // ═══════════════════════════════════════════════════════════════════════════
    // COMMON BUTTONS & LABELS
    // ═══════════════════════════════════════════════════════════════════════════
    m_en["btn_start_scrcpy"]      = "Start scrcpy";
    m_zh["btn_start_scrcpy"]      = "启动 scrcpy";
    m_en["btn_stop_scrcpy"]       = "Stop scrcpy";
    m_zh["btn_stop_scrcpy"]       = "停止 scrcpy";
    m_en["btn_stop_all"]          = "Stop All";
    m_zh["btn_stop_all"]          = "全部停止";
    m_en["btn_refresh"]           = "Refresh";
    m_zh["btn_refresh"]           = "刷新";
    m_en["btn_save"]              = "Save";
    m_zh["btn_save"]              = "保存";
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
    m_en["running_instances"]     = "Running Instances";
    m_zh["running_instances"]     = "运行中的实例";
    m_en["no_device_selected"]    = "No device selected";
    m_zh["no_device_selected"]    = "未选择设备";
    m_en["select_device_first"]   = "Select a device in the Device tab first.";
    m_zh["select_device_first"]   = "请先在设备标签页中选择一个设备。";

    // ═══════════════════════════════════════════════════════════════════════════
    // DEVICE PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    m_en["dev_connected"]         = "Connected Devices";
    m_zh["dev_connected"]         = "已连接设备";
    m_en["dev_serial"]            = "Serial";
    m_zh["dev_serial"]            = "序列号";
    m_en["dev_model"]             = "Model";
    m_zh["dev_model"]             = "型号";
    m_en["dev_state"]             = "State";
    m_zh["dev_state"]             = "状态";
    m_en["dev_type"]              = "Type";
    m_zh["dev_type"]              = "类型";
    m_en["dev_usb_hint"]          = "Connect a device via USB cable";
    m_zh["dev_usb_hint"]          = "通过 USB 线连接设备";
    m_en["dev_wifi_hint"]         = "Or use Connection tab for WiFi (port 5555)";
    m_zh["dev_wifi_hint"]         = "或使用连接标签页通过 WiFi 连接 (端口 5555)";
    m_en["dev_old_pc_hint"]       = "Tip: If your old PC can't USB-connect, use another device to enable wireless ADB, then connect via WiFi here.";
    m_zh["dev_old_pc_hint"]       = "提示: 如果旧电脑无法 USB 连接，可以用另一台设备开启无线 ADB，然后在此通过 WiFi 连接。";
    m_en["dev_active"]            = "Active";
    m_zh["dev_active"]            = "活跃";
    m_en["desc_device"]           = "View and manage connected Android devices. Select a device to use with scrcpy.";
    m_zh["desc_device"]           = "查看和管理已连接的 Android 设备。选择一个设备用于 scrcpy。";
    m_en["dev_select"]            = "Select";
    m_zh["dev_select"]            = "选择";
    m_en["desc_expand"]           = "Show description";
    m_zh["desc_expand"]           = "显示说明";
    m_en["desc_show_help"]        = "Click for description";
    m_zh["desc_show_help"]        = "点击显示说明";

    // ═══════════════════════════════════════════════════════════════════════════
    // CONNECTION PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    m_en["conn_wifi"]             = "WiFi (TCP/IP) Connection";
    m_zh["conn_wifi"]             = "WiFi (TCP/IP) 连接";
    m_en["conn_ip"]               = "IP Address";
    m_zh["conn_ip"]               = "IP 地址";
    m_en["conn_port"]             = "Port";
    m_zh["conn_port"]             = "端口";
    m_en["conn_connect_wifi"]     = "Connect via WiFi";
    m_zh["conn_connect_wifi"]     = "通过 WiFi 连接";
    m_en["conn_wizard"]           = "TCP/IP Setup Wizard";
    m_zh["conn_wizard"]           = "TCP/IP 设置向导";
    m_en["conn_step1"]            = "1. Connect device via USB (or use another PC)";
    m_zh["conn_step1"]            = "1. 通过 USB 连接设备 (或使用另一台电脑)";
    m_en["conn_step2"]            = "2. Click 'Enable TCP/IP' below";
    m_zh["conn_step2"]            = "2. 点击下方的 '启用 TCP/IP'";
    m_en["conn_step3"]            = "3. Disconnect USB cable";
    m_zh["conn_step3"]            = "3. 断开 USB 线";
    m_en["conn_step4"]            = "4. Enter device IP above and connect";
    m_zh["conn_step4"]            = "4. 输入设备 IP 并连接";
    m_en["conn_enable_tcpip"]     = "Enable TCP/IP (port 5555)";
    m_zh["conn_enable_tcpip"]     = "启用 TCP/IP (端口 5555)";
    m_en["conn_selected"]         = "Selected";
    m_zh["conn_selected"]         = "已选择";
    m_en["desc_connection"]       = "Manage wireless ADB connections. For old PCs without USB, enable TCP/IP on another device first, then connect via WiFi here.";
    m_zh["desc_connection"]       = "管理无线 ADB 连接。对于没有 USB 的旧电脑，先在其他设备上启用 TCP/IP，然后在此通过 WiFi 连接。";

    // ═══════════════════════════════════════════════════════════════════════════
    // DISPLAY PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    m_en["disp_otg_warning"]      = "OTG mode active: video controls disabled.";
    m_zh["disp_otg_warning"]      = "OTG 模式已激活：视频控件已禁用。";
    m_en["desc_display"]          = "Configure video display settings: resolution, bitrate, frame rate, codec, orientation, and cropping.";
    m_zh["desc_display"]          = "配置视频显示设置：分辨率、比特率、帧率、编码器、方向和裁剪。";

    // ═══════════════════════════════════════════════════════════════════════════
    // AUDIO PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    m_en["audio_otg_warning"]     = "OTG mode active: audio controls disabled.";
    m_zh["audio_otg_warning"]     = "OTG 模式已激活：音频控件已禁用。";
    m_en["desc_audio"]            = "Configure audio settings: codec, bitrate, source (output/microphone), buffering, and duplication.";
    m_zh["desc_audio"]            = "配置音频设置：编码器、比特率、源 (输出/麦克风)、缓冲和复制。";

    // ═══════════════════════════════════════════════════════════════════════════
    // CONTROL PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    m_en["desc_control"]          = "Configure input device modes: keyboard, mouse, gamepad injection. Also control screen state and wake locks.";
    m_zh["desc_control"]          = "配置输入设备模式：键盘、鼠标、手柄注入。同时控制屏幕状态和唤醒锁。";

    // ═══════════════════════════════════════════════════════════════════════════
    // RECORDING PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    m_en["desc_recording"]        = "Configure recording: file path, format (MP4/MKV/OPUS/FLAC/WAV), time limit, and playback options.";
    m_zh["desc_recording"]        = "配置录制：文件路径、格式 (MP4/MKV/OPUS/FLAC/WAV)、时间限制和回放选项。";

    // ═══════════════════════════════════════════════════════════════════════════
    // VIRTUAL DISPLAY PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    m_en["vd_launcher"]           = "Multi-Window Launcher";
    m_zh["vd_launcher"]           = "多窗口启动器";
    m_en["vd_launcher_desc"]      = "Enter one app package per line. Each creates a separate resizable virtual display window with --flex-display.";
    m_zh["vd_launcher_desc"]      = "每行输入一个应用包名。每个都会创建一个独立的可调大小的虚拟显示器窗口 (使用 --flex-display)。";
    m_en["vd_size"]               = "VD Size";
    m_zh["vd_size"]               = "虚拟显示器尺寸";
    m_en["vd_launch_all"]         = "Launch All as Virtual Displays";
    m_zh["vd_launch_all"]         = "全部启动为虚拟显示器";
    m_en["vd_launched"]           = "Launched";
    m_zh["vd_launched"]           = "已启动";
    m_en["vd_failed"]             = "Failed to launch VD for";
    m_zh["vd_failed"]             = "启动虚拟显示器失败";
    m_en["desc_virtual_display"]  = "Create Android virtual displays. Run apps in isolated windows. Supports --flex-display for auto-resize when dragging window edges.";
    m_zh["desc_virtual_display"]  = "创建 Android 虚拟显示器。在隔离的窗口中运行应用。支持 --flex-display，拖拽窗口边缘时自动调整分辨率。";

    // ═══════════════════════════════════════════════════════════════════════════
    // APP MANAGER PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    m_en["app_refresh_list"]      = "Refresh App List";
    m_zh["app_refresh_list"]      = "刷新应用列表";
    m_en["app_refreshing"]        = "Refreshing app list...";
    m_zh["app_refreshing"]        = "正在刷新应用列表...";
    m_en["app_no_apps"]           = "No apps loaded. Click 'Refresh App List' to scan device.";
    m_zh["app_no_apps"]           = "未加载应用。点击 '刷新应用列表' 扫描设备。";
    m_en["app_filter"]            = "Filter...";
    m_zh["app_filter"]            = "过滤...";
    m_en["app_freeze"]            = "Freeze";
    m_zh["app_freeze"]            = "冻结";
    m_en["app_unfreeze"]          = "Unfreeze";
    m_zh["app_unfreeze"]          = "解冻";
    m_en["app_launch"]            = "Launch";
    m_zh["app_launch"]            = "启动";
    m_en["app_open_vd"]           = "Open in VD";
    m_zh["app_open_vd"]           = "在 VD 中打开";
    m_en["app_system"]            = "Sys";
    m_zh["app_system"]            = "系统";
    m_en["app_user"]              = "User";
    m_zh["app_user"]              = "用户";
    m_en["desc_app_manager"]      = "Manage installed apps: view list, freeze/unfreeze, launch on main display or virtual display. App icons are cached locally for fast loading.";
    m_zh["desc_app_manager"]      = "管理已安装应用：查看列表、冻结/解冻、在主显示器或虚拟显示器上启动。应用图标本地缓存以加快加载速度。";

    // ═══════════════════════════════════════════════════════════════════════════
    // CAMERA PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    m_en["desc_camera"]           = "Use device camera as video source. Select camera ID, facing direction, size, frame rate, and enable torch/zoom.";
    m_zh["desc_camera"]           = "使用设备相机作为视频源。选择相机 ID、朝向、尺寸、帧率，以及启用闪光灯/变焦。";

    // ═══════════════════════════════════════════════════════════════════════════
    // GAMING PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    m_en["gaming_otg_desc"]       = "OTG mode: Direct USB input without mirroring.";
    m_zh["gaming_otg_desc"]       = "OTG 模式：直接 USB 输入，无镜像。";
    m_en["gaming_driver_hint"]    = "Requires WinUSB driver (install via Zadig on Win8.1).";
    m_zh["gaming_driver_hint"]    = "需要 WinUSB 驱动 (在 Win8.1 上通过 Zadig 安装)。";
    m_en["desc_gaming"]           = "OTG mode for direct USB input (keyboard/mouse/gamepad) without screen mirroring. Gamepad mode maps PC controllers to the Android device.";
    m_zh["desc_gaming"]           = "OTG 模式用于直接 USB 输入 (键盘/鼠标/手柄)，无屏幕镜像。手柄模式将 PC 控制器映射到 Android 设备。";

    // ═══════════════════════════════════════════════════════════════════════════
    // ADVANCED PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    m_en["adv_parse_cmd"]         = "Paste a scrcpy command to reverse-parse:";
    m_zh["adv_parse_cmd"]         = "粘贴 scrcpy 命令进行逆向解析：";
    m_en["adv_params"]            = "Advanced Parameters";
    m_zh["adv_params"]            = "高级参数";
    m_en["desc_advanced"]         = "Window options (fullscreen, borderless, always-on-top), screen timeout, screensaver, and command reverse-parsing. Paste any scrcpy command to auto-configure all controls.";
    m_zh["desc_advanced"]         = "窗口选项 (全屏、无边框、置顶)、屏幕超时、屏保和命令逆向解析。粘贴任意 scrcpy 命令可自动配置所有控件。";

    // ═══════════════════════════════════════════════════════════════════════════
    // LOGS PANEL
    // ═══════════════════════════════════════════════════════════════════════════
    m_en["logs_filter"]           = "Filter logs...";
    m_zh["logs_filter"]           = "过滤日志...";
    m_en["logs_clear"]            = "Clear";
    m_zh["logs_clear"]            = "清除";
    m_en["logs_auto_scroll"]      = "Auto-scroll";
    m_zh["logs_auto_scroll"]      = "自动滚动";

    // ═══════════════════════════════════════════════════════════════════════════
    // SETTINGS PANEL
    // ═══════════════════════════════════════════════════════════════════════════
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
    m_en["settings_profile"]      = "Profile (Scene Preset)";
    m_zh["settings_profile"]      = "配置方案 (场景预设)";
    m_en["settings_save_profile"] = "Save as Profile";
    m_zh["settings_save_profile"] = "保存为配置方案";
    m_en["settings_polling"]      = "Device Polling Interval (seconds)";
    m_zh["settings_polling"]      = "设备轮询间隔 (秒)";
    m_en["settings_fetch_icons"]  = "Fetch app icons from device (slower first load)";
    m_zh["settings_fetch_icons"]  = "从设备获取应用图标 (首次加载较慢)";
    m_en["settings_encoding"]     = "Text Encoding (for adb/scrcpy output)";
    m_zh["settings_encoding"]     = "文字编码 (用于 adb/scrcpy 输出)";
    m_en["settings_encoding_desc"]= "Change this if Chinese/Japanese/Korean text shows as garbled characters. Auto mode uses your system default codepage.";
    m_zh["settings_encoding_desc"]= "如果中文/日文/韩文显示为乱码，请更改此项。自动模式使用系统默认代码页。";
    m_en["settings_paths_desc"]   = "Paths to scrcpy.exe and adb.exe. Place them in the same directory as this app for automatic detection, or enter full paths here.";
    m_zh["settings_paths_desc"]   = "scrcpy.exe 和 adb.exe 的路径。将它们放在本程序同目录下可自动检测，或在此输入完整路径。";
    m_en["settings_polling_desc"] = "How often to check for device connections. Lower values = faster detection but more CPU. 20 seconds is recommended for most users.";
    m_zh["settings_polling_desc"] = "检测设备连接的频率。值越低 = 检测越快但 CPU 占用更高。建议大多数用户使用 20 秒。";
    m_en["settings_icons_desc"]   = "When enabled, app icons are extracted from APK files and cached locally. First load is slower but subsequent loads are instant. Disable to only show app names.";
    m_zh["settings_icons_desc"]   = "启用后，应用图标从 APK 文件中提取并本地缓存。首次加载较慢，但后续加载即时。禁用则仅显示应用名称。";
    m_en["settings_profile_desc"] = "Quick-load a preset configuration. Gaming: high FPS, low latency. Streaming: high bitrate, audio on. Battery Saver: low FPS, low bitrate, screen off.";
    m_zh["settings_profile_desc"] = "快速加载预设配置。游戏：高帧率、低延迟。串流：高比特率、音频开启。省电：低帧率、低比特率、关闭屏幕。";

    // ═══════════════════════════════════════════════════════════════════════════
    // FEATURE DESCRIPTIONS (for expandable help under each control)
    // ═══════════════════════════════════════════════════════════════════════════

    // Display features
    m_en["feat_max_size"]              = "Maximum video dimension in pixels. Lower = faster but blurrier. 0 = use device native resolution.";
    m_zh["feat_max_size"]              = "最大视频尺寸 (像素)。越低 = 越快但越模糊。0 = 使用设备原始分辨率。";
    m_en["feat_video_bit_rate"]        = "Video bitrate. Higher = better quality but more bandwidth. Default 8M is good for most uses.";
    m_zh["feat_video_bit_rate"]        = "视频比特率。越高 = 质量越好但带宽越大。默认 8M 适合大多数场景。";
    m_en["feat_max_fps"]               = "Maximum frame rate. Lower = smoother on slow devices. 0 = unlimited (device default, usually 30).";
    m_zh["feat_max_fps"]               = "最大帧率。越低 = 在慢设备上更流畅。0 = 无限制 (设备默认，通常 30)。";
    m_en["feat_video_codec"]           = "Video codec. h264: best compatibility, lowest latency. h265: better quality, higher CPU. av1: best quality, rare on devices.";
    m_zh["feat_video_codec"]           = "视频编码器。h264：最佳兼容性、最低延迟。h265：质量更好、CPU 更高。av1：质量最好、设备少见。";
    m_en["feat_crop"]                  = "Crop the device screen. Format: width:height:x:y. Useful for showing only part of the screen.";
    m_zh["feat_crop"]                  = "裁剪设备屏幕。格式：宽:高:X:Y。适用于只显示屏幕的一部分。";
    m_en["feat_capture_orientation"]   = "Orientation of the captured video from the device. Affects recording.";
    m_zh["feat_capture_orientation"]   = "从设备采集的视频方向。影响录制。";
    m_en["feat_orientation"]           = "Client-side display orientation. Can be changed dynamically with keyboard shortcuts.";
    m_zh["feat_orientation"]           = "客户端显示方向。可通过键盘快捷键动态更改。";
    m_en["feat_video_buffer"]          = "Video buffer in milliseconds. Adds latency but smooths playback. 0 = no buffer (lowest latency).";
    m_zh["feat_video_buffer"]          = "视频缓冲 (毫秒)。增加延迟但使播放更平滑。0 = 无缓冲 (最低延迟)。";
    m_en["feat_no_video"]              = "Disable video completely. Only audio will be forwarded.";
    m_zh["feat_no_video"]              = "完全禁用视频。仅转发音频。";
    m_en["feat_print_fps"]             = "Print current FPS to the console/log. Useful for performance monitoring.";
    m_zh["feat_print_fps"]             = "将当前帧率打印到控制台/日志。用于性能监控。";
    m_en["feat_no_video_playback"]     = "Capture video but don't play it locally. Useful for recording or V4L2 sink.";
    m_zh["feat_no_video_playback"]     = "采集视频但不在本地播放。适用于录制或 V4L2 输出。";
    m_en["feat_render_fit"]            = "How video fits the window. letterbox: preserve ratio with bars. unscaled: no scaling. stretched: fill window.";
    m_zh["feat_render_fit"]            = "视频如何适配窗口。letterbox：保持比例加黑边。unscaled：不缩放。stretched：拉伸填充。";

    // Audio features
    m_en["feat_no_audio"]              = "Disable audio forwarding completely.";
    m_zh["feat_no_audio"]              = "完全禁用音频转发。";
    m_en["feat_audio_codec"]           = "Audio codec. opus: best quality. aac: wide compatibility. flac: lossless. raw: uncompressed PCM.";
    m_zh["feat_audio_codec"]           = "音频编码器。opus：最佳质量。aac：广泛兼容。flac：无损。raw：未压缩 PCM。";
    m_en["feat_audio_bit_rate"]        = "Audio bitrate. Default 128K is good for most uses. Lower for bandwidth saving.";
    m_zh["feat_audio_bit_rate"]        = "音频比特率。默认 128K 适合大多数场景。降低可节省带宽。";
    m_en["feat_audio_source"]          = "Audio source. output: device audio output. playback: app playback. mic: microphone. voice-call: phone call audio.";
    m_zh["feat_audio_source"]          = "音频源。output：设备音频输出。playback：应用播放。mic：麦克风。voice-call：通话音频。";
    m_en["feat_audio_buffer"]          = "Audio buffer in milliseconds. Lower = less latency but more glitches. Default 50ms is balanced.";
    m_zh["feat_audio_buffer"]          = "音频缓冲 (毫秒)。越低 = 延迟越少但杂音越多。默认 50ms 是平衡值。";
    m_en["feat_audio_dup"]             = "Keep audio playing on the device while mirroring. Requires Android 13+ and audio source = playback.";
    m_zh["feat_audio_dup"]             = "镜像时保持音频在设备上播放。需要 Android 13+ 且音频源 = playback。";
    m_en["feat_no_audio_playback"]     = "Capture audio but don't play it locally. Useful for recording only.";
    m_zh["feat_no_audio_playback"]     = "采集音频但不在本地播放。适用于仅录制。";
    m_en["feat_audio_output_buffer"]   = "Audio output buffer. Default 10ms. Increase if audio sounds robotic or glitchy.";
    m_zh["feat_audio_output_buffer"]   = "音频输出缓冲。默认 10ms。如果音频出现机器人声或杂音，请增大。";
    m_en["feat_require_audio"]         = "If enabled, scrcpy will fail if audio capture is not available (instead of silently continuing without audio).";
    m_zh["feat_require_audio"]         = "如果启用，当音频采集不可用时 scrcpy 将失败 (而不是静默继续无音频)。";

    // Control features
    m_en["feat_keyboard"]              = "Keyboard input mode. sdk: standard Android input. uhid: virtual HID device. aoa: USB accessory. disabled: no keyboard.";
    m_zh["feat_keyboard"]              = "键盘输入模式。sdk：标准 Android 输入。uhid：虚拟 HID 设备。aoa：USB 附件。disabled：禁用键盘。";
    m_en["feat_mouse"]                 = "Mouse input mode. Same options as keyboard. UHID mode may provide better compatibility.";
    m_zh["feat_mouse"]                 = "鼠标输入模式。与键盘相同选项。UHID 模式可能提供更好的兼容性。";
    m_en["feat_gamepad"]               = "Gamepad input mode. uhid: maps PC gamepad to Android virtual gamepad. sdk: standard. aoa: USB accessory. disabled: no gamepad.";
    m_zh["feat_gamepad"]               = "手柄输入模式。uhid：将 PC 手柄映射到 Android 虚拟手柄。sdk：标准。aoa：USB 附件。disabled：禁用手柄。";
    m_en["feat_stay_awake"]            = "Prevent device from sleeping while charging (USB). No effect on WiFi-only connections.";
    m_zh["feat_stay_awake"]            = "充电时防止设备休眠 (USB)。仅 WiFi 连接时无效。";
    m_en["feat_turn_screen_off"]       = "Turn off the device screen while mirroring. Saves battery. Can be toggled with keyboard shortcuts.";
    m_zh["feat_turn_screen_off"]       = "镜像时关闭设备屏幕。节省电量。可通过键盘快捷键切换。";
    m_en["feat_show_touches"]          = "Show physical touch indicators on the device screen. Useful for presentations.";
    m_zh["feat_show_touches"]          = "在设备屏幕上显示物理触摸指示器。适用于演示。";
    m_en["feat_keep_active"]           = "Periodically signal user activity to prevent the screen from turning off.";
    m_zh["feat_keep_active"]           = "定期发送用户活动信号以防止屏幕关闭。";
    m_en["feat_power_off_on_close"]    = "Turn off the device screen when scrcpy is closed.";
    m_zh["feat_power_off_on_close"]    = "关闭 scrcpy 时关闭设备屏幕。";
    m_en["feat_no_power_on"]           = "Don't turn on the device screen when scrcpy starts.";
    m_zh["feat_no_power_on"]           = "启动 scrcpy 时不打开设备屏幕。";
    m_en["feat_no_control"]            = "Disable all input (keyboard, mouse, gamepad). View-only mode.";
    m_zh["feat_no_control"]            = "禁用所有输入 (键盘、鼠标、手柄)。仅查看模式。";

    // Recording features
    m_en["feat_record_file"]           = "File path for recording. Format is auto-detected from extension (.mp4, .mkv, .opus, .flac, .wav).";
    m_zh["feat_record_file"]           = "录制文件路径。格式从扩展名自动检测 (.mp4, .mkv, .opus, .flac, .wav)。";
    m_en["feat_record_format"]         = "Recording container format. mp4: most compatible. mkv: supports all codecs. opus/flac/wav: audio only.";
    m_zh["feat_record_format"]         = "录制容器格式。mp4：最兼容。mkv：支持所有编码器。opus/flac/wav：仅音频。";
    m_en["feat_time_limit"]            = "Recording time limit in seconds. 0 = no limit. Also works for mirroring (auto-stop after N seconds).";
    m_zh["feat_time_limit"]            = "录制时间限制 (秒)。0 = 无限制。也适用于镜像 (N 秒后自动停止)。";
    m_en["feat_no_playback"]           = "Disable both video and audio playback. Only recording. Use Ctrl+C to stop.";
    m_zh["feat_no_playback"]           = "禁用视频和音频回放。仅录制。使用 Ctrl+C 停止。";

    // Virtual Display features
    m_en["feat_new_display"]           = "Create a virtual display. Format: WIDTHxHEIGHT or WIDTHxHEIGHT/DPI. Empty = use device main display size.";
    m_zh["feat_new_display"]           = "创建虚拟显示器。格式：宽x高 或 宽x高/DPI。空 = 使用设备主显示器尺寸。";
    m_en["feat_start_app"]             = "Package name of the app to launch on the virtual display.";
    m_zh["feat_start_app"]             = "要在虚拟显示器上启动的应用包名。";
    m_en["feat_flex_display"]          = "Auto-resize the virtual display when you drag the window edges. Provides a native window experience.";
    m_zh["feat_flex_display"]          = "拖拽窗口边缘时自动调整虚拟显示器大小。提供原生窗口体验。";
    m_en["feat_no_vd_system_decos"]    = "Hide Android status bar and navigation bar on the virtual display.";
    m_zh["feat_no_vd_system_decos"]    = "隐藏虚拟显示器上的 Android 状态栏和导航栏。";
    m_en["feat_no_vd_destroy_content"] = "When closing the virtual display, move apps to main display instead of destroying them.";
    m_zh["feat_no_vd_destroy_content"] = "关闭虚拟显示器时，将应用移到主显示器而不是销毁它们。";

    // Camera features
    m_en["feat_video_source"]          = "Video source. display: mirror device screen (default). camera: use device camera.";
    m_zh["feat_video_source"]          = "视频源。display：镜像设备屏幕 (默认)。camera：使用设备相机。";
    m_en["feat_camera_id"]             = "Camera ID number. Use --list-cameras to see available cameras.";
    m_zh["feat_camera_id"]             = "相机 ID 编号。使用 --list-cameras 查看可用相机。";
    m_en["feat_camera_facing"]         = "Camera facing direction. front: selfie camera. back: main camera. external: USB camera.";
    m_zh["feat_camera_facing"]         = "相机朝向。front：前置摄像头。back：后置摄像头。external：外置摄像头。";
    m_en["feat_camera_size"]           = "Camera capture size in pixels (WIDTHxHEIGHT). Empty = auto-select based on max-size.";
    m_zh["feat_camera_size"]           = "相机采集尺寸 (像素)，格式：宽x高。空 = 根据最大尺寸自动选择。";
    m_en["feat_camera_fps"]            = "Camera frame rate. 0 = device default (usually 30). Higher values may require specific hardware.";
    m_zh["feat_camera_fps"]            = "相机帧率。0 = 设备默认 (通常 30)。更高值可能需要特定硬件。";
    m_en["feat_camera_torch"]          = "Enable camera flash/torch. Can be toggled with keyboard shortcuts during mirroring.";
    m_zh["feat_camera_torch"]          = "启用相机闪光灯。镜像期间可通过键盘快捷键切换。";

    // Advanced features
    m_en["feat_otg"]                   = "OTG mode: use device as USB input device without screen mirroring. Requires WinUSB driver on Windows.";
    m_zh["feat_otg"]                   = "OTG 模式：将设备用作 USB 输入设备，无屏幕镜像。Windows 上需要 WinUSB 驱动。";
    m_en["feat_no_window"]             = "Run without a window. Useful for background recording or audio-only mode.";
    m_zh["feat_no_window"]             = "无窗口运行。适用于后台录制或纯音频模式。";
    m_en["feat_always_on_top"]         = "Keep the scrcpy window always on top of other windows.";
    m_zh["feat_always_on_top"]         = "保持 scrcpy 窗口始终在其他窗口之上。";
    m_en["feat_fullscreen"]            = "Start in fullscreen mode. Toggle with F11 or MOD+F during mirroring.";
    m_zh["feat_fullscreen"]            = "以全屏模式启动。镜像期间按 F11 或 MOD+F 切换。";
    m_en["feat_window_borderless"]     = "Remove window title bar and borders.";
    m_zh["feat_window_borderless"]     = "移除窗口标题栏和边框。";
    m_en["feat_disable_screensaver"]   = "Prevent the computer's screensaver from activating while scrcpy is running.";
    m_zh["feat_disable_screensaver"]   = "运行 scrcpy 时禁止电脑屏保激活。";
    m_en["feat_screen_off_timeout"]    = "Device screen auto-off timeout in seconds while scrcpy is running. 0 = don't change.";
    m_zh["feat_screen_off_timeout"]    = "运行 scrcpy 时设备屏幕自动关闭超时 (秒)。0 = 不更改。";
}
