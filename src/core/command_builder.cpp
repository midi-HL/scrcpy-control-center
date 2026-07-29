#include "core/command_builder.h"
#include "core/feature_parser.h"
#include "core/state_manager.h"
#include "utils/logger.h"
#include <sstream>
#include <algorithm>

CommandBuilder& CommandBuilder::instance() {
    static CommandBuilder s_instance;
    return s_instance;
}

void CommandBuilder::addFeatureArg(std::vector<std::string>& args, const FeatureDef& feature) const {
    auto& state = StateManager::instance();

    if (feature.type == "boolean") {
        // Boolean: add parameter only if true (non-default)
        bool val = state.getBool(feature.id, false);
        bool def = (feature.default_val == "true" || feature.default_val == "1");
        if (val && !def) {
            args.push_back(feature.parameter);
        } else if (!val && def) {
            // Default was true, now false - some booleans are inverted
            // For now, just add when true
        }
    } else if (feature.type == "boolean_inverse") {
        // boolean_inverse: parameter is added when state is FALSE
        // e.g., no_audio: when "Disable Audio" is checked (true), add --no-audio
        bool val = state.getBool(feature.id, false);
        if (val) {
            args.push_back(feature.parameter);
        }
    } else if (feature.type == "int") {
        int val = state.getInt(feature.id, 0);
        int def = 0;
        try { def = std::stoi(feature.default_val); } catch (...) {}
        if (val != def && val != 0) {
            args.push_back(feature.parameter);
            args.push_back(std::to_string(val));
        }
    } else if (feature.type == "enum") {
        std::string val = state.getString(feature.id, feature.default_val);
        if (val != feature.default_val && !val.empty()) {
            args.push_back(feature.parameter);
            args.push_back(val);
        }
    } else if (feature.type == "string") {
        std::string val = state.getString(feature.id, "");
        std::string def = feature.default_val;
        if (!val.empty() && val != def) {
            args.push_back(feature.parameter);
            args.push_back(val);
        }
    }
}

std::vector<std::string> CommandBuilder::buildArgs(const std::string& serial) const {
    std::vector<std::string> args;

    // Add device serial if specified (critical for multi-device)
    if (!serial.empty()) {
        args.push_back("-s");
        args.push_back(serial);
    }

    // Iterate all features and add args based on state
    const auto& features = FeatureParser::instance().getFeatures();
    for (const auto& feature : features) {
        addFeatureArg(args, feature);
    }

    return args;
}

std::string CommandBuilder::buildPreviewString(const std::string& scrcpyPath,
                                                const std::string& serial) const {
    auto args = buildArgs(serial);
    std::ostringstream oss;
    oss << scrcpyPath;
    for (const auto& arg : args) {
        // Quote args with spaces
        if (arg.find(' ') != std::string::npos) {
            oss << " \"" << arg << "\"";
        } else {
            oss << " " << arg;
        }
    }
    return oss.str();
}

bool CommandBuilder::parseCommandString(const std::string& cmd) {
    // Tokenize the command string
    std::vector<std::string> tokens;
    std::istringstream iss(cmd);
    std::string token;
    bool inQuotes = false;
    char quoteChar = '\0';
    std::string current;

    for (char c : cmd) {
        if (!inQuotes && (c == '"' || c == '\'')) {
            inQuotes = true;
            quoteChar = c;
        } else if (inQuotes && c == quoteChar) {
            inQuotes = false;
            tokens.push_back(current);
            current.clear();
        } else if (!inQuotes && (c == ' ' || c == '\t')) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) tokens.push_back(current);

    // Skip "scrcpy" or path prefix
    auto& state = StateManager::instance();
    const auto& features = FeatureParser::instance().getFeatures();

    for (size_t i = 0; i < tokens.size(); ++i) {
        // Skip scrcpy binary name
        if (i == 0 && (tokens[i] == "scrcpy" || tokens[i].find("scrcpy") != std::string::npos)) {
            continue;
        }

        // Find matching feature
        for (const auto& feature : features) {
            if (tokens[i] == feature.parameter) {
                if (feature.type == "boolean" || feature.type == "boolean_inverse") {
                    state.setBool(feature.id, true);
                } else if (i + 1 < tokens.size()) {
                    // Next token is the value
                    if (feature.type == "int") {
                        try {
                            state.setInt(feature.id, std::stoi(tokens[i + 1]));
                        } catch (...) {}
                    } else {
                        state.setString(feature.id, tokens[i + 1]);
                    }
                    i++; // skip value
                }
                break;
            }
        }

        // Handle -s serial
        if (tokens[i] == "-s" && i + 1 < tokens.size()) {
            i++; // skip serial value
        }
    }

    LOG_INFO("Parsed command string: %s", cmd.c_str());
    return true;
}
