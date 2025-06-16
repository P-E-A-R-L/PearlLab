//
// Created by xabdomo on 6/16/25.
//

#include "pipeline.hpp"

namespace Pipeline {
    std::vector<PipelineGraph::ObjectRecipe> recipes;

    void init() {
        recipes.clear();
    }

    static void render_recipes() {
        ImGui::Begin("Recipes");

        int visibleCount = 0;
        for (int i = 0; i < recipes.size(); ++i) {
            auto& recipe = recipes[i];

            std::string typeName = "A";
            if (dynamic_cast<PipelineGraph::Nodes::EnvAcceptorNode*>(recipe.acceptor)) typeName = "E";
            if (dynamic_cast<PipelineGraph::Nodes::MethodAcceptorNode*>(recipe.acceptor)) typeName = "M";

            ImGui::Text(typeName.c_str());
            ImGui::SameLine();

            ImGui::SetCursorPos({30, ImGui::GetCursorPos().y});

            if (strlen(recipe.acceptor->_tag) == 0) {
                ImGui::Selectable("<empty>");
            } else {
                ImGui::Selectable(recipe.acceptor->_tag);
            }

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

    void render() {
        render_recipes();
    }

    void destroy() {
        recipes.clear();
    }
}

