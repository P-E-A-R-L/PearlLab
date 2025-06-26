#include "inspector.hpp"

#include <mutex>
#include <thread>

#include "imgui.h"
#include "imgui_internal.h"
#include "implot.h"
#include "pipeline.hpp"
#include "preview.hpp"
#include "../lab_layout.hpp"
#include "../utility/helpers.hpp"
#include "../utility/image_store.hpp"
#include "../widgets/image_text_button.hpp"
#include "../widgets/scrollable_plot.hpp"
#include "../widgets/splitter.hpp"

namespace Inspector {
    struct InspectorCache {
        bool window_opened = false;
        bool play          = false;
        std::string window_name = "";

        std::unordered_map<std::string, int> plot_scroll_offsets;
    };


    static std::map<Pipeline::VisualizedAgent*, Inspector::InspectorCache> _cache;
}


void Inspector::init() {
    ImPlot::CreateContext();
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

        LabLayout::dockWindow(_cache[a].window_name, LabLayout::CENTER_RIGHT);
    }
}


static auto IN_OBJ_HEADER  = ImVec4{166 / 255.0f, 25 / 255.0f, 163 / 255.0f, 255 / 255.0f};
static auto SECTION_HEADER = ImVec4{166 / 255.0f, 159 / 255.0f, 25 / 255.0f, 255 / 255.0f};
static auto SECTION_SUB_HEADER = ImVec4{31 / 255.0f, 171 / 255.0f, 73 / 255.0f, 255 / 255.0f};

static void render_python_object(PyLiveObject* obj) {
    std::lock_guard guard(*obj->m_lock);

    for (int i = 0; i < obj->annotations.size(); ++i) {
        auto& anno    = obj->annotations[i];
        auto& pub = obj->public_members_values[i];

        auto editable = anno.editable && pub.type != OTHER;

        ImGui::PushID(i);
        ImGui::BeginDisabled(editable == false);

        // todo: test this thing one day ...
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
                    if (anno.hasChoices) {
                        auto data = Helpers::toImGuiList(anno.choices);
                        int choice = 0;
                        for (auto& c: anno.choices) {
                            if (c == std::to_string(*pub.value.f)) {
                                break;
                            }
                            choice++;
                        }

                        // there isn't anything better to do tbh ..
                        if (choice == anno.choices.size()) choice = 0;

                        if (ImGui::Combo("##<empty>", &choice, data.first, data.second)) {
                            try {
                                 (*pub.value.f) = std::stof(anno.choices[choice]);
                            } catch (...) {
                                // no need to handle the error
                            }
                        }
                    } else {
                        float start = -1;
                        float end = -1;
                        bool ranged = false;
                        if (anno.rangeStart != "None" && anno.rangeEnd != "None") {
                            try {
                                start = std::stof(anno.rangeStart);
                                end   = std::stof(anno.rangeEnd);
                                ranged = true;
                            } catch (...) {
                                // no need to handle the error
                            }
                        }

                        if (ranged) {
                            ImGui::DragFloat(anno.attrName.c_str(), pub.value.f, 1, start, end);
                        } else {
                            ImGui::InputFloat(anno.attrName.c_str(), pub.value.f);
                        }
                        if (ImGui::BeginItemTooltip()) {
                            ImGui::Text("%s",anno.disc.c_str());
                            ImGui::EndTooltip();
                        }
                    }
                    break;
                }

                case STRING: {
                    if (anno.hasChoices) {
                        auto data = Helpers::toImGuiList(anno.choices);
                        int choice = 0;
                        for (auto& c: anno.choices) {
                            if (c == *pub.value.str) {
                                break;
                            }
                            choice++;
                        }

                        // there isn't anything better to do tbh ..
                        if (choice == anno.choices.size()) choice = 0;

                        if (ImGui::Combo("##<empty>", &choice, data.first, data.second)) {
                            try {
                                 (*pub.value.str) = anno.choices[choice];
                            } catch (...) {
                                // no need to handle the error
                            }
                        }
                    } else {
                        static char buff[1024];

                        strncpy(buff, pub.value.str->c_str(), 1023);
                        buff[1023] = '\0';

                        if (ImGui::InputText(anno.attrName.c_str(), buff, 1024)) {
                            *pub.value.str = buff;
                        }

                        if (ImGui::BeginItemTooltip()) {
                            ImGui::Text("%s",anno.disc.c_str());
                            ImGui::EndTooltip();
                        }
                    }
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

static void render_visualizable(Pipeline::VisualizedObject* vis) {
    // Object Tree
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IN_OBJ_HEADER);
        bool open = ImGui::TreeNodeEx("##object", ImGuiTreeNodeFlags_DefaultOpen, "[Object]");
        ImGui::PopStyleColor();
        if (open) {
            render_python_object(vis->visualizable);
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
            render_python_object(vis->rgb_array_params);
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
            render_python_object(vis->gray_params);
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
            render_python_object(vis->heat_map_params);
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
            render_python_object(vis->features_params);
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
            render_python_object(vis->bar_chart_params);
            ImGui::TreePop();
        }
    }
}


