//
// Created by xabdomo on 6/16/25.
//

#include "pipeline.hpp"

#include "logger.hpp"
#include "../font_manager.hpp"
#include "imgui_internal.h"

namespace Pipeline {

    namespace PipelineConfig {
        // all variables such as
        //   policy
        //   steps
        //   maxEp
        // etc .. will be stored here

        int maxSteps    = 4000;    // default max steps for an agent
        int maxEpisodes = 4000;    // default max episodes for an agent
        int activeEnv   = 0;       // the index of the current active env
        std::vector<ActiveAgent> activeAgents {};

        std::vector<PipelineAgent>  pipelineAgents  {};
        std::vector<PipelineMethod> pipelineMethods {};
    }

    std::vector<PipelineGraph::ObjectRecipe> envs;
    std::vector<PipelineGraph::ObjectRecipe> agents;
    std::vector<PipelineGraph::ObjectRecipe> methods;

    std::vector<PipelineGraph::ObjectRecipe> recipes;

    void setRecipes(std::vector<PipelineGraph::ObjectRecipe> r) {
        destroy(); // reset everything
        for (auto& recipe: r) {
            if (recipe.type == PipelineGraph::Environment) envs   .push_back(recipe);
            if (recipe.type == PipelineGraph::Agent)       agents .push_back(recipe);
            if (recipe.type == PipelineGraph::Method)      methods.push_back(recipe);

            recipes.push_back(recipe);
        }
    }

    void init() {
        envs.clear();
        agents.clear();
        methods.clear();
    }

    static void render_recipes() {
        ImGui::Begin("Recipes");

        int visibleCount = 0;
        for (int i = 0; i < recipes.size(); ++i) {
            auto& recipe = recipes[i];

            std::string id = "recipe_" + std::to_string(i);
            ImGui::PushID(id.c_str());

            std::string typeName = "A";
            if (recipe.type == PipelineGraph::Environment) typeName = "E";
            if (recipe.type == PipelineGraph::Method)      typeName = "M";

            ImGui::Text(typeName.c_str());
            ImGui::SameLine();

            ImGui::SetCursorPos({30, ImGui::GetCursorPos().y});

            ImGui::PushID(i);
            if (strlen(recipe.acceptor->_tag) == 0) {
                ImGui::Selectable("<empty>");
            } else {
                ImGui::Selectable(recipe.acceptor->_tag);
            }
            ImGui::PopID();

            // Drag-and-drop support
            if (ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload("Recipes::Acceptor", &i, sizeof(int));
                ImGui::Text("Dragging: %s", recipe.acceptor->_tag);
                ImGui::EndDragDropSource();
            }

            ImGui::PopID();

            visibleCount++;
        }

        if (visibleCount == 0)
            ImGui::Text("No recipes (attach acceptors in Pipeline graph).");

        ImGui::End();
    }

