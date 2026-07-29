#include "core/state_manager.h"
#include "core/feature_parser.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

StateManager& StateManager::instance() {
    static StateManager s_instance;
    return s_instance;
}

void StateManager::initFromDefaults() {
    m_states.clear();
    m_intStorage.clear();
    m_boolStorage.clear();

    const auto& features = FeatureParser::instance().getFeatures();
    for (const auto& f : features) {
        if (f.type == "int") {
            int val = 0;
            try { val = std::stoi(f.default_val); } catch (...) {}
            m_intStorage[f.id] = val;
            m_states[f.id] = val;
        } else if (f.type == "boolean" || f.type == "boolean_inverse") {
            bool val = (f.default_val == "true" || f.default_val == "1");
            m_boolStorage[f.id] = val;
            m_states[f.id] = val;
        } else {
            // string, enum
            m_states[f.id] = f.default_val;
        }
    }
    LOG_INFO("State initialized with %zu parameters", m_states.size());
}

int StateManager::getInt(const std::string& id, int defaultVal) const {
    auto it = m_intStorage.find(id);
    return (it != m_intStorage.end()) ? it->second : defaultVal;
}

bool StateManager::getBool(const std::string& id, bool defaultVal) const {
    auto it = m_boolStorage.find(id);
    return (it != m_boolStorage.end()) ? it->second : defaultVal;
}

std::string StateManager::getString(const std::string& id, const std::string& defaultVal) const {
    auto it = m_states.find(id);
    if (it != m_states.end()) {
        if (const auto* s = std::get_if<std::string>(&it->second)) {
            return *s;
        }
    }
    return defaultVal;
}

int* StateManager::getIntRef(const std::string& id) {
    auto it = m_intStorage.find(id);
    if (it != m_intStorage.end()) return &it->second;
    m_intStorage[id] = 0;
    return &m_intStorage[id];
}

bool* StateManager::getBoolRef(const std::string& id) {
    auto it = m_boolStorage.find(id);
    if (it != m_boolStorage.end()) return &it->second;
    m_boolStorage[id] = false;
    return &m_boolStorage[id];
}

std::string* StateManager::getStringRef(const std::string& id) {
    auto it = m_states.find(id);
    if (it != m_states.end()) {
        if (auto* s = std::get_if<std::string>(&it->second)) {
            return s;
        }
    }
    m_states[id] = std::string("");
    return std::get_if<std::string>(&m_states[id]);
}

void StateManager::setInt(const std::string& id, int val) {
    m_intStorage[id] = val;
    m_states[id] = val;
}

void StateManager::setBool(const std::string& id, bool val) {
    m_boolStorage[id] = val;
    m_states[id] = val;
}

void StateManager::setString(const std::string& id, const std::string& val) {
    m_states[id] = val;
}

bool StateManager::loadFromJson(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    try {
        json root = json::parse(file);
        for (auto& [key, value] : root.items()) {
            if (value.is_number_integer()) {
                setInt(key, value.get<int>());
            } else if (value.is_boolean()) {
                setBool(key, value.get<bool>());
            } else if (value.is_string()) {
                setString(key, value.get<std::string>());
            }
        }
        LOG_INFO("State loaded from %s", path.c_str());
        return true;
    } catch (const json::exception& e) {
        LOG_ERROR("Failed to load state from %s: %s", path.c_str(), e.what());
        return false;
    }
}

bool StateManager::saveToJson(const std::string& path) const {
    json root;
    for (const auto& [key, val] : m_states) {
        if (const auto* i = std::get_if<int>(&val)) {
            root[key] = *i;
        } else if (const auto* b = std::get_if<bool>(&val)) {
            root[key] = *b;
        } else if (const auto* s = std::get_if<std::string>(&val)) {
            root[key] = *s;
        }
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to save state to %s", path.c_str());
        return false;
    }
    file << root.dump(2);
    LOG_INFO("State saved to %s", path.c_str());
    return true;
}

void StateManager::resetAll() {
    initFromDefaults();
}
