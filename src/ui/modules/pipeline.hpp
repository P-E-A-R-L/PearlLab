#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <atomic>
#include <vector>

#include "pipeline_graph.hpp"

#include "../../backend/py_agent.hpp"
#include "../../backend/py_method.hpp"
#include "../../backend/py_env.hpp"
#include "../widgets/image_viewer.hpp"

class GLTexture;

namespace Pipeline
{
    extern std::vector<PipelineGraph::ObjectRecipe> envs;
    extern std::vector<PipelineGraph::ObjectRecipe> agents;
    extern std::vector<PipelineGraph::ObjectRecipe> methods;

    enum ActiveAgentState {
        IDLE,
        SELECTING_ACTION,
        STEPPING,
        RESET_ENV
    };

    struct ActiveAgent
    {
        char name[256] = "Agent";

        PyAgent *agent = nullptr;
        PyEnv   *env = nullptr;
        std::vector<PyMethod *> methods;


        std::mutex score_update_lock;

        // each item in this vector corresponds to a specific explainability method total score
        std::vector<float> scores_total;
        std::vector<float> scores_ep;

        struct StepsScores {
            // stores the score that a step produced for each step
            std::vector<float> scores;
        };

        // for each method, store separate series
        std::vector<StepsScores> steps_scores_ep;
        std::vector<StepsScores> steps_scores_total;

        StepsScores steps_rewards_total;
        StepsScores steps_rewards_ep;
        float reward_total = 0;
        float reward_ep = 0;

        int64_t steps_current_episode;
        int64_t total_episodes;
        int64_t total_steps;

        bool env_terminated;
        bool env_truncated;
        float last_move_reward = 0;

        // state variables
        std::mutex                    looper_lock;                // looper master lock,
                                                                  // agent worker will attempt to obtain this
                                                                  // lock at each loop iteration
        std::mutex                    available_actions_lock;     // this lock is only locked when the agent
                                                                  // worker updates `available_actions`

        std::atomic<size_t>           request_pause = 0;          // request the main looper to pause

        std::atomic<ActiveAgentState> state = IDLE;
        std::atomic<size_t>           next_action   = 0;
        std::atomic<size_t>           steps_to_take = 0;

        struct Action {
            std::string name;
            float probability;
        };

        std::vector<Action> available_actions;

        std::atomic<bool> _worker_stop    = false; // a signal to notify the worker to stop
        std::atomic<bool> _worker_running = true;  // is a worker running for this agent?

    };

    struct TextureBuffer {
        std::vector<unsigned char> data;
        int width;
        int height;
        int channels;
        bool valid = true;
    };

    struct VisualizedObject
    {
        PyVisualizable *visualizable;

        GLTexture *rgb_array                    = nullptr;
        GLTexture *gray                         = nullptr;
        GLTexture *heat_map                     = nullptr;

        TextureBuffer rgb_array_buffer;
        TextureBuffer gray_buffer;
        TextureBuffer heat_map_buffer;

        ImageViewer* rgb_array_viewer = nullptr;
        ImageViewer* gray_viewer      = nullptr;
        ImageViewer* heat_map_viewer  = nullptr;

        std::map<std::string, float>       bar_chart;
        std::map<std::string, std::string> features;

        PyLiveObject *rgb_array_params      = nullptr;
        PyLiveObject *gray_params           = nullptr;
        PyLiveObject *heat_map_params       = nullptr;
        PyLiveObject *features_params       = nullptr;
        PyLiveObject *bar_chart_params      = nullptr;

        void init(PyVisualizable *);
        [[nodiscard]] bool supports(VisualizationMethod method) const;

        // update buffers & data [worker thread]
        void update();

        // update textures only [ui thread]
        void update_tex();

        std::mutex* m_lock = nullptr;

        ~VisualizedObject();

    private:
        void _init_rgb_array();
        void _update_rgb_array();

        void _init_gray();
        void _update_gray();

        void _init_heat_map();
        void _update_heat_map();

        void _init_features();
        void _update_features();

        void _init_bar_chart();
        void _update_bar_chart();
    };

    struct VisualizedAgent
    {
        Pipeline::ActiveAgent *agent;
        int agent_index;
        VisualizedObject      *env_visualization = nullptr;
        std::vector<VisualizedObject *> method_visualizations;

        void init(Pipeline::ActiveAgent *agent, int agent_index);

        void update() const;
        void update_tex() const;

        ~VisualizedAgent();
    };

    struct PipelineAgent
    {
        char name[256] = "Method";
        PipelineGraph::ObjectRecipe *recipe{};
        bool active = true;
        int recipe_index = 0;
    };

    struct PipelineMethod
    {
        char name[256] = "Agent";
        PipelineGraph::ObjectRecipe *recipe{};
        float weight = 1;
        bool active = true;
        int recipe_index = 0;
    };

    namespace PipelineConfig
    {
        // all variables such as
        //   policy
        //   steps
        //   maxEp
        // etc .. will be stored here

        extern int maxSteps;    // default max steps for an agent
        extern int maxEpisodes; // default max episodes for an agent
        extern int activeEnv;   // the index of the current active env

        extern std::vector<PipelineAgent> pipelineAgents;
        extern std::vector<PipelineMethod> pipelineMethods;
    }

    enum ExperimentState {
        STOPPED,
        INITIALIZING,
        INITIALIZING_TEXTURES,
        FAILED,
        RUNNING,
        STOPPING,
    };

    namespace PipelineState
    {
        extern std::atomic<ExperimentState> experimentState;

        extern std::mutex agentsLock;

        enum StepPolicy
        {
            RANDOM, // each agent steps randomly
            // RANDOM_UNIFORM,  // steps randomly but all agents does the same
            BEST_AGENT,  // the best agent is followed by all other agents
            WORST_AGENT, // the worst agent is followed by all other agents
            INDEPENDENT, // each agent takes it's action independently based on it's decision
        };

        enum ScorePolicy
        {
            PEARL,  // Pearl score is what determines the criteria
            REWARD, // agent's reward is what determines the criteria
        };

        extern StepPolicy stepPolicy;
        extern ScorePolicy scorePolicy;

        extern std::vector<ActiveAgent*> activeAgents;
        extern std::vector<VisualizedAgent *> activeVisualizations;
    }

    // simulation control
    // returns true if the experiment is running
    bool isExperimenting();

    // builds the experiment, and starts it
    // - create instances of Environments, Agents and methods
    // - starts a simulation in paused mode
    void beginExperiment();

    // pauses the simulation if running, and stops the experiment (destroys objects!)
    void stopExperiment();


    // void resetExperiment();

    // step simulation for one agent
    void stepSim(int agent_index);
    void stepSim(int agent_index, int action_index);
    void resetEnv(int agent_index);

    float evalAgent(int agent_index);

    void setRecipes(std::vector<PipelineGraph::ObjectRecipe>);
    void init();
    void render();

    // called before render
    void update();

    void destroy();
}

#endif // PIPELINE_HPP
