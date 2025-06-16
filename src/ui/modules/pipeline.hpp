//
// Created by xabdomo on 6/16/25.
//

#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <vector>

#include "pipeline_graph.hpp"
#include "../../backend/py_agent.hpp"

namespace Pipeline {
    extern std::vector<PipelineGraph::ObjectRecipe> envs;
    extern std::vector<PipelineGraph::ObjectRecipe> agents;
    extern std::vector<PipelineGraph::ObjectRecipe> methods;

    struct ActiveAgent {
        char name[256]   = "Agent";
        PyAgent* agent   = nullptr;
        PyEnv* env       = nullptr;
        std::vector<PyMethod*> methods;
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
        extern std::vector<ActiveAgent> activeAgents;
    }

    // simulation control
    bool isRunning();
    void runSim();
    void stopSim();
    void pauseSim();

    void setRecipes(std::vector<PipelineGraph::ObjectRecipe>);
    void init();
    void render();
    void destroy();
}



#endif //PIPELINE_HPP
