#include "inspector.hpp"

#include <mutex>
#include <thread>

#include "imgui.h"
#include "pipeline.hpp"
#include "preview.hpp"
#include "../utility/helpers.hpp"
#include "../utility/image_store.hpp"
#include "../widgets/image_text_button.hpp"
#include "../widgets/splitter.hpp"

namespace Inspector {
    struct InspectorCache {
        bool window_opened = false;
        bool play          = false;
        std::string window_name = "";
    };


}

static std::map<Pipeline::VisualizedAgent*, Inspector::InspectorCache> _cache;

void Inspector::init() {

}

void Inspector::update() {
    for (auto& [k, v]: _cache) {
        if (v.play) {
            Pipeline::stepSim(k->agent_index); // step the agent
        }
    }
}

void Inspector::onStart() {
    std::lock_guard guard(Pipeline::PipelineState::agentsLock);

    _cache.clear();

    for (auto a: Pipeline::PipelineState::activeVisualizations) {
        if (a->agent == nullptr || a->env_visualization == nullptr) {
            throw std::runtime_error("agent or evn is null in Inspector::onStart()");
        }

        _cache[a] = {
            .window_opened = false,
            .play          = false,
            .window_name   = std::string("(") + std::to_string(a->agent_index) + std::string(") ") + std::string(a->agent->name)
        };
    }
}


static auto IN_OBJ_HEADER  = ImVec4{166 / 255.0f, 25 / 255.0f, 163 / 255.0f, 255 / 255.0f};
static auto SECTION_HEADER = ImVec4{166 / 255.0f, 159 / 255.0f, 25 / 255.0f, 255 / 255.0f};
static auto SECTION_SUB_HEADER = ImVec4{31 / 255.0f, 171 / 255.0f, 73 / 255.0f, 255 / 255.0f};

static void _render_python_object(PyLiveObject* obj) {
    std::lock_guard guard(*obj->m_lock);

    for (int i = 0; i < obj->annotations.size(); ++i) {
        auto& anno    = obj->annotations[i];
        auto& pub = obj->public_members_values[i];

        auto editable = anno.editable && pub.type != OTHER;

        ImGui::PushID(i);
        ImGui::BeginDisabled(editable == false);

            switch (pub.type) {
                case INTEGER: {
                    if (anno.hasChoices) {
                        auto data = Helpers::toImGuiList(anno.choices);
                        int choice = 0;
                        for (auto& c: anno.choices) {
                            if (c == std::to_string(*pub.value.i)) {
                                break;
                            }
                            choice++;
                        }

                        // there isn't anything better to do tbh ..
                        if (choice == anno.choices.size()) choice = 0;

                        if (ImGui::Combo("##<empty>", &choice, data.first, data.second)) {
                            try {
                                 (*pub.value.i) = std::stoi(anno.choices[choice]);
                            } catch (...) {
                                // no need to handle the error
                            }
                        }
                    } else {
                        int start = -1;
                        int end = -1;
                        bool ranged = false;
                        if (anno.rangeStart != "None" && anno.rangeEnd != "None") {
                            try {
                                start = std::stoi(anno.rangeStart);
                                end   = std::stoi(anno.rangeEnd);
                                ranged = true;
                            } catch (...) {
                                // no need to handle the error
                            }
                        }

                        if (ranged) {
                            ImGui::DragInt(anno.attrName.c_str(), pub.value.i, 1, start, end);
                        } else {
                            ImGui::InputInt(anno.attrName.c_str(), pub.value.i);
                        }
                        if (ImGui::BeginItemTooltip()) {
                            ImGui::Text("%s",anno.disc.c_str());
                            ImGui::EndTooltip();
                        }
                    }
                    break;
                }

                case FLOAT: {
                    // todo
                    break;
                }

                case STRING: {
                    // todo
                    break;
                }

                default: ;
            }

        ImGui::EndDisabled();
        ImGui::PopID();
    }

    if (obj->annotations.size() == 0) {
        ImGui::TextDisabled("This object has no annotations");
    }
}

