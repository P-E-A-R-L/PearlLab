//
// Created by xabdomo on 6/16/25.
//

#include "pipeline.hpp"

#include "logger.hpp"
#include "../font_manager.hpp"

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

        char* data = new char[size];
        size_t offset = 0;
        for (int i = 0;i < names.size();i++) {
            strcpy(data + offset, names[i].c_str());
            offset += names[i].size() + 1;
        }

        if (ImGui::Combo("##<empty>", &PipelineConfig::activeEnv, data, size)) {
            // nothing to do here
        }

        ImGui::SameLine();
        ImGui::Text("Environment");

        ImGui::InputInt("Max Steps", &PipelineConfig::maxSteps);
        ImGui::InputInt("Max Episodes", &PipelineConfig::maxEpisodes);

        ImGui::Separator();
        ImGui::TextDisabled("Agents");

        static int select_agent = 0;
        static int counter = 200;
        static int rename_agent_index = -1;

        if (PipelineConfig::activeAgents.empty()) {
            ImGui::Text("<drag agents from recipes>");
        } else {
            // simulate selection until inspector is active

            counter--;
            if (counter <= 0) {
                select_agent = (select_agent + 1) % PipelineConfig::activeAgents.size();
                counter = 200;
            }

            for (int i = 0; i < PipelineConfig::activeAgents.size(); ++i) {
                auto& agent = PipelineConfig::activeAgents[i];

                ImGui::PushID(i);

                // Handle rename input field
                if (rename_agent_index == i) {
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::InputText("##rename", agent.name, IM_ARRAYSIZE(agent.name), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        rename_agent_index = -1; // Done renaming
                    }
                } else {
                    // Selectable item
                    if (ImGui::Selectable(agent.name, select_agent == i)) {
                        select_agent = i;
                    }

                    // Right-click context menu
                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem("Rename")) {
                            rename_agent_index = i;
                        }
                        if (ImGui::MenuItem("Delete")) {
                            PipelineConfig::activeAgents.erase(PipelineConfig::activeAgents.begin() + i);
                            if (select_agent == i) select_agent = -1;
                            if (rename_agent_index == i) rename_agent_index = -1;
                            ImGui::EndPopup();
                            ImGui::PopID();
                            break; // Exit now (one frame will be corrupted, but well it is what it is).
                        }
                        ImGui::EndPopup();
                    }

                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
                        FontManager::pushFont("Light");
                        std::string py_type_name = py::str(agent.agent->module);
                        ImGui::Text(py_type_name.c_str());
                        ImGui::Text(agent.agent->moduleName.c_str());
                        FontManager::popFont();
                        ImGui::PopStyleColor();
                        ImGui::EndTooltip();
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
                    ActiveAgent activeAgent;
                    strcpy(activeAgent.name, recipe.acceptor->_tag);
                    try {
                        py::object agentObject = recipe.create();
                        if (agentObject.is_none()) {
                            Logger::error("Failed to create agent from recipe: " + std::string(recipe.acceptor->_tag));
                        } else {
                            activeAgent.agent = new PyAgent();
                            if (PyScope::parseLoadedModule(getattr(agentObject, "__class__"), *activeAgent.agent)) {
                                activeAgent.agent->object = agentObject;
                                activeAgent.env   = nullptr;
                                activeAgent.methods.clear();
                                PipelineConfig::activeAgents.push_back(activeAgent);
                                Logger::info("Added new agent");
                            } else {
                                delete activeAgent.agent;
                            }
                        }
                    } catch (...) {
                        Logger::error("Unknown error while trying to instantiating Agent");
                    }
                } else {
                    Logger::warning("Dropped a non-agent recipe, ignoring.");
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

