#pragma once
#include <string>
#include <unordered_map>
#include <variant>

// Manages the current state of all scrcpy parameters.
// This is the single source of truth for all GUI control values.
// ImGui controls bind directly to these state values.
class StateManager {
public:
    // State value can be int, bool, or string
    using StateValue = std::variant<int, bool, std::string>;

    static StateManager& instance();

    // Initialize all states from FeatureParser defaults
    void initFromDefaults();

    // Typed getters with safe defaults
    int    getInt(const std::string& id, int defaultVal = 0) const;
    bool   getBool(const std::string& id, bool defaultVal = false) const;
    std::string getString(const std::string& id, const std::string& defaultVal = "") const;

    // Mutable references for ImGui binding (ImGui needs T*)
    int*    getIntRef(const std::string& id);
    bool*   getBoolRef(const std::string& id);
    std::string* getStringRef(const std::string& id);

    // Setters
    void setInt(const std::string& id, int val);
    void setBool(const std::string& id, bool val);
    void setString(const std::string& id, const std::string& val);

    // Get all states as a map (for serialization)
    const std::unordered_map<std::string, StateValue>& getAllStates() const { return m_states; }

    // Load/Save to JSON
    bool loadFromJson(const std::string& path);
    bool saveToJson(const std::string& path) const;

    // Reset all to defaults
    void resetAll();

private:
    StateManager() = default;
    ~StateManager() = default;
    StateManager(const StateManager&) = delete;
    StateManager& operator=(const StateManager&) = delete;

    std::unordered_map<std::string, StateValue> m_states;
    std::unordered_map<std::string, int> m_intStorage;   // stable addresses for ImGui
    std::unordered_map<std::string, bool> m_boolStorage; // stable addresses for ImGui
};