    static void render_pipeline() {
        ImGui::Begin("Pipeline");
        ImGui::TextDisabled("Configuration");

        std::vector<std::string> names;
        for (auto recipe: envs) {
            names.push_back(recipe.acceptor->_tag);
        }

        if (names.empty()) {
            names.push_back("<empty>");
        }

        size_t size = 0;
        for (auto& name: names) {
            size += name.length() + 1;
        }

        char* data = new char[size + 1];
        size_t offset = 0;
        for (int i = 0;i < names.size();i++) {
            strcpy(data + offset, names[i].c_str());
            offset += names[i].size() + 1;
        }

        data[size] = '\0'; // Null-terminate the for list

        if (ImGui::Combo("##<empty>", &PipelineConfig::activeEnv, data, size)) {
            // nothing to do here
        }

        ImGui::SameLine();
        ImGui::Text("Environment");

        ImGui::InputInt("Max Steps", &PipelineConfig::maxSteps);
        ImGui::InputInt("Max Episodes", &PipelineConfig::maxEpisodes);

        ImGui::Separator();
        ImGui::TextDisabled("Agents");

        static int rename_agent_index = -1;

        if (PipelineConfig::pipelineAgents.empty()) {
            ImGui::Text("<drag agents from recipes>");
        } else {
            for (int i = 0; i < PipelineConfig::pipelineAgents.size(); ++i) {
                ImGui::PushID(i);
                auto& agent = PipelineConfig::pipelineAgents[i];

                // Handle rename input field
                if (rename_agent_index == i) {
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::InputText("##rename", agent.name, IM_ARRAYSIZE(agent.name), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        rename_agent_index = -1; // Done renaming
                    }
                } else {


                    ImVec2 group_start = ImGui::GetCursorScreenPos();
                    ImGui::BeginGroup();

                    ImGui::Checkbox("", &agent.active);
                    ImGui::SameLine();
                    ImGui::Text("%s", agent.name);

                    ImGui::EndGroup();

                    ImVec2 group_end = ImGui::GetItemRectMax();
                    ImVec2 g_size(group_end.x - group_start.x, group_end.y - group_start.y);
                    ImGui::SetCursorScreenPos(group_start);
                    ImGui::InvisibleButton("##context_target", g_size);

                    // Right-click context menu
                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem("Rename")) {
                            rename_agent_index = i;
                        }
                        if (ImGui::MenuItem("Delete")) {
                            PipelineConfig::activeAgents.erase(PipelineConfig::activeAgents.begin() + i);
                            if (rename_agent_index == i) rename_agent_index = -1;
                            ImGui::PopID();
                            ImGui::EndPopup();
                            break; // Exit now (one frame will be corrupted, but well it is what it is).
                        }
                        ImGui::EndPopup();
                    }

                    if (ImGui::IsItemHovered()) {
                        std::string id = "recipe_" + std::to_string(agent.recipe_index);
                        ImGui::BeginTooltip();
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
                        FontManager::pushFont("Light");
                        std::string py_type_name = py::str(agent.recipe->acceptor->_tag);
                        ImGui::Text(py_type_name.c_str());
                        FontManager::popFont();
                        ImGui::PopStyleColor();
                        ImGui::EndTooltip();

                        // fixme: find a way to draw a rect around the recipe that creates this object
                        auto mId = ImGui::GetID(id.c_str());

                    }
                }

                ImGui::PopID();
            }
        }

        ImGui::Separator();
        ImGui::TextDisabled("Methods");

        static int rename_method_index = -1;

        if (PipelineConfig::pipelineMethods.empty()) {
            ImGui::Text("<drag methods from recipes>");
        } else {
            for (int i = 0; i < PipelineConfig::pipelineMethods.size(); ++i) {
                ImGui::PushID(i + PipelineConfig::pipelineAgents.size());

                auto& method = PipelineConfig::pipelineMethods[i];

                // Handle rename input field
                if (rename_method_index == i) {
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::InputText("##rename", method.name, IM_ARRAYSIZE(method.name), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        rename_agent_index = -1; // Done renaming
                    }
                } else {
                    ImVec2 group_start = ImGui::GetCursorScreenPos();
                    ImGui::BeginGroup();

                    ImGui::Checkbox("##box", &method.active);
                    ImGui::SameLine();
                    ImGui::Text("%s", method.name);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(std::min(150, (int) ImGui::GetContentRegionAvail().x));
                    ImGui::SliderFloat("##slider", &method.weight, 0, 1, "Weight: %.2f");

                    ImGui::EndGroup();

                    ImVec2 group_end = ImGui::GetItemRectMax();
                    ImVec2 g_size(group_end.x - group_start.x, group_end.y - group_start.y);
                    ImGui::SetCursorScreenPos(group_start);
                    ImGui::InvisibleButton("##context_target", g_size);

                    // Right-click context menu
                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem("Rename")) {
                            rename_agent_index = i;
                        }
                        if (ImGui::MenuItem("Delete")) {
                            PipelineConfig::activeAgents.erase(PipelineConfig::activeAgents.begin() + i);
                            if (rename_agent_index == i) rename_agent_index = -1;
                            ImGui::EndPopup();
                            ImGui::PopID();
                            break; // Exit now (one frame will be corrupted, but well it is what it is).
                        }
                        ImGui::EndPopup();
                    }

                    if (ImGui::IsItemHovered()) {
                        std::string id = "recipe_" + std::to_string(method.recipe_index);
                        ImGui::BeginTooltip();
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
                        FontManager::pushFont("Light");
                        std::string py_type_name = py::str(method.recipe->acceptor->_tag);
                        ImGui::Text(py_type_name.c_str());
                        FontManager::popFont();
                        ImGui::PopStyleColor();
                        ImGui::EndTooltip();

                        // fixme: find a way to draw a rect around the recipe that creates this object
                        auto mId = ImGui::GetID(id.c_str());

                    }
                }

                ImGui::PopID();
            }
        }

        ImVec2 region_min = ImGui::GetWindowContentRegionMin();
        ImVec2 region_max = ImGui::GetWindowContentRegionMax();
        ImVec2 window_pos = ImGui::GetWindowPos();

        ImVec2 abs_min = ImVec2(window_pos.x + region_min.x, window_pos.y + region_min.y); // top-left
        ImVec2 abs_max = ImVec2(window_pos.x + region_max.x, window_pos.y + region_max.y); // bot-right

        ImVec2 region_size = ImVec2(region_max.x - region_min.x, region_max.y - region_min.y);

        // Create a full-window invisible button (must render something for drag-drop to bind)
        // Doesn't steal other events tho so that's nice
        ImGui::SetCursorScreenPos(abs_min);
        ImGui::InvisibleButton("##Dropdown Recipes::Acceptor", region_size);

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Recipes::Acceptor")) {
                int   index  = *static_cast<int*>(payload->Data);
                auto& recipe = recipes[index];
                if (recipe.type == PipelineGraph::Agent) {

                    PipelineAgent agent;
                    agent.recipe = &recipe;
                    agent.active = true;
                    strcpy(agent.name, recipe.acceptor->_tag);
                    agent.recipe_index = index;
                    PipelineConfig::pipelineAgents.push_back(agent);

                } else if (recipe.type == PipelineGraph::Method) {
                    PipelineMethod method;
                    method.recipe = &recipe;
                    method.active = true;
                    method.weight = 1.0;
                    strcpy(method.name, recipe.acceptor->_tag);
                    method.recipe_index = index;
                    PipelineConfig::pipelineMethods.push_back(method);
                } else {
                    Logger::warning("Drop Agents / Methods, You can select the environment from the drop down list.");
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::End();
    }

    void render() {
        render_recipes();
        render_pipeline();
    }

    void destroy() {
        recipes.clear();
        for (auto& active: PipelineConfig::activeAgents) {
            delete active.agent;
            for (auto& m: active.methods) {
                delete m;
            }
        }
    }
}

