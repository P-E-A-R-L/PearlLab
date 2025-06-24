#include "pipeline.hpp"

#include <iostream>
#include <thread>

#include "logger.hpp"
#include "../font_manager.hpp"
#include "imgui_internal.h"
#include "preview.hpp"
#include "../../backend/py_safe_wrapper.hpp"
#include "../utility/helpers.hpp"

namespace Pipeline
{
    namespace PipelineConfig
    {
        // all variables such as
        //   policy
        //   steps
        //   maxEp
        // etc .. will be stored here

        int maxSteps = 4000;    // default max steps for an agent
        int maxEpisodes = 4000; // default max episodes for an agent
        int activeEnv = 0;      // the index of the current active env

        std::vector<PipelineAgent> pipelineAgents{};
        std::vector<PipelineMethod> pipelineMethods{};

    }

    std::vector<PipelineGraph::ObjectRecipe> envs;
    std::vector<PipelineGraph::ObjectRecipe> agents;
    std::vector<PipelineGraph::ObjectRecipe> methods;

    std::vector<PipelineGraph::ObjectRecipe> recipes;

    namespace PipelineState
    {
        std::atomic<ExperimentState> experimentState = STOPPED;
        bool Simulating = false;
        int StepSimFrames = 0;

        std::vector<ActiveAgent*> activeAgents{};
        std::vector<VisualizedAgent *> previews{};

        StepPolicy stepPolicy = INDEPENDENT;
        ScorePolicy scorePolicy = PEARL;

        static std::atomic<int> _agentsCountOk;
        static std::atomic<int> _agentsCountFailed;
    }

    bool isExperimenting()
    {
        return PipelineState::experimentState == RUNNING;
    }

    static void _agent_worker_init(ActiveAgent* agent, PipelineAgent* recipe) {
        Logger::info("Initializing agent: " + std::string(agent->name));
        py::gil_scoped_acquire acquire; // take GIL lock

        agent->agent            = new PyAgent();
        agent->agent->object    = recipe->recipe->create();

        if (agent->agent->object.is_none()) {
            Logger::error("Failed to create agent: " + std::string(agent->name));
            PipelineState::_agentsCountFailed += 1;

            delete agent->env;
            delete agent->agent;
            agent->agent = nullptr;
            agent->env = nullptr;

            for (auto& m: agent->methods) {
                delete m;
            }
            agent->methods.clear();

            return;
        }

        PyScope::parseLoadedModule(py::getattr(agent->agent->object, "__class__"), *agent->agent);

        agent->env               = new PyEnv();
        agent->env->object       = envs[PipelineConfig::activeEnv].create();

        if (agent->env->object.is_none()) {
            Logger::error("Failed to create environment for agent: " + std::string(agent->name));
            PipelineState::_agentsCountFailed += 1;

            delete agent->env;
            delete agent->agent;
            agent->agent = nullptr;
            agent->env = nullptr;

            for (auto& m: agent->methods) {
                delete m;
            }
            agent->methods.clear();

            return;
        }

        PyScope::parseLoadedModule(py::getattr(agent->env->object, "__class__"), *agent->env);

        agent->reward_total = 0;
        agent->reward_ep    = 0;

        agent->steps_current_episode = 0;
        agent->total_episodes        = 0;
        agent->total_steps           = 0;

        agent->last_move_reward      = 0;
        agent->env_terminated        = false;
        agent->env_truncated         = false;

        for (auto& method: PipelineConfig::pipelineMethods) {
            const auto methodPtr          = new PyMethod();
            methodPtr->object             = method.recipe->create();

            if (methodPtr->object.is_none()) {
                delete methodPtr;
                Logger::error("Failed to create method for agent: " + std::string(agent->name));
                PipelineState::_agentsCountFailed += 1;

                delete agent->env;
                delete agent->agent;
                agent->agent = nullptr;
                agent->env = nullptr;

                for (auto& m: agent->methods) {
                    delete m;
                }
                agent->methods.clear();

                return;
            }

            PyScope::parseLoadedModule(
                py::getattr(methodPtr->object, "__class__"), *methodPtr
            );

            agent->methods      .push_back(methodPtr);
            agent->scores_total .push_back(0);
            agent->scores_ep    .push_back(0);
        }

        agent->steps_to_take = 0;
        agent->state         = IDLE;
        agent->next_action   = 0;

        Logger::info("Initializing agent: " + std::string(agent->name) + ", Completed.");
    }

