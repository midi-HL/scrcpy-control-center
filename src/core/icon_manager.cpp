#include "core/icon_manager.h"
#include "core/adb_manager.h"
#include "core/process_manager.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <direct.h>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using json = nlohmann::json;

// ── Minimal ZIP reader for APK icon extraction ───────────────────────────────
// APK files are ZIP archives. This reader extracts PNG icon entries.

namespace {

// ZIP end of central directory record
struct ZipEndRecord {
    uint32_t signature;       // 0x06054b50
    uint16_t diskNum;
    uint16_t centralDirDisk;
    uint16_t numEntriesDisk;
    uint16_t numEntries;
    uint32_t centralDirSize;
    uint32_t centralDirOffset;
    uint16_t commentLen;
};

// ZIP central directory file header
struct ZipFileHeader {
    uint32_t signature;       // 0x02014b50
    uint16_t versionMade;
    uint16_t versionNeeded;
    uint16_t flags;
    uint16_t compression;
    uint16_t modTime;
    uint16_t modDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t nameLen;
    uint16_t extraLen;
    uint16_t commentLen;
    uint16_t diskStart;
    uint16_t internalAttr;
    uint32_t externalAttr;
    uint32_t localHeaderOffset;
};

// ZIP local file header
struct ZipLocalHeader {
    uint32_t signature;       // 0x04034b50
    uint16_t versionNeeded;
    uint16_t flags;
    uint16_t compression;
    uint16_t modTime;
    uint16_t modDate;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t nameLen;
    uint16_t extraLen;
};

// Find icon PNG entries in an APK
bool extractIconFromApk(const std::string& apkPath, const std::string& outputPath) {
    std::ifstream file(apkPath, std::ios::binary);
    if (!file.is_open()) return false;

    // Read file into memory (for simplicity; APKs can be large but we only read headers)
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Find end of central directory (search from end)
    if (fileSize < sizeof(ZipEndRecord)) return false;

    std::vector<char> eocdBuf(65536 + sizeof(ZipEndRecord));
    size_t searchStart = (fileSize > 65536 + sizeof(ZipEndRecord))
                         ? fileSize - 65536 - sizeof(ZipEndRecord) : 0;
    file.seekg(searchStart);
    file.read(eocdBuf.data(), eocdBuf.size());
    size_t bytesRead = file.gcount();

    // Search for EOCD signature
    ZipEndRecord eocd = {};
    bool foundEocd = false;
    for (size_t i = bytesRead; i >= sizeof(ZipEndRecord); --i) {
        uint32_t sig;
        memcpy(&sig, eocdBuf.data() + i - sizeof(ZipEndRecord), 4);
        if (sig == 0x06054b50) {
            memcpy(&eocd, eocdBuf.data() + i - sizeof(ZipEndRecord), sizeof(ZipEndRecord));
            foundEocd = true;
            break;
        }
    }

    if (!foundEocd) return false;

    // Read central directory
    file.seekg(eocd.centralDirOffset);

    std::string bestIconName;
    uint32_t bestIconOffset = 0;
    uint32_t bestIconSize = 0;

    for (uint16_t i = 0; i < eocd.numEntries; ++i) {
        ZipFileHeader fh;
        file.read(reinterpret_cast<char*>(&fh), sizeof(fh));
        if (fh.signature != 0x02014b50) break;

        std::string name(fh.nameLen, '\0');
        file.read(&name[0], fh.nameLen);
        file.seekg(fh.extraLen + fh.commentLen, std::ios::cur);

        // Look for launcher icon PNG
        bool isPng = (name.size() > 4 &&
                      name.substr(name.size() - 4) == ".png");
        bool isIcon = (name.find("ic_launcher") != std::string::npos ||
                       name.find("ic_launcher_round") != std::string::npos);
        bool isMipmap = (name.find("mipmap") != std::string::npos);

        if (isPng && isIcon && isMipmap) {
            // Prefer higher density (xxxhdpi > xxhdpi > xhdpi > hdpi > mdpi)
            int density = 0;
            if (name.find("xxxhdpi") != std::string::npos) density = 5;
            else if (name.find("xxhdpi") != std::string::npos) density = 4;
            else if (name.find("xhdpi") != std::string::npos) density = 3;
            else if (name.find("hdpi") != std::string::npos) density = 2;
            else if (name.find("mdpi") != std::string::npos) density = 1;

            // Skip round icons (prefer regular)
            if (name.find("_round") != std::string::npos) density -= 1;

            if (density > 0) {
                // Check if this is better than current best
                // We use the uncompressed size as a proxy for quality
                if (fh.uncompressedSize > bestIconSize) {
                    bestIconName = name;
                    bestIconOffset = fh.localHeaderOffset;
                    bestIconSize = fh.uncompressedSize;
                }
            }
        }
    }

    if (bestIconName.empty()) return false;

    // Read local file header and extract data
    file.seekg(bestIconOffset);
    ZipLocalHeader lh;
    file.read(reinterpret_cast<char*>(&lh), sizeof(lh));
    if (lh.signature != 0x04034b50) return false;

    file.seekg(lh.nameLen + lh.extraLen, std::ios::cur);

    // Only store uncompressed (method 0) icons for simplicity
    if (lh.compression != 0) {
        // For compressed icons, we'd need zlib. Skip for now.
        // Try to find an uncompressed one instead.
        return false;
    }

    std::vector<char> iconData(lh.uncompressedSize);
    file.read(iconData.data(), lh.uncompressedSize);

    // Save to output
    std::ofstream out(outputPath, std::ios::binary);
    if (!out.is_open()) return false;
    out.write(iconData.data(), lh.uncompressedSize);

    return true;
}

} // anonymous namespace

