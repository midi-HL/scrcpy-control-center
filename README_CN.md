# Scrcpy 控制中心

![License](https://img.shields.io/badge/License-MIT-blue.svg)
![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-orange.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%208.1%20x86-green.svg)

专为 Windows 8.1 32 位 (x86) 深度优化的轻量级 scrcpy 图形界面包装器。基于 Dear ImGui + DirectX 9 构建，资源占用极低。

## 项目概述

Scrcpy 控制中心**不是** scrcpy 的重新实现。它是一个高级 GUI 包装器，负责管理 scrcpy 进程、提供数据驱动的参数配置引擎，并增加设备管理、虚拟显示工作流和应用冻结功能——同时保持接近零的 CPU 和内存开销。

## 核心特性

- **46 参数数据驱动 UI** — 所有 scrcpy 选项在 JSON 映射文件中定义，新增参数无需修改代码
- **多设备管理** — USB/WiFi 设备自动发现，多设备场景自动注入序列号
- **虚拟显示工作流** — 创建 Android 虚拟显示器，在其中启动应用，支持 `--flex-display` 自动缩放
- **应用冻结器** — 通过 `pm disable-user` 冻结/解冻应用（无需 Root），可在虚拟显示器中启动冻结的应用
- **参数依赖关系** — OTG 模式自动禁用音视频控件，`--no-audio` 自动禁用音频比特率控件
- **场景预设** — 一键切换游戏、串流、省电模式配置
- **命令逆向解析** — 粘贴任意 scrcpy 命令字符串，自动配置所有 UI 控件
- **中英双语** — 运行时切换中英文界面
- **GBK/UTF-8 编码处理** — 正确处理中文 Windows 上 adb/scrcpy 输出的编码问题
- **僵尸进程防护** — 退出时自动终止所有子进程

## 快速开始

### 环境要求

- Windows 8.1 或更高版本（32 位或 64 位）
- [scrcpy](https://github.com/Genymobile/scrcpy) v2.4+ 已安装并在 PATH 中（或在设置中配置路径）
- [Android SDK Platform Tools](https://developer.android.com/studio/releases/platform-tools)（adb）在 PATH 中

### 下载安装

1. 从 [Releases](../../releases) 页面下载最新发布包
2. 解压到任意目录
3. 确保 `scrcpy.exe` 和 `adb.exe` 可访问（在 PATH 中或在设置中配置路径）

### 从源码构建

需要 Visual Studio 2019/2022 及 C++ 桌面开发工作负载：

```batch
git clone https://github.com/midi-HL/scrcpy-control-center.git
cd scrcpy-control-center
build.bat
```

或手动使用 CMake：

```batch
cmake -S . -B build -A Win32
cmake --build build --config Release
```

### 基本使用

1. 连接已启用 USB 调试的 Android 设备
2. 启动 `ScrcpyControlCenter.exe`
3. 设备选项卡将自动检测到您的设备
4. 在显示/音频/控制选项卡中配置参数
5. 点击底部的 **启动 scrcpy**

## 架构设计

```
+---------------------+     +-------------------+
|   ImGui 展示层      |<--->|  状态管理器       |
|   (UI 渲染)         |     |  (参数值)         |
+---------------------+     +-------------------+
         |                           |
         v                           v
+---------------------+     +-------------------+
|  功能解析器         |     |  命令构建器       |
|  (JSON → 功能定义)  |---->|  (状态 → 参数)    |
+---------------------+     +-------------------+
                                    |
                                    v
+---------------------+     +-------------------+
|  进程管理器         |<--->|  scrcpy.exe       |
|  (CreateProcess)    |     |  adb.exe          |
+---------------------+     +-------------------+
```

### 核心原则

GUI **绝不**硬编码 scrcpy 参数。所有参数到 UI 的映射由 `config/scrcpy_features.json` 驱动。要支持新的 scrcpy 参数：

1. 在 `scrcpy_features.json` 中添加条目
2. 重新构建——UI 和命令生成自动适配

## 配置文件

| 文件 | 用途 |
|------|------|
| `config/scrcpy_features.json` | 参数映射（UI + 命令生成） |
| `config/settings.json` | 全局设置（语言、主题、路径） |
| `config/profiles/*.json` | 场景预设（游戏、串流、省电） |
| `config/last_state.json` | 自动保存的状态（启动时恢复） |

## 高级用法

### OTG 模式

启用 OTG 模式可将 Android 设备用作 USB 输入设备（键盘/鼠标），无需屏幕镜像。在 Windows 8.1 上需要 WinUSB 驱动（通过 [Zadig](https://zadig.akeo.ie/) 安装）。

### 虚拟显示器

创建隔离的 Android 虚拟显示器并在其中运行应用：

1. 进入虚拟显示选项卡
2. 输入显示尺寸（如 `1920x1080`）
3. 可选输入应用包名
4. 点击 **创建虚拟显示**

或在应用管理器中点击任意应用旁边的 **在 VD 中启动**。

### 命令逆向解析

在高级选项卡中粘贴 scrcpy 命令以自动配置所有控件：

```
scrcpy --max-fps 30 --video-bit-rate 4M --no-audio --turn-screen-off
```

## 许可证

[MIT](LICENSE)