static void _render_visualizable(Pipeline::VisualizedObject* vis) {
    // Object Tree
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IN_OBJ_HEADER);
        bool open = ImGui::TreeNodeEx("##object", ImGuiTreeNodeFlags_DefaultOpen, "[Object]");
        ImGui::PopStyleColor();
        if (open) {
            _render_python_object(vis->visualizable);
            ImGui::TreePop();
        }
    }

    // RGB Params
    if (vis->rgb_array_params && vis->rgb_array_params->annotations.size()) {
        ImGui::Dummy({1, 2});
        ImGui::PushStyleColor(ImGuiCol_Text, IN_OBJ_HEADER);
        bool open = ImGui::TreeNode("##rgb", "[RGB Params]");
        ImGui::PopStyleColor();
        if (open) {
            _render_python_object(vis->rgb_array_params);
            ImGui::TreePop();
        }
    }

    // Gray Params
    if (vis->gray_params && vis->gray_params->annotations.size()) {
        ImGui::Dummy({1, 2});
        ImGui::PushStyleColor(ImGuiCol_Text, IN_OBJ_HEADER);
        bool open = ImGui::TreeNode("##gray", "[Gray Params]");
        ImGui::PopStyleColor();
        if (open) {
            _render_python_object(vis->gray_params);
            ImGui::TreePop();
        }
    }

    // HeatMap Params
    if (vis->heat_map_params && vis->heat_map_params->annotations.size()) {
        ImGui::Dummy({1, 2});
        ImGui::PushStyleColor(ImGuiCol_Text, IN_OBJ_HEADER);
        bool open = ImGui::TreeNode("##heatmap", "[HeatMap Params]");
        ImGui::PopStyleColor();
        if (open) {
            _render_python_object(vis->heat_map_params);
            ImGui::TreePop();
        }
    }

    // Features Params
    if (vis->features_params && vis->features_params->annotations.size()) {
        ImGui::Dummy({1, 2});
        ImGui::PushStyleColor(ImGuiCol_Text, IN_OBJ_HEADER);
        bool open = ImGui::TreeNode("##features", "[Features Params]");
        ImGui::PopStyleColor();
        if (open) {
            _render_python_object(vis->features_params);
            ImGui::TreePop();
        }
    }

    // BarChart Params
    if (vis->bar_chart_params && vis->bar_chart_params->annotations.size()) {
        ImGui::Dummy({1, 2});
        ImGui::PushStyleColor(ImGuiCol_Text, IN_OBJ_HEADER);
        bool open = ImGui::TreeNode("##barchart", "[BarChart Params]");
        ImGui::PopStyleColor();
        if (open) {
            _render_python_object(vis->bar_chart_params);
            ImGui::TreePop();
        }
    }
}


static void _render_info(Pipeline::VisualizedAgent* agent) {
    ImGui::TextColored(SECTION_HEADER, "%s", agent->agent->name);
    if (ImGui::BeginItemTooltip()) {
        ImGui::TextDisabled("%s",agent->agent->agent->moduleName.c_str());
        ImGui::EndTooltip();
    }
    _render_python_object(agent->agent->agent);

    // TODO
    // add scores / final score etc ..
}

