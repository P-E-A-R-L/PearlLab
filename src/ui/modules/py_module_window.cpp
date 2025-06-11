//
// Created by xabdomo on 6/11/25.
//

#include "py_module_window.hpp"

#include <imgui.h>

#include "../../backend/py_scope.hpp"

namespace PyModuleWindow {
    std::vector<py::object> modules{};
    
    void init() {

    }

    void render() {
        ImGui::Begin("Python Modules", nullptr, ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto& obj : modules) {
            try {
                std::string className = py::str(obj.attr("__name__"));
                std::string headerId = "##" + className; // Unique ID without repeating label

                // Collapsing header: toggle open/close
                bool open = ImGui::CollapsingHeader((className + headerId).c_str(),
                                                     ImGuiTreeNodeFlags_DefaultOpen);

                if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Click to expand for details");
                    ImGui::EndTooltip();
                }

                if (open) {
                    py::dict dict = obj.attr("__annotations__");

                    for (const auto& [key, value] : dict) {
                        std::string attrName = py::str(key);

                        ImGui::BulletText("%s", attrName.c_str());

                        if (py::hasattr(value, "typ")) {
                            try {
                                auto param = PyScope::parseParamFromAnnotation(value);

                                ImGui::Indent();
                                ImGui::Text("Type: %s", ParamTypeAsString(param.type).c_str());
                                ImGui::Text("Editable: %s", param.editable ? "Yes" : "No");
                                if (param.rangeStart != "None" && param.rangeEnd != "None")
                                    ImGui::Text("Range: (%s, %s)", param.rangeStart.c_str(), param.rangeEnd.c_str());
                                if (param.type == STRING) ImGui::Text("Is File Path: %s", param.isFilePath ? "Yes" : "No");

                                if (param.hasChoices) {
                                    std::string choices = "[";
                                    for (size_t i = 0; i < param.choices.size(); ++i) {
                                        choices += "'" + param.choices[i] + "'";
                                        if (i + 1 < param.choices.size()) choices += ", ";
                                    }
                                    choices += "]";
                                    ImGui::Text("Choices: %s", choices.c_str());
                                }

                                if (param.defaultValue != "None")
                                    ImGui::Text("Default: %s", param.defaultValue.c_str());

                                ImGui::Unindent();
                            } catch (...) {
                                ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "  Failed to resolve Param info");
                            }
                        } else {
                            std::string val = py::str(value);
                            ImGui::Text("  Value: %s", val.c_str());
                        }

                        ImGui::Spacing(); // slight space between attributes
                    }
                }

            } catch (const std::exception& e) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error parsing module: %s", e.what());
            }
        }


        ImGui::End();
    }

    void destroy() {
        modules.clear();
    }
}