    static void _agent_worker_update(ActiveAgent* agent, VisualizedAgent* preview) {
        // std::cout << "Agent[" << agent->name << "] steps=" << agent->steps_to_take << std::endl;

        preview->update();

        if (agent->state == IDLE) {
            if (agent->steps_to_take > 0) {
                agent->state = ActiveAgentState::SELECTING_ACTION;
                agent->steps_to_take -= 1;
            } else {
                // nothing to do, just idle
            }
        } else if (agent->state == ActiveAgentState::SELECTING_ACTION) {
            goto _update_agents_state_select_action;
        } else if (agent->state == ActiveAgentState::STEPPING) {
            goto update_state;
        } else {
            // unknown state, just continue
        }

        return;

        _update_agents_state_select_action: {
            py::gil_scoped_acquire acquire; // take GIL lock
            auto target_agent = agent;
            auto actions = target_agent->env->get_available_actions();

            if (!actions) {
                Logger::error("Unable to retrieve actions, agent[" + std::string(py::str(target_agent->agent->object)) + "] environment didn't provide actions, unable to step.");
                return;
            }

            switch (PipelineState::stepPolicy) {
                case PipelineState::RANDOM:
                    target_agent->next_action = rand() % actions->size();
                    break;
                case PipelineState::BEST_AGENT: {
                    int best_agent = -1;
                    float best_score = std::numeric_limits<float>::min();
                    for (int i = 0; i < PipelineState::activeAgents.size(); ++i) {
                        if (best_agent == -1) {best_agent = i;
                            continue;
                        }

                        auto score = evalAgent(i);
                        if (best_score < score) {
                            best_score = score;
                            best_agent = i;
                        }
                    }

                    // now we have the best agent
                    // time we select its action
                    auto &best = PipelineState::activeAgents[best_agent];
                    SafeWrapper::execute([target_agent, best]{
                        py::gil_scoped_acquire acquire;
                        auto observations = target_agent->env->get_observations();
                        auto prediction = best->agent->predict(observations);
                        target_agent->next_action = PyScope::argmax(prediction);
                    });
                }
                    break;
                case PipelineState::WORST_AGENT: {
                    int worst_agent = -1;
                    float worst_score = std::numeric_limits<float>::min();
                    for (int i = 0; i < PipelineState::activeAgents.size(); ++i) {
                        if (worst_agent == -1) {
                            worst_agent = i;
                            continue;
                        }
                        auto score = evalAgent(i);
                        if (worst_score > score)
                        {
                            worst_score = score;
                            worst_agent = i;
                        }
                    }

                    auto &worst = PipelineState::activeAgents[worst_agent];
                    SafeWrapper::execute([target_agent, worst]{
                        py::gil_scoped_acquire acquire;
                        auto observations = target_agent->env->get_observations();
                        auto prediction = worst->agent->predict(observations);
                        target_agent->next_action = PyScope::argmax( prediction);
                    });
                }
                    break;

                case PipelineState::INDEPENDENT: {
                    SafeWrapper::execute([target_agent]{
                        py::gil_scoped_acquire acquire;
                        auto observations = target_agent->env->get_observations();
                        auto prediction = target_agent->agent->predict(observations);
                        target_agent->next_action = PyScope::argmax( prediction);
                    });
                }
                    break;

                default:
                    target_agent->next_action = 0; // default to just the first action
            }

            target_agent->state = ActiveAgentState::STEPPING; // update to indate we are ready for the next state :)
            return;
        }

        update_state: {
            py::gil_scoped_acquire acquire; // take GIL lock
            auto target_agent = agent;

            SafeWrapper::execute([&]{
                // get the observations
                auto ops = target_agent->env->get_observations();

                // notify all explainability methods
                size_t action = target_agent->next_action;
                for (auto& method: target_agent->methods) {
                    method->onStep(py::int_(action));
                }

                // do the action
                auto result = target_agent->env->step(py::int_(action));

                for (auto& method: target_agent->methods) {
                    method->onStepAfter(py::int_(action), std::get<1>(result), std::get<2>(result) || std::get<3>(result), std::get<4>(result));
                }

                // update agent analytics
                target_agent->total_steps           += 1;
                target_agent->steps_current_episode += 1;
                target_agent->env_terminated = std::get<2>(result);
                target_agent->env_truncated  = std::get<3>(result);

                for (int i = 0; i < target_agent->methods.size(); ++i) {
                    auto value = target_agent->methods[i]->value(ops);
                    target_agent->scores_total[i] += value;
                    target_agent->scores_ep   [i] += value;
                }
            });

            target_agent->state = IDLE;

            return;
        }

    }