static void ShowAgentStateColored(Pipeline::ActiveAgentState state)
{
    const char* label = "Unknown";
    ImVec4 color = ImVec4(1, 1, 1, 1);

    switch (state) {
        case Pipeline::IDLE:              label = "IDLE";              color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); break;
        case Pipeline::SELECTING_ACTION:  label = "SELECTING_ACTION";  color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f); break;
        case Pipeline::STEPPING:          label = "STEPPING";          color = ImVec4(0.4f, 0.7f, 1.0f, 1.0f); break;
        case Pipeline::RESET_ENV:         label = "RESET_ENV";         color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break;
    }

    ImGui::TextColored(color, "%s", label);
}

static void ShowAgentStatsUI(Pipeline::ActiveAgent* agent, Pipeline::VisualizedAgent* preview)
{
    if (!agent) {
        ImGui::Text("No agent loaded.");
        return;
    }

    ImGui::SeparatorText("Basic");
    if (ImGui::BeginTable("BasicInfo", 2)) {
        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Agent State:");
        ImGui::TableSetColumnIndex(1); ShowAgentStateColored(agent->state.load());

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Pause Requested:");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", agent->request_pause.load() ? "Yes" : "No");

        ImGui::EndTable();
    }

    ImGui::SeparatorText("Environment");
    if (ImGui::BeginTable("EnvInfo", 2)) {
        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Terminated:");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", agent->env_terminated ? "Yes" : "No");

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Truncated:");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", agent->env_truncated ? "Yes" : "No");

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Last Move Reward:");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", agent->last_move_reward);

        ImGui::EndTable();
    }

    ImGui::SeparatorText("Episode Progress");
    if (ImGui::BeginTable("Progress", 2)) {
        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Total Reward (All):");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", agent->reward_total);

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Total Reward (Ep):");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f", agent->reward_ep);

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Steps (Ep):");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%" PRId64, agent->steps_current_episode);

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Total Steps:");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%" PRId64, agent->total_steps);

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Episodes Completed:");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%" PRId64, agent->total_episodes);

        ImGui::EndTable();
    }

    ImGui::SeparatorText("Worker State");
    if (ImGui::BeginTable("WorkerInfo", 2)) {
        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Worker Running:");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", agent->_worker_running.load() ? "Yes" : "No");

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Worker Stop Requested:");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", agent->_worker_stop.load() ? "Yes" : "No");

        ImGui::EndTable();
    }

    ImGui::SeparatorText("Action Logic");
    if (ImGui::BeginTable("Action Logic", 2)) {
        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Last Taken Action:");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%zu", agent->next_action.load());

        ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::Text("Steps To Take:");
        ImGui::TableSetColumnIndex(1); ImGui::Text("%zu", agent->steps_to_take.load());

        ImGui::EndTable();
    }

    ImGui::SeparatorText("Explainability Scores");
    if (ImGui::BeginTable("AgentScoresTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Method");
        ImGui::TableSetupColumn("Episode Score");
        ImGui::TableSetupColumn("Total Score");
        ImGui::TableSetupColumn("Normalized");
        ImGui::TableHeadersRow();

        std::lock_guard<std::mutex> lock(agent->score_update_lock);
        for (size_t i = 0; i < agent->methods.size(); ++i) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", Pipeline::PipelineConfig::pipelineMethods[i].name);

            ImGui::TableSetColumnIndex(1);
            float ep_score = i < agent->scores_ep.size() ? static_cast<float>(agent->scores_ep[i]) : 0.0f;
            ImGui::Text("%.2f", ep_score);

            ImGui::TableSetColumnIndex(2);
            float total_score = i < agent->scores_total.size() ? static_cast<float>(agent->scores_total[i]) : 0.0f;
            ImGui::Text("%.2f", total_score);

            ImGui::TableSetColumnIndex(3);
            float normalized = (agent->total_steps > 0) ? total_score / static_cast<float>(agent->total_steps) : 0.0f;
            ImGui::Text("%.4f", normalized);
        }

        ImGui::EndTable();
    }


    // Control parameters
    static int window_size = 50; // Number of points visible at once

    auto PlotSeriesScrollable = [&](const char* title, const std::vector<std::vector<float>>& series_list,
                                    const std::vector<const char*>& labels) {
        if (series_list.empty() || series_list[0].empty())
            return;

        int total_points = static_cast<int>(series_list[0].size());
        int& offset = Inspector::_cache[preview].plot_scroll_offsets[title]; // persistent scroll per plot

        offset = std::clamp(offset, 0, std::max(0, total_points - window_size));

        ImGui::SliderInt(std::string("Scroll##" + std::string(title)).c_str(), &offset, 0,
                         std::max(0, total_points - window_size));

        if (ImPlot::BeginPlot(title, ImVec2(-1, 200))) {
            ImPlot::SetupAxes("Step", "Score", ImPlotAxisFlags_NoTickLabels);
            ImPlot::SetupAxisLimits(ImAxis_X1, offset, offset + window_size, ImPlotCond_Always);

            for (size_t i = 0; i < series_list.size(); ++i) {
                const auto& series = series_list[i];
                if (series.size() < static_cast<size_t>(offset))
                    continue;

                int n = std::min(window_size, total_points - offset);
                ImPlot::PlotLine(labels[i], &series[offset], n);
            }

            ImPlot::EndPlot();
        }
    };

    ImGui::SeparatorText("Step-Level Score Visualizations");

    // Episode-level scores
    {
        std::vector<std::vector<float>> series_list;
        std::vector<const char*> labels;

        for (size_t i = 0; i < agent->steps_scores_ep.size(); ++i) {
            series_list.push_back(agent->steps_scores_ep[i].scores);
            labels.push_back(Pipeline::PipelineConfig::pipelineMethods[i].name);
        }

        PlotSeriesScrollable("Episode Scores", series_list, labels);
    }

    // Total-level scores
    {
        std::vector<std::vector<float>> series_list;
        std::vector<const char*> labels;

        for (size_t i = 0; i < agent->steps_scores_total.size(); ++i) {
            series_list.push_back(agent->steps_scores_total[i].scores);
            labels.push_back(Pipeline::PipelineConfig::pipelineMethods[i].name);
        }

        PlotSeriesScrollable("Total Scores", series_list, labels);
    }

    ImGui::SeparatorText("Reward Trajectory");

    // Rewards
    {
        std::vector<std::vector<float>> series_list = {
            agent->steps_rewards_ep.scores,
            agent->steps_rewards_total.scores
        };
        const std::vector<const char*> labels = { "Episode Reward", "Total Reward" };

        PlotSeriesScrollable("Rewards", series_list, labels);
    }
}