static void _render_control(Pipeline::VisualizedAgent* agent) {
    ImGui::TextColored(SECTION_HEADER, "Controls");

    std::vector<Pipeline::ActiveAgent::Action> data;
    {
        std::lock_guard guard(agent->agent->available_actions_lock);
        data = agent->agent->available_actions;
    }

    float barHeight = 24.0f;
    float fullWidth = ImGui::GetContentRegionAvail().x;

    for (size_t i = 0; i < data.size(); ++i) {
        const auto& action = data[i];

        float barWidth = action.probability * fullWidth;

        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImVec2 fullSize = ImVec2(fullWidth, barHeight);
        ImVec2 fullEnd = ImVec2(cursorPos.x + fullSize.x, cursorPos.y + fullSize.y);

        // Entire border area
        ImGui::InvisibleButton(("bar_full_" + std::to_string(i)).c_str(), fullSize);
        bool hovered = ImGui::IsItemHovered();
        bool clicked = ImGui::IsItemClicked();

        // Background bar color
        ImU32 borderColor = hovered ? IM_COL32(120, 220, 255, 255) : IM_COL32(180, 180, 180, 255);
        ImU32 fillColor = hovered ? IM_COL32(100, 200, 255, 255) : IM_COL32(100, 150, 250, 255);

        // Draw filled part
        ImVec2 barEnd = ImVec2(cursorPos.x + barWidth, cursorPos.y + barHeight);
        ImGui::GetWindowDrawList()->AddRectFilled(cursorPos, barEnd, fillColor, 4.0f);

        // Draw border around the entire full-width area
        ImGui::GetWindowDrawList()->AddRect(cursorPos, fullEnd, borderColor, 4.0f, 0, 1.5f);

        // Draw text label on top of bar
        ImVec2 textPos = ImVec2(cursorPos.x + 6, cursorPos.y + 4);
        ImGui::GetWindowDrawList()->AddText(textPos, IM_COL32(255, 255, 255, 255), action.name.c_str());

        // Tooltip with action and probability
        if (hovered) {
            ImGui::BeginTooltip();
            ImGui::Text("Action: %s", action.name.c_str());
            ImGui::Text("Probability: %.2f", action.probability);
            ImGui::Text("Explore environment with this action.");
            ImGui::EndTooltip();
        }

        if (clicked) {
            Pipeline::stepSim(agent->agent_index, i);
        }

        ImGui::Dummy(ImVec2(0.0f, 6.0f)); // Spacing between bars
    }

    if (Widgets::ImageTextButton(ImageStore::idOf("./assets/icons/reset-grad.png"), "Reset Env", ImVec2(28, 28))) {
        Pipeline::resetEnv(agent->agent_index);
    }
}

static void _render_env_details(Pipeline::VisualizedAgent* agent) {

    ImGui::TextColored(SECTION_HEADER, "%s", Pipeline::envs[Pipeline::PipelineConfig::activeEnv].acceptor->_tag);
    if (ImGui::BeginItemTooltip()) {
        ImGui::TextDisabled("%s", agent->agent->env->moduleName.c_str());
        ImGui::EndTooltip();
    }
    _render_visualizable(agent->env_visualization);
}

static void _render_methods_details(Pipeline::VisualizedAgent* agent) {
    bool first = true;
    for (int i = 0; i < agent->method_visualizations.size(); ++i) {
        ImGui::PushID(i);
        if (auto& method = agent->method_visualizations[i]) {
            if (!first) {
                ImGui::Dummy({1, 5});
            }
            first = false;

            auto& pipMethod = Pipeline::PipelineConfig::pipelineMethods[i];
            ImGui::TextColored(SECTION_SUB_HEADER, "%s", pipMethod.name);
            if (ImGui::BeginItemTooltip()) {
                ImGui::TextDisabled("%s", method->visualizable->moduleName.c_str());
                ImGui::EndTooltip();
            }
            _render_visualizable(method);
        }
        ImGui::PopID();
    }
}

static void _render_content(Pipeline::VisualizedAgent* agent) {
    static std::vector<float> sizes = {0.1, 0.3, 0.3, 0.3};

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
        Splitter::Vertical(sizes, [agent](int i) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
                if (i == 0) _render_info(agent);
                if (i == 1) _render_control(agent);
                if (i == 2) _render_env_details(agent);
                if (i == 3) _render_methods_details(agent);
            ImGui::PopStyleVar();
        });
    ImGui::PopStyleVar();
}

void Inspector::render() {
    if (!Pipeline::isExperimenting()) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    int index = 0;
    for (auto agent: Pipeline::PipelineState::activeVisualizations) {
        auto& cache = _cache[agent];
        index++;

        if (ImGui::Begin(cache.window_name.c_str(), &cache.window_opened))
            _render_content(agent);
        ImGui::End();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

void Inspector::onStop() {
    _cache.clear();
}

void Inspector::destroy() {
    _cache.clear();
}

void Inspector::open(Pipeline::VisualizedAgent* agent) {
    auto it = _cache.find(agent);
    if (it != _cache.end()) {
        it->second.window_opened = true;
    }
}
