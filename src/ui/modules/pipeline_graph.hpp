//
// Created by xabdomo on 6/12/25.
//

#ifndef PIPELINE_GRAPH_HPP
#define PIPELINE_GRAPH_HPP


#include <imgui_node_editor.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include "../../backend/py_scope.hpp"
#include "../../ui/shared_ui.hpp"

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

        char _tag[256] = "<tag>";
        bool _editing_tag = false;

        virtual void init();
        virtual bool canExecute();
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
    void clearNode(ed::NodeId id);
    ed::LinkId addLink(ed::PinId inputPinId, ed::PinId outputPinId);
    void removeLink(ed::LinkId id);


    struct ObjectRecipe;

    // build objects recipes from Acceptor nodes and return a list of them
    // if fails due to cycles, will output on console, and return a list of valid recipes (if any)
    std::vector<ObjectRecipe> build();

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
        struct PrimitiveIntNode: virtual public Node {
            int    value{};
            int    rangeStart{};
            int    rangeEnd{};

            explicit PrimitiveIntNode(int val);
            PrimitiveIntNode(int val, int rangeStart, int rangeEnd);
            void exec() override;
            void render() override;
            ~PrimitiveIntNode() override;
        };

        struct PrimitiveFloatNode: virtual public Node {
            float    value{};
            float    rangeStart{};
            float    rangeEnd{};

            explicit PrimitiveFloatNode(float val);
            PrimitiveFloatNode(float val, float rangeStart, float rangeEnd);
            void exec() override;
            void render() override;
            ~PrimitiveFloatNode() override;
        };

        struct PrimitiveStringNode: virtual public Node {

            char _value[1024] = "";
            bool _file = false;

            explicit PrimitiveStringNode(bool file);
            explicit PrimitiveStringNode(const std::string& );
            void exec() override;
            void render() override;
            ~PrimitiveStringNode() override;
        };

        struct AdderNode: virtual public Node {

            explicit AdderNode();
            void exec() override;
            void render() override;
            ~AdderNode() override;
        };

        struct SingleOutputNode: virtual public Node { // nodes of this type, will only have one output
        };

        struct PythonModuleNode: virtual public SingleOutputNode {
            PyScope::LoadedModule* _type;
            std::string _name; // I have no idea why the SharedUi::LoadedModule->moduleName breaks

            explicit PythonModuleNode(PyScope::LoadedModule*);
            void exec() override;
            void render() override;
            ~PythonModuleNode() override;
        };

        struct PythonFunctionNode: virtual public SingleOutputNode {
            PyScope::LoadedModule* _type;
            std::string _name; // I have no idea why the SharedUi::LoadedModule->moduleName breaks
            bool _pointer;
            std::vector<Pin> _inputs;

            explicit PythonFunctionNode(PyScope::LoadedModule*);
            explicit PythonFunctionNode(PyScope::LoadedModule*, bool);
            void exec() override;
            void render() override;
            ~PythonFunctionNode() override;
        };

        struct AcceptorNode: public Node {
            py::object _result;
            explicit AcceptorNode();
            void exec() override;
            void render() override;
            ~AcceptorNode() override;
        };

        struct DeMuxNode: public Node {
            explicit DeMuxNode(int numOutputs);
            void exec() override;
            void render() override;
            ~DeMuxNode() override;
        };

        struct AgentAcceptorNode: public AcceptorNode {
            explicit AgentAcceptorNode();
        };

        struct EnvAcceptorNode: public AcceptorNode {
            explicit EnvAcceptorNode();
        };

        struct MaskAcceptorNode: public AcceptorNode {
            explicit MaskAcceptorNode();
        };

        struct MethodAcceptorNode: public AcceptorNode {
            explicit MethodAcceptorNode();
        };

    }

    enum RecipeType {
        Agent,
        Environment,
        Method
    };

    struct ObjectRecipe {
        std::vector<Node*> plan;
        Nodes::AcceptorNode* acceptor;
        RecipeType type;
        py::object create();
    };
};



#endif //PIPELINE_GRAPH_HPP
