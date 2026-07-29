#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// Feature definition loaded from scrcpy_features.json
// This is the core data structure that drives the entire GUI
struct FeatureDef {
    std::string id;              // unique identifier (e.g., "max_size")
    std::string gui_label_en;    // English label for UI
    std::string gui_label_zh;    // Chinese label for UI
    std::string category;        // sidebar category (e.g., "display", "audio")
    std::string parameter;       // scrcpy CLI parameter (e.g., "--max-size")
    std::string type;            // "int", "string", "boolean", "boolean_inverse", "enum"
    std::string default_val;     // default value as string
    std::vector<std::string> enum_options; // for enum type
    int min_val = 0;             // for int type
    int max_val = 0;             // for int type

    // Dependency system: when THIS feature is active, it disables others
    std::vector<std::string> disables;
    // When ANY of these features are active, THIS feature is disabled
    std::vector<std::string> disabled_by;
};

// Parses scrcpy_features.json and provides feature definitions
class FeatureParser {
public:
    static FeatureParser& instance();

    // Load features from JSON file
    bool loadFromFile(const std::string& path);

    // Access features
    const std::vector<FeatureDef>& getFeatures() const { return m_features; }
    const FeatureDef* getFeatureById(const std::string& id) const;

    // Get features filtered by category
    std::vector<FeatureDef> getFeaturesByCategory(const std::string& category) const;

    // Get all unique categories
    std::vector<std::string> getCategories() const;

    // Check if a feature is currently disabled by dependencies
    bool isFeatureDisabled(const std::string& featureId) const;

    // Get target scrcpy version
    const std::string& getTargetScrcpyVersion() const { return m_targetScrcpyVersion; }

private:
    FeatureParser() = default;
    ~FeatureParser() = default;
    FeatureParser(const FeatureParser&) = delete;
    FeatureParser& operator=(const FeatureParser&) = delete;

    std::vector<FeatureDef> m_features;
    std::unordered_map<std::string, size_t> m_idIndex; // id -> index in m_features
    std::string m_targetScrcpyVersion;
};
