//
// Created by xabdomo on 6/16/25.
//

#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <vector>

#include "pipeline_graph.hpp"

#include "../../backend/py_agent.hpp"
#include "../../backend/py_method.hpp"
#include "../../backend/py_env.hpp"

namespace Pipeline {
    extern std::vector<PipelineGraph::ObjectRecipe> envs;
    extern std::vector<PipelineGraph::ObjectRecipe> agents;
    extern std::vector<PipelineGraph::ObjectRecipe> methods;

    struct ActiveAgent {
        char name[256]   = "Agent";
        PyAgent* agent   = nullptr;
        PyEnv* env       = nullptr;
        std::vector<PyMethod*> methods;
        std::vector<float>     scores;
        float reward     = 0;
    };

    struct PipelineAgent {
        char name[256]            = "Method";
        PipelineGraph::ObjectRecipe* recipe;
        bool active = true;
        int recipe_index = 0;
    };

    struct PipelineMethod {
        char name[256]            = "Agent";
        PipelineGraph::ObjectRecipe* recipe;
        float weight = 1;
        bool active = true;
        int recipe_index = 0;
    };

    namespace PipelineConfig {
        // all variables such as
        //   policy
        //   steps
        //   maxEp
        // etc .. will be stored here

        extern int maxSteps;    // default max steps for an agent
        extern int maxEpisodes; // default max episodes for an agent
        extern int activeEnv;   // the index of the current active env

        extern std::vector<PipelineAgent>  pipelineAgents;
        extern std::vector<PipelineMethod> pipelineMethods;
    }

    namespace PipelineState {
        extern bool Experimenting;
        extern bool Simulating;
        extern int  StepSimFrames;

        extern std::vector<ActiveAgent> activeAgents;
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


    // returns either the simulation is running freely or not
    bool isSimRunning();

    void pauseSim();
    void continueSim();
    void stepSim();
    void resetSim();

    void setRecipes(std::vector<PipelineGraph::ObjectRecipe>);
    void init();
    void render();
    void destroy();
}



#endif //PIPELINE_HPP