// ── IconManager implementation ───────────────────────────────────────────────

IconManager& IconManager::instance() {
    static IconManager s_instance;
    return s_instance;
}

IconManager::~IconManager() {
    // Release DX9 textures
    for (auto& [key, tex] : m_textures) {
        if (tex) tex->Release();
    }
}

bool IconManager::init(const std::string& cacheDir) {
    m_cacheDir = cacheDir;
    m_manifestPath = m_cacheDir + "/manifest.json";

    // Create cache directory
    _mkdir(m_cacheDir.c_str());

    // Load manifest
    loadManifest();

    LOG_INFO("IconManager initialized, cache: %s, %zu entries",
             m_cacheDir.c_str(), m_manifest.size());
    return true;
}

bool IconManager::isCached(const std::string& packageName) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_manifest.find(packageName) != m_manifest.end();
}

LPDIRECT3DTEXTURE9 IconManager::getIcon(const std::string& packageName) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Check memory cache first
    auto it = m_textures.find(packageName);
    if (it != m_textures.end()) {
        return it->second;
    }

    // Check if disk-cached
    auto mit = m_manifest.find(packageName);
    if (mit == m_manifest.end()) {
        return nullptr; // not cached
    }

    // Load from disk
    std::string path = m_cacheDir + "/" + mit->second;
    LPDIRECT3DTEXTURE9 tex = loadTextureFromFile(path);
    if (tex) {
        m_textures[packageName] = tex;
    }
    return tex;
}

void IconManager::requestIcon(const std::string& serial, const std::string& packageName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_requested.find(packageName) != m_requested.end()) return;
    m_requested[packageName] = true;
    m_requestQueue.push_back(packageName);
}

void IconManager::processQueue(const std::string& serial) {
    std::string pkg;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_requestQueue.empty()) return;
        pkg = m_requestQueue.front();
        m_requestQueue.erase(m_requestQueue.begin());
    }

    // Check if already cached
    if (isCached(pkg)) return;

    // Extract icon in background
    extractIcon(serial, pkg);
}

bool IconManager::extractIcon(const std::string& serial, const std::string& packageName) {
    // Get APK path
    std::string pmOutput = AdbManager::instance().execute(
        {"-s", serial, "shell", "pm", "path", packageName});

    // Parse: "package:/data/app/xxx/base.apk"
    std::string apkPath;
    size_t pos = pmOutput.find("package:");
    if (pos != std::string::npos) {
        apkPath = pmOutput.substr(pos + 8);
        while (!apkPath.empty() && (apkPath.back() == '\r' || apkPath.back() == '\n')) {
            apkPath.pop_back();
        }
    }

    if (apkPath.empty()) {
        LOG_WARN("Could not find APK path for %s", packageName.c_str());
        return false;
    }

    // Pull APK to temp file
    std::string tempApk = m_cacheDir + "/temp_" + packageName + ".apk";
    std::string pullResult = AdbManager::instance().execute(
        {"-s", serial, "pull", apkPath, tempApk}, 30000);

    if (pullResult.find("error") != std::string::npos &&
        pullResult.find("1 file pulled") == std::string::npos) {
        LOG_WARN("Failed to pull APK for %s: %s", packageName.c_str(), pullResult.c_str());
        return false;
    }

    // Extract icon from APK
    std::string iconFilename = packageName + ".png";
    std::string iconPath = m_cacheDir + "/" + iconFilename;

    bool ok = extractIconFromApk(tempApk, iconPath);

    // Clean up temp APK
    remove(tempApk.c_str());

    if (ok) {
        // Update manifest
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_manifest[packageName] = iconFilename;
        }
        saveManifest();
        LOG_INFO("Icon extracted for %s", packageName.c_str());
        return true;
    }

    LOG_WARN("Could not extract icon for %s (compressed or not found)", packageName.c_str());
    return false;
}

LPDIRECT3DTEXTURE9 IconManager::loadTextureFromFile(const std::string& path) {
    // Use stb_image to load PNG
    int width, height, channels;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4); // force RGBA
    if (!pixels) {
        return nullptr;
    }

    // We need the D3D device to create a texture. We'll get it from a global or static.
    // For now, return nullptr and let the rendering code handle it.
    // In a real implementation, we'd pass the device pointer.

    // Actually, we can create the texture here if we have the device.
    // The device is created in App::initDirectX(). We need to make it accessible.
    // For now, we'll store the raw pixel data and create textures on demand.

    stbi_image_free(pixels);
    return nullptr; // TODO: create DX9 texture when device is available
}

void IconManager::saveManifest() {
    json root = json::array();
    for (const auto& [pkg, filename] : m_manifest) {
        root.push_back({{"package", pkg}, {"icon", filename}});
    }

    std::ofstream file(m_manifestPath);
    if (file.is_open()) {
        file << root.dump(2);
    }
}

void IconManager::loadManifest() {
    std::ifstream file(m_manifestPath);
    if (!file.is_open()) return;

    try {
        json root = json::parse(file);
        if (root.is_array()) {
            for (const auto& entry : root) {
                std::string pkg = entry.value("package", "");
                std::string icon = entry.value("icon", "");
                if (!pkg.empty() && !icon.empty()) {
                    m_manifest[pkg] = icon;
                }
            }
        }
    } catch (const json::exception& e) {
        LOG_WARN("Failed to parse icon manifest: %s", e.what());
    }
}
