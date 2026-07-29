#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <d3d9.h>

// Manages app icon extraction, caching, and rendering.
// Icons are extracted from APK via adb, cached locally as PNG files.
// A JSON manifest maps package names to cached icon paths.
class IconManager {
public:
    static IconManager& instance();

    // Initialize cache directory and load manifest
    bool init(const std::string& cacheDir = "cache/icons");

    // Get icon as DX9 texture (returns nullptr if not loaded yet)
    LPDIRECT3DTEXTURE9 getIcon(const std::string& packageName);

    // Request icon extraction from device (non-blocking, queues the request)
    void requestIcon(const std::string& serial, const std::string& packageName);

    // Process pending icon requests (call from main thread periodically)
    void processQueue(const std::string& serial);

    // Check if icon is cached on disk
    bool isCached(const std::string& packageName) const;

    // Load a cached PNG into DX9 texture
    LPDIRECT3DTEXTURE9 loadTextureFromFile(const std::string& path);

    // Get cache directory
    const std::string& getCacheDir() const { return m_cacheDir; }

private:
    IconManager() = default;
    ~IconManager();
    IconManager(const IconManager&) = delete;
    IconManager& operator=(const IconManager&) = delete;

    // Extract icon from device APK
    bool extractIcon(const std::string& serial, const std::string& packageName);

    // Save manifest to disk
    void saveManifest();
    void loadManifest();

    std::string m_cacheDir = "cache/icons";
    std::string m_manifestPath;

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, LPDIRECT3DTEXTURE9> m_textures;
    std::unordered_map<std::string, std::string> m_manifest; // pkg -> icon filename
    std::vector<std::string> m_requestQueue;
    std::unordered_map<std::string, bool> m_requested; // dedup
};