static void render_info(Pipeline::VisualizedAgent* agent) {
    auto active_agent = agent->agent;

    ImGui::TextColored(SECTION_HEADER, "%s", active_agent->name);
    if (ImGui::BeginItemTooltip()) {
        ImGui::TextDisabled("%s",active_agent->agent->moduleName.c_str());
        ImGui::EndTooltip();
    }

    ImGui::TextColored(IN_OBJ_HEADER, "[Object]");
    render_python_object(active_agent->agent);


    ImGui::TextColored(IN_OBJ_HEADER, "[Info]");
    ShowAgentStatsUI(agent->agent, agent);
}

static void render_control(Pipeline::VisualizedAgent* agent) {
    ImGui::TextColored(SECTION_HEADER, "Controls");

    std::vector<Pipeline::ActiveAgent::Action> data;
    {
        std::lock_guard guard(agent->agent->available_actions_lock);
        data = agent->agent->available_actions;
    }

    float barHeight = 24.0f;
    float fullWidth = ImGui::GetContentRegionAvail().x;

    ImGui::TextColored(IN_OBJ_HEADER, "[Actions]");
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



    ImGui::TextColored(IN_OBJ_HEADER, "[Agent]");
    auto& cache = Inspector::_cache[agent];
    {
        std::string Icon = "./assets/icons/play-grad.png";
        std::string text = "Play";
        if (cache.play) {
            Icon = "./assets/icons/pause-grad.png";
            text = "Pause";
        }

        if (Widgets::ImageTextButton(ImageStore::idOf(Icon), text.c_str(), {28, 28})) {
            if (!cache.play)
                cache.play = true;
            else
                cache.play = false;
        }
    }

    ImGui::SameLine();
    ImGui::Dummy({10, 10});
    ImGui::SameLine();

    {
        if (Widgets::ImageTextButton(ImageStore::idOf("./assets/icons/step-grad.png"), "Step", ImVec2(28, 28))) {
            Pipeline::stepSim(agent->agent_index);
        }
    }

    ImGui::SameLine();
    ImGui::Dummy({10, 10});
    ImGui::SameLine();

    {
        if (Widgets::ImageTextButton(ImageStore::idOf("./assets/icons/reset-grad.png"), "Reset Env", ImVec2(28, 28))) {
            Pipeline::resetEnv(agent->agent_index);
        }
    }
}