    static void _agent_worker(ActiveAgent* agent, PipelineAgent* recipe, VisualizedAgent* preview) {
        std::thread([agent, recipe, preview] {
            agent->_worker_running = true;

            _agent_worker_init(agent, recipe);

            if (agent->agent == nullptr) {
                // some error happened in init
                agent->_worker_running = false;
                return; // early exit
            }

            agent->state = IDLE;
            agent->next_action = 0;
            agent->steps_to_take = 0;

            PipelineState::_agentsCountOk += 1;

            while (!agent->_worker_stop && PipelineState::experimentState == INITIALIZING) {
                // wait for all agents to be initialized
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }


            if (PipelineState::experimentState == INITIALIZING_TEXTURES) { // set up preview
                py::gil_scoped_acquire acquire;
                try {
                    preview->init(agent);
                } catch (...) {
                    // I don't give a fk if it fails at this point tbh ...
                }
                PipelineState::_agentsCountOk += 1;
            }

            while (!agent->_worker_stop) {
                _agent_worker_update(agent, preview);
                // std::cout << "Agent[" << agent->name << "] worker running." << std::endl;
            }

            // std::cout << "Agent[" << agent->name << "] worker stopped." << std::endl;
            agent->_worker_running = false;
        }).detach();
    }

    void beginExperiment()
    {
        if (PipelineState::experimentState != STOPPED)
            return;

        // some sanity checks
        if (PipelineConfig::pipelineAgents.empty())
        {
            Logger::error("No agents configured for the experiment.");
            return;
        }

        if (PipelineConfig::pipelineMethods.empty())
        {
            Logger::warning("No methods configured for the experiment.");
        }

        if (PipelineConfig::activeEnv >= envs.size())
        {
            Logger::error("Select an environment.");
            return;
        }

        if (PipelineConfig::maxEpisodes <= 0)
        {
            Logger::error("Max episodes must be greater than 0.");
            return;
        }

        if (PipelineConfig::maxSteps <= 0)
        {
            Logger::error("Max steps must be greater than 0.");
            return;
        }

        // now prepare the actual agents
        Logger::info("Preparing agents for the experiment...");

        PipelineState::experimentState    = INITIALIZING;
        PipelineState::_agentsCountOk     = 0;
        PipelineState::_agentsCountFailed = 0;

        for (int i = 0;i < PipelineConfig::pipelineAgents.size();i++) {
            auto activeAgent = new ActiveAgent();
            auto preview     = new VisualizedAgent();

            strcpy(activeAgent->name, PipelineConfig::pipelineAgents[i].name);
            _agent_worker(activeAgent, &PipelineConfig::pipelineAgents[i], preview);

            PipelineState::activeAgents.push_back(activeAgent);
            PipelineState::previews.push_back(preview);
        }
    }

    void _clearActiveAgents()
    {
        py::gil_scoped_acquire acquire;
        for (auto active : PipelineState::activeAgents) {
            delete active->agent;
            delete active->env;

            for (auto &m : active->methods) {
                delete m;
            }

            delete active;
        }

        for (auto preview : PipelineState::previews) {
            delete preview;
        }

        PipelineState::activeAgents.clear();
        PipelineState::previews.clear();
    }

