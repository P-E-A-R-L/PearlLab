//
// Created by xabdomo on 6/12/25.
//

#ifndef PIPELINE_GRAPH_HPP
#define PIPELINE_GRAPH_HPP


#include <imgui_node_editor.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

namespace ed = ax::NodeEditor;
namespace py = pybind11;

namespace PipelineGraph {

    enum PinType {
        INPUT,
        OUTPUT,
        PRIMITIVE_INT,
        PRIMITIVE_FLOAT,
        PRIMITIVE_STRING
    };

    union PinValue {
        std::string* str;
        float* f;
        int* i;
    };

    struct Pin {
        ed::PinId   id;
        std::string name;
        std::string tooltip;

        PinType     type;
        py::object* py_type;
        PinValue    value; // in case the pin is a primitive type
    };

    struct Node {
        ed::NodeId id;
        std::string name;
        std::string disc;
        std::vector<Pin> inputs;
        std::vector<Pin> outputs;
    };

    struct Link {
        ed::LinkId id;
        ed::PinId inputPinId;
        ed::PinId outputPinId;
    };

    ed::NodeId addNode(Node*);
    void removeNode(ed::NodeId id);
    ed::LinkId addLink(ed::PinId inputPinId, ed::PinId outputPinId);
    void removeLink(ed::LinkId id);
    Node* getNode(ed::NodeId id);
    Link* getLink(ed::LinkId id);

    extern std::vector<PipelineGraph::Node> nodes;
    extern std::vector<PipelineGraph::Link> links;

    void init();
    void render();
    void destroy();
};



#endif //PIPELINE_GRAPH_HPP
