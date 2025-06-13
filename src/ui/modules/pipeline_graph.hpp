//
// Created by xabdomo on 6/12/25.
//

#ifndef PIPELINE_GRAPH_HPP
#define PIPELINE_GRAPH_HPP


#include <imgui_node_editor.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

namespace SharedUi {
    struct LoadedModule;
}

namespace ed = ax::NodeEditor;
namespace py = pybind11;

namespace PipelineGraph {

    union PrimitiveValue {
        std::string* str;
        float* f;
        int* i;
    };

    enum PinDirection {
        INPUT,
        OUTPUT
    };

    struct Pin {
        ed::PinId   id;
        std::string name;
        std::string tooltip;

        PinDirection direction;

        py::object type  = py::none();      // the output / input type of this node
        py::object value = py::none();
    };

    struct Node {
        ed::NodeId id;
        std::string name;
        std::string disc;
        std::vector<Pin> inputs;
        std::vector<Pin> outputs;

        bool _executed = false;

        virtual void init();
        bool canExecute();
        virtual void exec() = 0;
        virtual void render() = 0;
        virtual ~Node() = default;
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

    extern std::vector<PipelineGraph::Node*> nodes;
    extern std::vector<PipelineGraph::Link*> links;

    extern std::unordered_map<ed::NodeId, Node*> nodeLookup;
    extern std::unordered_map<ed::PinId , std::vector<Link*>> pinLinkLookup;
    extern std::unordered_map<ed::PinId , Node*> pinNodeLookup;
    extern std::unordered_map<ed::LinkId, Link*> linkLookup;

    int GetNextId();

    void init();
    void render();
    void destroy();

    namespace Nodes {
        struct PrimitiveIntNode: public Node {
            bool _editing = false;
            char _buffer[128] = "value";

            int    value{};
            int    rangeStart{};
            int    rangeEnd{};

            explicit PrimitiveIntNode(int val);
            PrimitiveIntNode(int val, int rangeStart, int rangeEnd);
            void exec() override;
            void render() override;
            ~PrimitiveIntNode() override;
        };

        struct PrimitiveFloatNode: public Node {
            bool _editing = false;
            char _buffer[128] = "value";

            float    value{};
            float    rangeStart{};
            float    rangeEnd{};

            explicit PrimitiveFloatNode(float val);
            PrimitiveFloatNode(float val, float rangeStart, float rangeEnd);
            void exec() override;
            void render() override;
            ~PrimitiveFloatNode() override;
        };

        struct PrimitiveStringNode: public Node {
            bool _editing = false;
            char _buffer[128] = "value";

            char _value[1024] = "";
            bool _file = false;


            explicit PrimitiveStringNode(bool file);
            void exec() override;
            void render() override;
            ~PrimitiveStringNode() override;
        };

        struct AdderNode: public Node {

            explicit AdderNode();
            void exec() override;
            void render() override;
            ~AdderNode() override;
        };

        struct PythonModuleNode: public Node {
            SharedUi::LoadedModule* _type;
            std::string _name; // I have no idea why the SharedUi::LoadedModule->moduleName breaks

            explicit PythonModuleNode(SharedUi::LoadedModule*);
            void exec() override;
            void render() override;
            ~PythonModuleNode() override;
        };

        struct PythonFunctionNode: public Node {
            SharedUi::LoadedModule* _type;
            std::string _name; // I have no idea why the SharedUi::LoadedModule->moduleName breaks

            explicit PythonFunctionNode(SharedUi::LoadedModule*);
            void exec() override;
            void render() override;
            ~PythonFunctionNode() override;
        };
    }
};



#endif //PIPELINE_GRAPH_HPP
