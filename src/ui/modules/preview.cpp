#include "preview.hpp"

#include <imgui.h>
#include <iostream>

#include "inspector.hpp"
#include "logger.hpp"
#include "pipeline.hpp"
#include "../font_manager.hpp"
#include "../../backend/py_safe_wrapper.hpp"
#include "../utility/gl_texture.hpp"
#include "../utility/image_store.hpp"

namespace Preview {
    struct PreviewCache {
        ImVec2 content_size          = ImVec2(1, 1);
    };


}

static bool isRunning = false;
static std::map<Pipeline::VisualizedAgent*, Preview::PreviewCache> _previewCache;

void Preview::init() {
    isRunning = false;
}

void Preview::update() {
    if (isRunning) {
        Pipeline::stepSim(-1); // step all agents
    }
}

static void render_visualizable(Pipeline::VisualizedObject *obj, const std::string &message)
{
    if (ImGui::BeginTabBar("##vis")) {
        if (obj->supports(VisualizationMethod::RGB_ARRAY) && ImGui::BeginTabItem("RGB")) {
            {
                std::lock_guard guard(*obj->m_lock);
                auto obs = obj->rgb_array;
                obj->rgb_array_viewer->Render("##RGBArrayView");
            }
            ImGui::EndTabItem();
        }

        if (obj->supports(VisualizationMethod::GRAY_SCALE) && ImGui::BeginTabItem("Gray")) {
            {
                std::lock_guard guard(*obj->m_lock);
                obj->gray_viewer->Render("##GrayView");
            }

            ImGui::EndTabItem();
        }

        if (obj->supports(VisualizationMethod::HEAT_MAP) && ImGui::BeginTabItem("Heat Map")) {
            {
                std::lock_guard guard(*obj->m_lock);
                obj->heat_map_viewer->Render("##HeatMapView");
            }

            ImGui::EndTabItem();
        }

        if (obj->supports(VisualizationMethod::FEATURES) && ImGui::BeginTabItem("Features")) {
            std::map<std::string, std::string> obs;

            {
                std::lock_guard guard(*obj->m_lock);
                obs = obj->features;
            }

            if (ImGui::BeginTable("StringFloatTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();

                for (const auto &[name, value] : obs)
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(name.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", value.c_str());
                }

                ImGui::EndTable();
            }

            ImGui::EndTabItem();
        }

        if (obj->supports(VisualizationMethod::BAR_CHART) && ImGui::BeginTabItem("Bar Chart")) {

            std::map<std::string, float> obs;
            {
                std::lock_guard guard(*obj->m_lock);
                obs = obj->bar_chart;
            }

            // min/max values for scaling
            float min_value = 0.0f;
            float max_value = 0.0f;
            for (const auto &[key, value] : obs) {
                min_value = std::min(min_value, value);
                max_value = std::max(max_value, value);
            }

            float range_padding = std::max(std::abs(max_value), std::abs(min_value)) * 0.2f;
            min_value -= range_padding;
            max_value += range_padding;
            if (min_value == max_value) {
                min_value -= 1.0f;
                max_value += 1.0f;
            }

            // chart parameters
            const float bar_width = 50.0f;
            const float bar_spacing = 30.0f;
            const float graph_height = 300.0f;
            const float zero_line_y = graph_height * (max_value / (max_value - min_value));
            const float value_range = max_value - min_value;

            const float graph_width = (bar_width + bar_spacing) * obs.size() + bar_spacing;

            if (ImGui::BeginChild("BarChartView", ImVec2(0, graph_height + 50), false,
                                  ImGuiWindowFlags_HorizontalScrollbar)) {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                const ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
                const ImU32 positive_color = ImGui::GetColorU32(ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                const ImU32 negative_color = ImGui::GetColorU32(ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                const ImU32 text_color = ImGui::GetColorU32(ImGuiCol_Text);
                const ImU32 axis_color = ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

                // zero line
                draw_list->AddLine(
                    ImVec2(cursor_pos.x, cursor_pos.y + zero_line_y),
                    ImVec2(cursor_pos.x + graph_width, cursor_pos.y + zero_line_y),
                    axis_color, 2.0f);

                float x_offset = bar_spacing;

                for (const auto &[key, value] : obs)
                {
                    const bool is_positive = value >= 0;
                    const float bar_height = (std::abs(value) / value_range) * graph_height;

                    // bar positions
                    ImVec2 bar_min, bar_max;
                    if (is_positive)
                    {
                        bar_min = ImVec2(cursor_pos.x + x_offset, cursor_pos.y + zero_line_y - bar_height);
                        bar_max = ImVec2(cursor_pos.x + x_offset + bar_width, cursor_pos.y + zero_line_y);
                    }
                    else
                    {
                        bar_min = ImVec2(cursor_pos.x + x_offset, cursor_pos.y + zero_line_y);
                        bar_max = ImVec2(cursor_pos.x + x_offset + bar_width, cursor_pos.y + zero_line_y + bar_height);
                    }

                    draw_list->AddRectFilled(bar_min, bar_max,
                                             is_positive ? positive_color : negative_color, 3.0f);

                    // value text
                    char value_text[32];
                    snprintf(value_text, sizeof(value_text), "%.2f", value);
                    const ImVec2 text_size = ImGui::CalcTextSize(value_text);

                    ImVec2 text_pos;
                    if (is_positive)
                    {
                        text_pos = ImVec2(
                            bar_min.x + (bar_width - text_size.x) * 0.5f,
                            bar_min.y - text_size.y - 2.0f);
                    }
                    else
                    {
                        text_pos = ImVec2(
                            bar_min.x + (bar_width - text_size.x) * 0.5f,
                            bar_max.y + 2.0f);
                    }
                    draw_list->AddText(text_pos, text_color, value_text);

                    // feature names
                    const ImVec2 name_pos = ImVec2(
                        bar_min.x + (bar_width - ImGui::CalcTextSize(key.c_str()).x) * 0.5f,
                        cursor_pos.y + graph_height + 5.0f);
                    draw_list->AddText(name_pos, text_color, key.c_str());

                    x_offset += bar_width + bar_spacing;
                }
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

static void _render_agent_basic(const Pipeline::ActiveAgent* agent, float width, int index)
{
    ImVec2 childStart = ImGui::GetCursorScreenPos();
    auto& vis = Pipeline::PipelineState::activeVisualizations[index];
    auto& cache              = _previewCache[vis];

    ImVec2 contentRegion = cache.content_size;
    ImVec2 buttonPos = ImGui::GetCursorScreenPos();

    ImGui::BeginChild(agent->name, ImVec2(width, 0), true);
    ImVec2 childPos = ImGui::GetCursorScreenPos();

    ImGui::PushID(index);
    ImGui::InvisibleButton("##click_area", contentRegion);
    bool clicked = ImGui::IsItemClicked();
    ImGui::PopID();

    /*
    if (Preview::curr_selected_preview == vis) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            buttonPos, ImVec2(buttonPos.x + contentRegion.x , buttonPos.y + contentRegion.y),
            IM_COL32(91, 150, 204, 150)
        );
    } else
     */
    if (ImGui::IsItemHovered()) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            buttonPos, ImVec2(buttonPos.x + contentRegion.x , buttonPos.y + contentRegion.y),
            IM_COL32(255, 255, 0, 50)
        );
    }

    if (clicked) {
        Inspector::open(vis);
    }

    ImGui::SetCursorScreenPos(childPos);

    FontManager::pushFont("Bold");
    ImGui::Text("%s", agent->name);
    FontManager::popFont();

    if (agent->env) {
        render_visualizable(Pipeline::PipelineState::activeVisualizations[index]->env_visualization, "No observations available.");
    } else {
        ImGui::Text("No environment available.");
    }

    if (ImGui::BeginTable("AgentScoresTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {

        ImGui::TableSetupColumn("Method");
        ImGui::TableSetupColumn("Episode Score");
        ImGui::TableSetupColumn("Total Score");
        ImGui::TableSetupColumn("Normalized");
        ImGui::TableHeadersRow();

        for (int i = 0; i < agent->methods.size(); ++i)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", Pipeline::PipelineConfig::pipelineMethods[i].name);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", agent->scores_ep[i]);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%.2f", agent->scores_total[i]);

            ImGui::TableSetColumnIndex(3);
            float normalized = (agent->total_steps > 0) ? agent->scores_total[i] / agent->total_steps : 0.0f;
            ImGui::Text("%.4f", normalized);
        }

        ImGui::EndTable();
    }

    if (agent->env_terminated || agent->env_truncated)
    {
        ImGui::Text("<Episode completed>");
    }

    ImGui::EndChild();

    ImVec2 childEnd = ImGui::GetItemRectMax();

    cache.content_size = ImVec2(childEnd.x - childStart.x, childEnd.y - childStart.y);
}

typedef std::function<void(Pipeline::ActiveAgent*, float, int)> _agent_render_function;

static int agents_per_row = 2;
static void tab_wrapper(const _agent_render_function &_f)
{
    ImGui::BeginChild("AgentsScrollArea", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);
    
    { // agents previews (env)
        auto available_width = ImGui::GetContentRegionAvail().x;
        auto child_width = (available_width - (agents_per_row + 1) * 10) / agents_per_row;
        child_width = std::max(child_width, 320.0f);

        int num_rows = (Pipeline::PipelineState::activeAgents.size() + agents_per_row - 1) / agents_per_row;
        float total_height = 0;
        
        for (int row = 0; row < num_rows; row++) {
            float max_height_in_row = 0;
            
            for (int col = 0; col < agents_per_row; col++) {
                int agent_index = row * agents_per_row + col;
                if (agent_index >= Pipeline::PipelineState::activeAgents.size())
                    break;
                
                auto &agent = Pipeline::PipelineState::activeAgents[agent_index];
                ImGui::PushID(agent_index);
                
                ImGui::SetCursorPos(ImVec2(col * (child_width + 10) + 10, total_height + 10));
                ImGui::BeginGroup();
                _f(agent, child_width, agent_index);
                ImGui::EndGroup();
                
                ImGui::PopID();
                auto size = ImGui::GetItemRectSize();
                max_height_in_row = std::max(max_height_in_row, size.y);
            }
            
            total_height += max_height_in_row + 10; // spacing after each row
        }
        
        // dummy item to ensure proper scrolling
        ImGui::SetCursorPos(ImVec2(0, total_height));
        ImGui::Dummy(ImVec2(1, 1));
    }
    
    ImGui::EndChild();
}


static void _render_preview() {
    for (int i = 0; i < Pipeline::PipelineState::activeAgents.size(); i++) {
        Pipeline::PipelineState::activeVisualizations[i]->update_tex(); // update the visualizations (textures)
    }

    { // control zone
        auto size = ImGui::GetContentRegionAvail();
        auto control_size = 16 * 2 + ImGui::GetStyle().ItemSpacing.x;
        auto x = (size.x - control_size) / 2;

        std::string Icon = "./assets/icons/play-grad.png";
        if (isRunning)
            Icon = "./assets/icons/pause-grad.png";

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        ImGui::SetCursorPosX(x);
        if (ImGui::ImageButton("##play_btn", ImageStore::idOf(Icon), {20, 20})) {
            if (!isRunning)
                isRunning = true;
            else
                isRunning = false;
        }

        ImGui::SameLine();
        if (ImGui::ImageButton("##step_btn", ImageStore::idOf("./assets/icons/step-grad.png"), {20, 20})) {
            Pipeline::stepSim(-1);
        }
        ImGui::PopStyleVar();
    }

    if (ImGui::BeginTabBar("Tabs"))
    {
        if (ImGui::BeginTabItem("Basic"))
        {
            tab_wrapper(_render_agent_basic);
            ImGui::EndTabItem();
        }

        for (int i = 0; i < Pipeline::PipelineConfig::pipelineMethods.size(); i++)
        {
            ImGui::PushID(i);
            if (Pipeline::PipelineConfig::pipelineMethods[i].active && ImGui::BeginTabItem(Pipeline::PipelineConfig::pipelineMethods[i].name))
            {
                tab_wrapper([&](Pipeline::ActiveAgent *agent, float width, int index)
                            {
                    ImGui::BeginChild(agent->name, ImVec2(width, 0), true);
                    FontManager::pushFont("Bold");
                    ImGui::Text("%s"   , agent->name);
                    FontManager::popFont();

                    if (agent->methods[i]) {
                        render_visualizable(Pipeline::PipelineState::activeVisualizations[index]->method_visualizations[i], "No observations available.");
                    } else {
                        ImGui::Text("No method visualization available.");
                    }


                    if (agent->env_terminated || agent->env_truncated) {
                        ImGui::Text("<Episode completed>");
                    }

                    ImGui::EndChild(); });
                ImGui::EndTabItem();
            }
            ImGui::PopID();
        }

        ImGui::EndTabBar();
    }
}

void Preview::render()
{
    ImGui::Begin("Preview");

    if (!Pipeline::isExperimenting()) {
        auto size = ImGui::GetContentRegionAvail();
        auto text_size = ImGui::CalcTextSize("No active experiment.\nStart an experiment in the pipeline tab.");

        auto x = (size.x - text_size.x) / 2;
        auto y = (size.y - text_size.y) / 2;

        ImGui::SetCursorPos({x, y});
        ImGui::Text("No active experiment.\nStart an experiment in the pipeline tab.");
    } else {
        _render_preview();
    }

    ImGui::End();
}

void Preview::onStart() {
    std::lock_guard guard(Pipeline::PipelineState::agentsLock);

    _previewCache.clear();
    for (auto& a: Pipeline::PipelineState::activeVisualizations) {
        _previewCache[a] = {};
    }
}

void Preview::onStop() {
    _previewCache.clear();
    isRunning = false;
}

void Preview::destroy()
{
    _previewCache.clear();
}