static void render_env_details(Pipeline::VisualizedAgent* agent) {

    ImGui::TextColored(SECTION_HEADER, "%s", Pipeline::envs[Pipeline::PipelineConfig::activeEnv].acceptor->_tag);
    if (ImGui::BeginItemTooltip()) {
        ImGui::TextDisabled("%s", agent->agent->env->moduleName.c_str());
        ImGui::EndTooltip();
    }
    render_visualizable(agent->env_visualization);
}

static void render_methods_details(Pipeline::VisualizedAgent* agent) {
    ImGui::TextColored(SECTION_HEADER, "Methods");
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
            render_visualizable(method);
        }
        ImGui::PopID();
    }
}

static void _render_content(Pipeline::VisualizedAgent* agent) {
    static std::vector<float> sizes = {0.5, 0.3, 0.1, 0.1};

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
        Splitter::Vertical(sizes, [agent](int i) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
                if (i == 0) render_info(agent);
                if (i == 1) render_control(agent);
                if (i == 2) render_env_details(agent);
                if (i == 3) render_methods_details(agent);
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

        if (cache.window_opened) {
            ImGui::SetNextWindowSize(ImVec2(300, 600), ImGuiCond_FirstUseEver);
            if (ImGui::Begin(cache.window_name.c_str(), &cache.window_opened))
                _render_content(agent);
            ImGui::End();
        }
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
