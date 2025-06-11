//
// Created by xabdomo on 6/11/25.
//

#include "logger.hpp"
#include <imgui.h>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <sstream>

static std::map<Logger::Level, std::string> LevelsNames;
static std::vector<Logger::Level> types{};
static bool autoScroll = true;
static bool sortByTime = false;

static std::vector<Logger::Entry> entries{};

void Logger::init() {
    LevelsNames = {
        {Level::INFO, "Info"},
        {Level::ERROR, "Error"},
        {Level::WARNING, "Warning"},
        {Level::MESSAGE, "Message"}
    };

    types = { Level::INFO, Level::ERROR, Level::WARNING, Level::MESSAGE };
}

void Logger::log(const std::string& message, Level level, ImVec4 color, time_t time) {
    entries.push_back({ message, level, color, time });
}

void Logger::log(const std::string& message, Level level) {
    ImVec4 color;
    switch (level) {
        case INFO:    color = ImVec4(0.4f, 0.8f, 1.0f, 1.0f); break;
        case WARNING: color = ImVec4(1.0f, 0.78f, 0.25f, 1.0f); break;
        case ERROR:   color = ImVec4(1.0f, 0.25f, 0.25f, 1.0f); break;
        case MESSAGE: color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); break;
        default:      color = ImVec4(1, 1, 1, 1); break;
    }

    time_t now = std::chrono::system_clock::now();
    log(message, level, color, now);
}

void Logger::info(const std::string& message) {
    log(message, INFO);
}

void Logger::warning(const std::string& message) {
    log(message, WARNING);
}

void Logger::error(const std::string& message) {
    log(message, ERROR);
}

void Logger::message(const std::string& message) {
    log(message, MESSAGE);
}

void Logger::clear() {
    entries.clear();
}

void Logger::setAutoScroll(bool value) {
    autoScroll = value;
}

std::vector<Logger::Level>& Logger::shownTypes() {
    return types;
}

void Logger::render() {
    ImGui::Begin("Event Log");

    if (ImGui::BeginPopupContextItem("EventLogContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
        ImGui::TextDisabled("Filter by Type:");
        for (auto& [level, name] : LevelsNames) {
            bool selected = std::find(types.begin(), types.end(), level) != types.end();
            if (ImGui::MenuItem(name.c_str(), nullptr, selected)) {
                if (selected)
                    types.erase(std::remove(types.begin(), types.end(), level), types.end());
                else
                    types.push_back(level);
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("Options:");
        ImGui::MenuItem("Auto Scroll", nullptr, &autoScroll);
        ImGui::MenuItem("Sort by Time", nullptr, &sortByTime);

        ImGui::Separator();
        if (ImGui::MenuItem("Clear Log")) {
            Logger::clear();
        }

        ImGui::EndPopup();
    }


    std::vector<Entry> filteredEntries;
    for (const auto& entry : entries) {
        if (std::find(types.begin(), types.end(), entry.level) != types.end()) {
            filteredEntries.push_back(entry);
        }
    }

    if (sortByTime) {
        std::sort(filteredEntries.begin(), filteredEntries.end(), [](const Entry& a, const Entry& b) {
            return a.time < b.time;
        });
    }

    ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& entry : filteredEntries) {
        std::time_t time = std::chrono::system_clock::to_time_t(entry.time);
        std::tm* tm = std::localtime(&time);
        std::stringstream ss;
        ss << "[" << std::put_time(tm, "%H:%M:%S") << "] ";

        ImGui::PushStyleColor(ImGuiCol_Text, entry.color);
        ImGui::TextWrapped("%s[%s] %s", ss.str().c_str(), LevelsNames[entry.level].c_str(), entry.message.c_str());
        ImGui::PopStyleColor();
    }

    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}