    void stopExperiment()
    {
        if (PipelineState::experimentState != RUNNING)
            return;

        Logger::info("Stopping experiment ...");
        PipelineState::experimentState = STOPPING;

        for (auto& agent: PipelineState::activeAgents) {
            agent->_worker_stop = true;
        }
    }

    bool isSimRunning()
    {
        return PipelineState::Simulating;
    }

    void pauseSim()
    {
        PipelineState::Simulating = false;
    }

    void continueSim()
    {
        PipelineState::Simulating = true;
    }

    void stepSim() {
        PipelineState::StepSimFrames = 1; // step one frame
    }

    void resetSim() {
        py::gil_scoped_acquire acquire{};
        for (auto active : PipelineState::activeAgents)
        {
            // reset stats
            active->env->reset();

            for (auto method : active->methods)
            {
                method->set(active->env->object);
                method->prepare(active->agent->object);
            }

            // reset scores and rewards
            active->reward_total = 0;
            active->reward_ep = 0;

            active->steps_current_episode = 0;
            active->total_episodes = 0;
            active->total_steps = 0;

            active->last_move_reward = 0;
            active->env_terminated = false;
            active->env_truncated = false;

            for (int i = 0; i < active->scores_ep.size(); i++)
            {
                active->scores_ep[i] = 0;
                active->scores_total[i] = 0;
            }

            active->state = IDLE;
            active->next_action = 0;
            active->steps_to_take = 0;
        }
    }

    void stepSim(int action_index){
        // TODO
    }

    void stepSim(int action_index, int agent_index) {
        // TODO
    }

    float evalAgent(int agent_index)
    {
        if (agent_index < 0 || agent_index >= PipelineState::activeAgents.size())
        {
            Logger::error("Invalid agent index, tried to get scores for agent[" + std::to_string(agent_index) + "] which doesn't exist.");
            return -1;
        }

        auto &agent = PipelineState::activeAgents[agent_index];
        switch (PipelineState::scorePolicy)
        {
            case PipelineState::PEARL: {
                float result = 0;
                for (int i = 0; i < agent->scores_total.size(); ++i)
                {
                    result += agent->scores_total[i] * (PipelineConfig::pipelineMethods[i].active ? PipelineConfig::pipelineMethods[i].weight : 0);
                }
                return result / agent->total_steps;
            }
            case PipelineState::REWARD: {
                return agent->reward_total / agent->total_steps;
            }
        }

        return -1;
    }

    void setRecipes(std::vector<PipelineGraph::ObjectRecipe> r)
    {
        if (isExperimenting())
        {
            Logger::warning("Cannot set recipes while an experiment is running.");
            return;
        }

        // todo: memory leak here

        recipes.clear();
        envs.clear();
        agents.clear();
        methods.clear();

        PipelineConfig::pipelineAgents.clear();
        PipelineConfig::pipelineMethods.clear();

        for (auto &recipe : r)
        {
            if (recipe.type == PipelineGraph::Environment)
                envs.push_back(recipe);
            if (recipe.type == PipelineGraph::Agent)
                agents.push_back(recipe);
            if (recipe.type == PipelineGraph::Method)
                methods.push_back(recipe);

            recipes.push_back(recipe);
        }
    }

    void init()
    {
        envs.clear();
        agents.clear();
        methods.clear();
    }

