#include "utils/logger.h"
#include "imgui.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

Logger& Logger::instance() {
    static Logger s_instance;
    return s_instance;
}

const char* Logger::levelTag(Level level) const {
    switch (level) {
        case Level::Info:    return "[INFO]";
        case Level::Warning: return "[WARN]";
        case Level::Error:   return "[ERROR]";
        case Level::Debug:   return "[DEBUG]";
        default:             return "[????]";
    }
}

void Logger::log(Level level, const char* fmt, ...) {
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    char line[2100];
    snprintf(line, sizeof(line), "%s %s", levelTag(level), buf);

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_lines.size() >= MAX_LINES) {
        m_lines.pop_front();
    }
    m_lines.push_back(line);
}

void Logger::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lines.clear();
}

void Logger::renderPanel() {
    ImGui::Text("Logs");
    ImGui::Separator();

    // Filter input
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##log_filter", "Filter logs...", m_filterBuf, sizeof(m_filterBuf));

    if (ImGui::Button("Clear")) clear();
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
    ImGui::Separator();

    // Log content area
    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& line : m_lines) {
        // Apply filter
        if (m_filterBuf[0] != '\0' && strstr(line.c_str(), m_filterBuf) == nullptr) {
            continue;
        }

        // Color by level
        ImVec4 color;
        if (strstr(line.c_str(), "[ERROR]")) {
            color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
        } else if (strstr(line.c_str(), "[WARN]")) {
            color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
        } else if (strstr(line.c_str(), "[DEBUG]")) {
            color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        } else {
            color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(line.c_str());
        ImGui::PopStyleColor();
    }

    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}
