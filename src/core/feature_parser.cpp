#include "core/feature_parser.h"
#include "core/state_manager.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

FeatureParser& FeatureParser::instance() {
    static FeatureParser s_instance;
    return s_instance;
}

bool FeatureParser::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open feature mapping file: %s", path.c_str());
        return false;
    }

    try {
        json root = json::parse(file);
        m_targetScrcpyVersion = root.value("target_scrcpy", "unknown");
        m_features.clear();
        m_idIndex.clear();

        if (!root.contains("features") || !root["features"].is_array()) {
            LOG_ERROR("Invalid feature mapping: missing 'features' array");
            return false;
        }

        for (const auto& item : root["features"]) {
            FeatureDef def;
            def.id           = item.value("id", "");
            def.gui_label_en = item.value("gui_label_en", "");
            def.gui_label_zh = item.value("gui_label_zh", def.gui_label_en);
            def.category     = item.value("category", "");
            def.parameter    = item.value("parameter", "");
            def.type         = item.value("type", "string");
            def.default_val  = item.value("default", "");
            def.min_val      = item.value("min", 0);
            def.max_val      = item.value("max", 0);

            if (item.contains("options") && item["options"].is_array()) {
                for (const auto& opt : item["options"]) {
                    def.enum_options.push_back(opt.get<std::string>());
                }
            }

            // Parse dependency fields
            if (item.contains("disables") && item["disables"].is_array()) {
                for (const auto& d : item["disables"]) {
                    def.disables.push_back(d.get<std::string>());
                }
            }
            if (item.contains("disabled_by") && item["disabled_by"].is_array()) {
                for (const auto& d : item["disabled_by"]) {
                    def.disabled_by.push_back(d.get<std::string>());
                }
            }

            if (!def.id.empty()) {
                m_idIndex[def.id] = m_features.size();
                m_features.push_back(std::move(def));
            }
        }

        LOG_INFO("Loaded %zu features from %s (target scrcpy %s)",
                 m_features.size(), path.c_str(), m_targetScrcpyVersion.c_str());
        return true;

    } catch (const json::exception& e) {
        LOG_ERROR("JSON parse error in %s: %s", path.c_str(), e.what());
        return false;
    }
}

const FeatureDef* FeatureParser::getFeatureById(const std::string& id) const {
    auto it = m_idIndex.find(id);
    if (it != m_idIndex.end()) {
        return &m_features[it->second];
    }
    return nullptr;
}

std::vector<FeatureDef> FeatureParser::getFeaturesByCategory(const std::string& category) const {
    std::vector<FeatureDef> result;
    for (const auto& f : m_features) {
        if (f.category == category) {
            result.push_back(f);
        }
    }
    return result;
}

std::vector<std::string> FeatureParser::getCategories() const {
    std::vector<std::string> cats;
    std::unordered_map<std::string, bool> seen;
    for (const auto& f : m_features) {
        if (!seen[f.category]) {
            cats.push_back(f.category);
            seen[f.category] = true;
        }
    }
    return cats;
}

bool FeatureParser::isFeatureDisabled(const std::string& featureId) const {
    const FeatureDef* feature = getFeatureById(featureId);
    if (!feature) return false;

    // Check if any feature in disabled_by list is currently active (true/non-default)
    auto& state = StateManager::instance();
    for (const auto& depId : feature->disabled_by) {
        const FeatureDef* dep = getFeatureById(depId);
        if (!dep) continue;

        if (dep->type == "boolean") {
            // If the dependency is a boolean and it's true, this feature is disabled
            if (state.getBool(depId, false)) {
                return true;
            }
        } else if (dep->type == "int") {
            // If the dependency is an int and it's non-zero, this feature is disabled
            if (state.getInt(depId, 0) != 0) {
                return true;
            }
        } else if (dep->type == "string" || dep->type == "enum") {
            // If the dependency has a non-empty non-default value, this feature is disabled
            std::string val = state.getString(depId, "");
            if (!val.empty() && val != dep->default_val) {
                return true;
            }
        }
    }

    return false;
}
