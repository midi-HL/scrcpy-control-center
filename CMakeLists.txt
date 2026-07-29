cmake_minimum_required(VERSION 3.15)
project(ScrcpyControlCenter LANGUAGES CXX)

# ── C++17 Standard ───────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Force x86 (Win32) Architecture ───────────────────────────────────────────
# This is critical for Windows 8.1 32-bit compatibility.
if(NOT CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
    set(CMAKE_GENERATOR_PLATFORM "Win32" CACHE STRING "Target platform" FORCE)
endif()

# ── Windows 8.1 SDK Targeting ────────────────────────────────────────────────
# Ensure we don't use APIs newer than Win8.1
add_definitions(-DWINVER=0x0603 -D_WIN32_WINNT=0x0603 -DNTDDI_VERSION=0x06030000)

# ── ImGui Sources ────────────────────────────────────────────────────────────
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx9.cpp
)

# ── Application Sources ──────────────────────────────────────────────────────
set(APP_SOURCES
    src/main.cpp
    src/app.cpp
    src/ui/sidebar.cpp
    src/ui/content_panel.cpp
    src/core/state_manager.cpp
    src/core/command_builder.cpp
    src/core/process_manager.cpp
    src/core/adb_manager.cpp
    src/core/config_manager.cpp
    src/core/feature_parser.cpp
    src/core/device_manager.cpp
    src/core/app_manager.cpp
    src/utils/logger.cpp
    src/utils/i18n.cpp
)

# ── Executable ───────────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME} WIN32 ${APP_SOURCES} ${IMGUI_SOURCES})

# ── Include Directories ──────────────────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/src
)

# ── Link Libraries ───────────────────────────────────────────────────────────
# DirectX 9 is part of the Windows SDK, always available on Win8.1
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d9
    dwmapi
    winmm
    imm32
)

# ── Compiler Options ─────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3                 # Warning level 3
        /MP                 # Multi-processor compilation
        /Gy                 # Function-level linking (smaller binary)
        /Oi                 # Enable intrinsic functions
        /EHsc               # Standard C++ exception handling
        /D_CRT_SECURE_NO_WARNINGS
        /DIMGUI_DEFINE_MATH_OPERATORS
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        /SUBSYSTEM:WINDOWS
        /LTCG               # Link-time code generation (smaller/faster)
        /OPT:REF            # Remove unreferenced functions
        /OPT:ICF            # COMDAT folding
    )
else()
    # MinGW fallback
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -O2
        -DIMGUI_DEFINE_MATH_OPERATORS
        -D_CRT_SECURE_NO_WARNINGS
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        -mwindows
        -static
    )
endif()

# ── Copy config directory to build output ────────────────────────────────────
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/config
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/config
    COMMENT "Copying config directory to output..."
)