    static void render_recipes()
    {
        ImGui::Begin("Recipes");

        int visibleCount = 0;
        for (int i = 0; i < recipes.size(); ++i)
        {
            auto &recipe = recipes[i];

            std::string id = "recipe_" + std::to_string(i);
            ImGui::PushID(id.c_str());

            std::string typeName = "A";
            if (recipe.type == PipelineGraph::Environment)
                typeName = "E";
            if (recipe.type == PipelineGraph::Method)
                typeName = "M";

            ImGui::Text(typeName.c_str());
            ImGui::SameLine();

            ImGui::SetCursorPos({30, ImGui::GetCursorPos().y});

            ImGui::PushID(i);
            if (strlen(recipe.acceptor->_tag) == 0)
            {
                ImGui::Selectable("<empty>");
            }
            else
            {
                ImGui::Selectable(recipe.acceptor->_tag);
            }
            ImGui::PopID();

            // Drag-and-drop support
            if (ImGui::BeginDragDropSource())
            {
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

    static void render_pipeline()
    {
        ImGui::Begin("Pipeline");

        if (isExperimenting()) {
            if (ImGui::Button("Stop Experiment")) {
                stopExperiment();
            }
        } else {
            if (ImGui::Button("Start Experiment")) {
                beginExperiment();
            }
        }

        if (isExperimenting()) {
            ImGui::BeginDisabled();
        }

        ImGui::TextDisabled("Configuration");

        std::vector<std::string> names;
        for (auto recipe : envs) {
            names.push_back(recipe.acceptor->_tag);
        }

        if (names.empty()) {
            names.push_back("<empty>");
        }

        auto data = Helpers::toImGuiList(names);

        if (ImGui::Combo("##<empty>", &PipelineConfig::activeEnv, data.first, data.second)) {
            // nothing to do here
        }

        delete[] data.first;

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
                auto &agent = PipelineConfig::pipelineAgents[i];

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
                            PipelineConfig::pipelineAgents.erase(PipelineConfig::pipelineAgents.begin() + i);
                            if (rename_agent_index == i)
                                rename_agent_index = -1;
                            ImGui::EndPopup();
                            ImGui::PopID();
                            break; // Exit now (one frame will be corrupted, but well it is what it is).
                        }

                        ImGui::EndPopup();
                    }

                    if (ImGui::IsItemHovered()) {
                        std::string id = "recipe_" + std::to_string(agent.recipe_index);
                        ImGui::BeginTooltip();
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
                        FontManager::pushFont("Light");
                        std::string py_type_name = std::string(agent.recipe->acceptor->_tag);
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

        if (PipelineConfig::pipelineMethods.empty())
        {
            ImGui::Text("<drag methods from recipes>");
        }
        else
        {
            for (int i = 0; i < PipelineConfig::pipelineMethods.size(); ++i)
            {
                ImGui::PushID(i + PipelineConfig::pipelineAgents.size());

                auto &method = PipelineConfig::pipelineMethods[i];

                // Handle rename input field
                if (rename_method_index == i)
                {
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::InputText("##rename", method.name, IM_ARRAYSIZE(method.name), ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        rename_method_index = -1; // Done renaming
                    }
                }
                else
                {
                    ImVec2 group_start = ImGui::GetCursorScreenPos();
                    ImGui::BeginGroup();

                    ImGui::Checkbox("##box", &method.active);
                    ImGui::SameLine();
                    ImGui::Text("%s", method.name);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(std::min(150, (int)ImGui::GetContentRegionAvail().x));
                    ImGui::SliderFloat("##slider", &method.weight, 0, 1, "Weight: %.2f");

                    ImGui::EndGroup();

                    ImVec2 group_end = ImGui::GetItemRectMax();
                    ImVec2 g_size(group_end.x - group_start.x, group_end.y - group_start.y);
                    ImGui::SetCursorScreenPos(group_start);
                    ImGui::InvisibleButton("##context_target", g_size);

                    // Right-click context menu
                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem("Rename"))
                        {
                            rename_method_index = i;
                        }
                        if (ImGui::MenuItem("Delete"))
                        {
                            PipelineConfig::pipelineMethods.erase(PipelineConfig::pipelineMethods.begin() + i);
                            if (rename_method_index == i)
                                rename_method_index = -1;
                            ImGui::EndPopup();
                            ImGui::PopID();
                            break; // Exit now (one frame will be corrupted, but well it is what it is).
                        }
                        ImGui::EndPopup();
                    }

                    if (ImGui::IsItemHovered())
                    {

                        std::string id = "recipe_" + std::to_string(method.recipe_index);
                        ImGui::BeginTooltip();
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
                        FontManager::pushFont("Light");
                        std::string py_type_name = std::string(method.recipe->acceptor->_tag);
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

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Recipes::Acceptor"))
            {
                int index = *static_cast<int *>(payload->Data);
                auto &recipe = recipes[index];
                if (recipe.type == PipelineGraph::Agent)
                {

                    PipelineAgent agent;
                    agent.recipe = &recipe;
                    agent.active = true;
                    strcpy(agent.name, recipe.acceptor->_tag);
                    agent.recipe_index = index;
                    PipelineConfig::pipelineAgents.push_back(agent);
                }
                else if (recipe.type == PipelineGraph::Method)
                {
                    PipelineMethod method;
                    method.recipe = &recipe;
                    method.active = true;
                    method.weight = 1.0;
                    strcpy(method.name, recipe.acceptor->_tag);
                    method.recipe_index = index;
                    PipelineConfig::pipelineMethods.push_back(method);
                }
                else
                {
                    Logger::warning("Drop Agents / Methods, You can select the environment from the drop down list.");
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (isExperimenting())
        {
            ImGui::EndDisabled();
        }

        ImGui::End();
    }

    void render()
    {
        render_recipes();
        render_pipeline();
    }

    void update() {
        if (PipelineState::experimentState == STOPPED) {
            // nothing
        }

        if (PipelineState::experimentState == INITIALIZING) {
            if (PipelineState::_agentsCountOk == PipelineState::activeAgents.size()) {
                // everyone is ready
                Logger::info("Experiment started with " + std::to_string(PipelineConfig::pipelineAgents.size()) + " agents and " +
                         std::to_string(PipelineConfig::pipelineMethods.size()) + " methods.");


                resetSim();
                Preview::onStart();

                PipelineState::_agentsCountOk = 0;
                PipelineState::experimentState = INITIALIZING_TEXTURES; // must be set AFTER resetSim(), because Agent depends on this call
                PipelineState::Simulating = false;
            } else if (PipelineState::_agentsCountOk + PipelineState::_agentsCountFailed == PipelineState::activeAgents.size()) {
                // someone failed
                Logger::error("Experiment failed to start, one or more agent's failed to initialize, stopping experiment ...");
                for (auto& agent: PipelineState::activeAgents) {
                    agent->_worker_stop = true;
                }
                PipelineState::experimentState = FAILED;
            }
        }

        if (PipelineState::experimentState == INITIALIZING_TEXTURES) {
            // wait for textures to be initialized
            if (PipelineState::_agentsCountOk == PipelineState::activeAgents.size()) {
                PipelineState::experimentState = RUNNING;
                PipelineState::Simulating = false;
                Logger::info("Experiment is running.");

                const char* windowName = "Preview";
                ImGuiWindow* window = ImGui::FindWindowByName(windowName);
                if (window)
                    ImGui::FocusWindow(window);
            }
        }


        if (PipelineState::experimentState == STOPPING || PipelineState::experimentState == FAILED) {
            bool _stopped = true;
            for (auto& agent: PipelineState::activeAgents) {
                if (agent->_worker_running) {
                    _stopped = false;
                    break;
                }
            }

            if (_stopped) {
                PipelineState::experimentState = STOPPED;

                // now clean everything
                PipelineState::Simulating = false;
                _clearActiveAgents();
                Logger::info("Experiment stopped.");
                Preview::onStop();
            }
        }

        if (PipelineState::experimentState == RUNNING) {
            for (auto& agent: PipelineState::activeAgents) {
                agent->steps_to_take += PipelineState::StepSimFrames;
            }
            PipelineState::StepSimFrames = 0;

            if (isSimRunning()) {
                for (auto& agent: PipelineState::activeAgents) {
                    agent->steps_to_take = 1;
                }
            }

            // each agent now has it's own worker that handles it's logic independently, we send signals to it to indicate the desired behavior
            // _update_agents_state(-1); // update all agents
        }
    }

    void destroy() {
        if (agents.size()) {
            py::gil_scoped_release release; // release GIL lock
            PipelineState::experimentState = STOPPING;
            for (auto& agent: PipelineState::activeAgents) {
                agent->_worker_stop = true;
            }

            while (PipelineState::experimentState != STOPPED) {
                update();
            }
        }
    }
}
