#pragma once
#include <string>
#include <vector>

// Builds scrcpy command-line arguments from current state.
// This is the ONLY place where state values are converted to CLI parameters.
// The GUI layer must NEVER hardcode parameter generation.
class CommandBuilder {
public:
    static CommandBuilder& instance();

    // Build the full argument list for scrcpy
    std::vector<std::string> buildArgs(const std::string& serial = "") const;

    // Build a human-readable command string for preview
    std::string buildPreviewString(const std::string& scrcpyPath = "scrcpy",
                                   const std::string& serial = "") const;

    // Reverse-parse a command string back into state values
    // (e.g., user pastes "scrcpy --max-fps 30 -m 1024")
    bool parseCommandString(const std::string& cmd);

private:
    CommandBuilder() = default;
    ~CommandBuilder() = default;
    CommandBuilder(const CommandBuilder&) = delete;
    CommandBuilder& operator=(const CommandBuilder&) = delete;

    // Add a single feature's argument if it differs from default
    void addFeatureArg(std::vector<std::string>& args, const struct FeatureDef& feature) const;
};
