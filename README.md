# Scrcpy Control Center

![License](https://img.shields.io/badge/License-MIT-blue.svg)
![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-orange.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%208.1%20x86-green.svg)
![Build](https://img.shields.io/github/actions/workflow/status/user/scrcpy-control-center/build.yml?branch=master)

A lightweight, high-performance scrcpy GUI wrapper optimized for Windows 8.1 32-bit (x86). Built with Dear ImGui and DirectX 9 for minimal resource usage.

## Overview

Scrcpy Control Center is **not** a reimplementation of scrcpy. It is an advanced GUI wrapper that manages scrcpy processes, provides a data-driven parameter configuration engine, and adds device management, virtual display workflows, and app freezing capabilities — all while maintaining near-zero CPU and memory overhead.

## Features

- **46-Parameter Data-Driven UI** — All scrcpy options are defined in a JSON mapping file. Adding new parameters requires zero code changes.
- **Multi-Device Management** — Auto-discovery of USB/WiFi devices with automatic serial injection for multi-device scenarios.
- **Virtual Display Workflows** — Create Android virtual displays, launch apps on them, and auto-resize with `--flex-display`.
- **App Freezer** — Freeze/unfreeze apps via `pm disable-user` without Root. Launch frozen apps on virtual displays.
- **Parameter Dependencies** — OTG mode automatically disables audio/video controls. `--no-audio` disables audio bitrate controls.
- **Profile System** — Switch between Gaming, Streaming, and Battery Saver presets instantly.
- **Command Reverse-Parsing** — Paste any scrcpy command string to automatically configure all UI controls.
- **Dual-Language Support** — English and Chinese UI with runtime switching.
- **GBK/UTF-8 Encoding** — Proper handling of Chinese Windows codepage issues in adb/scrcpy output.
- **Zombie Process Prevention** — All child processes are tracked and terminated on exit.

## Getting Started

### Prerequisites

- Windows 8.1 or later (32-bit or 64-bit)
- [scrcpy](https://github.com/Genymobile/scrcpy) v2.4+ installed and in PATH (or configured in Settings)
- [Android SDK Platform Tools](https://developer.android.com/studio/releases/platform-tools) (adb) in PATH

### Download

1. Download the latest release from the [Releases](../../releases) page.
2. Extract the archive to any directory.
3. Ensure `scrcpy.exe` and `adb.exe` are accessible (either in PATH or configured in Settings).

### Build from Source

Requires Visual Studio 2019/2022 with C++ Desktop workload:

```batch
git clone https://github.com/user/scrcpy-control-center.git
cd scrcpy-control-center
build.bat
```

Or manually with CMake:

```batch
cmake -S . -B build -A Win32
cmake --build build --config Release
```

### Basic Usage

1. Connect an Android device with USB debugging enabled.
2. Launch `ScrcpyControlCenter.exe`.
3. The Device tab will auto-detect your device.
4. Configure options in the Display/Audio/Control tabs.
5. Click **Start scrcpy** at the bottom.

## Architecture

```
+---------------------+     +-------------------+
|   ImGui Layer       |<--->|  State Manager    |
|   (UI Rendering)    |     |  (Parameter Values)|
+---------------------+     +-------------------+
         |                           |
         v                           v
+---------------------+     +-------------------+
|  Feature Parser     |     |  Command Builder  |
|  (JSON -> Features) |---->|  (State -> Args)  |
+---------------------+     +-------------------+
                                    |
                                    v
+---------------------+     +-------------------+
|  Process Manager    |<--->|  scrcpy.exe       |
|  (CreateProcess)    |     |  adb.exe          |
+---------------------+     +-------------------+
```

### Key Principle

The GUI **never** hardcodes scrcpy parameters. All parameter-to-UI mapping is driven by `config/scrcpy_features.json`. To add support for a new scrcpy parameter:

1. Add an entry to `scrcpy_features.json`
2. Rebuild — the UI and command generation adapt automatically

## Configuration

| File | Purpose |
|------|---------|
| `config/scrcpy_features.json` | Parameter mapping (UI + command generation) |
| `config/settings.json` | Global settings (language, theme, paths) |
| `config/profiles/*.json` | Scene presets (gaming, streaming, battery_saver) |
| `config/last_state.json` | Auto-saved state (restored on startup) |

## Advanced Usage

### OTG Mode

Enable OTG mode to use your Android device as a USB input device (keyboard/mouse) without screen mirroring. Requires WinUSB driver (install via [Zadig](https://zadig.akeo.ie/) on Windows 8.1).

### Virtual Displays

Create isolated Android virtual displays and run apps in them:

1. Go to Virtual Display tab
2. Enter display size (e.g., `1920x1080`)
3. Optionally enter an app package name
4. Click **Create Virtual Display**

Or from App Manager: click **Launch on VD** next to any app.

### Command Reverse-Parsing

Paste a scrcpy command in the Advanced tab to auto-configure all controls:

```
scrcpy --max-fps 30 --video-bit-rate 4M --no-audio --turn-screen-off
```

## License

[MIT](LICENSE)
