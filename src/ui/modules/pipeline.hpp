//
// Created by xabdomo on 6/16/25.
//

#ifndef PIPELINE_HPP
#define PIPELINE_HPP
#include <vector>

#include "pipeline_graph.hpp"


namespace Pipeline {
    extern std::vector<PipelineGraph::ObjectRecipe> recipes;

    void init();
    void render();
    void destroy();
}



#endif //PIPELINE_HPP
