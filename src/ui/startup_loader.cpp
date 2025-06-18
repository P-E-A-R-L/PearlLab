//
// Created by xabdomo on 6/18/25.
//

#include "startup_loader.hpp"

#include <vector>
#include <string>

#include "shared_ui.hpp"
#include "../backend/py_scope.hpp"
#include "modules/logger.hpp"
#include "modules/pipeline_graph.hpp"


static std::map<std::string, PyScope::LoadedModule*> modules;

void StartupLoader::load_modules() {
    // modules loading
    std::vector<std::string> filePaths = {
        "./py/test_pearl.py",
    };

    for (auto filePath : filePaths) {
        auto result = PyScope::LoadModuleForClasses(filePath);
        for (auto obj: result) {
            SharedUi::pushModule(obj);
            Logger::info("Loaded module: " + std::string(py::str(obj.attr("__name__"))));
        }
    }

    // graph building
    std::map<std::string, std::string> mapping;
    mapping.insert({"test_pearl.DQN"                                     , "DQN"});
    mapping.insert({"test_pearl.preprocess"                              , "Preprocess"});
    mapping.insert({"test_pearl.Wrapper"                                 , "Wrapper"});
    mapping.insert({"test_pearl.cudaDevice"                              , "Device"});
    mapping.insert({"pearl.agents.TourchDQN.TorchDQN"                    , "TorchDQN"});
    mapping.insert({"pearl.enviroments.GymRLEnv.GymRLEnv"                , "GymRLEnv"});
    mapping.insert({"pearl.provided.AssaultEnv.AssaultEnvShapMask"       , "AssaultEnvShapMask"});
    mapping.insert({"pearl.methods.ShapExplainability.ShapExplainability", "ShapExplainability"});


    for (int i = 0;i < SharedUi::loadedModules.size();i++) {
        auto& mod = SharedUi::loadedModules[i];
        auto it = mapping.find(mod.moduleName);
        if (it != mapping.end()) {
            modules[it->second] = &SharedUi::loadedModules[i];
            Logger::info("Mapped module: " + it->second + " to " + mod.moduleName);
        }
    }
}

void StartupLoader::load_graph() {
    // graph - adding recipes

    // device
    auto device   = new PipelineGraph::Nodes::PythonFunctionNode(modules["Device"]);
    PipelineGraph::addNode(device);

    // mask
    auto maskNode = new PipelineGraph::Nodes::PythonModuleNode(modules["AssaultEnvShapMask"]);
    auto shapNode = new PipelineGraph::Nodes::PythonModuleNode(modules["ShapExplainability"]);
    auto methodAcceptor = new PipelineGraph::Nodes::MethodAcceptorNode();

    PipelineGraph::addNode(maskNode);
    PipelineGraph::addNode(shapNode);
    PipelineGraph::addNode(methodAcceptor);

    PipelineGraph::addLink(shapNode->inputs[1].id, maskNode->outputs[0].id);
    PipelineGraph::addLink(shapNode->inputs[0].id, device->outputs[0].id);
    PipelineGraph::addLink(methodAcceptor->inputs[0].id, shapNode->outputs[0].id);

    // env
    auto gymEnv         = new PipelineGraph::Nodes::PythonModuleNode(modules["GymRLEnv"]);
    auto int4Node       = new PipelineGraph::Nodes::PrimitiveIntNode(4);
    auto envNameNode    = new PipelineGraph::Nodes::PrimitiveStringNode(std::string("ALE/Assault-v5"));
    auto renderModeNode = new PipelineGraph::Nodes::PrimitiveStringNode(std::string("rgb_array"));
    auto preprocessNode = new PipelineGraph::Nodes::PythonFunctionNode(modules["Preprocess"], true);
    auto wrapperNode    = new PipelineGraph::Nodes::PythonModuleNode(modules["Wrapper"]);
    auto envAcceptor    = new PipelineGraph::Nodes::EnvAcceptorNode();
    auto int1Node       = new PipelineGraph::Nodes::PrimitiveIntNode(1);

    PipelineGraph::addNode(gymEnv);
    PipelineGraph::addNode(int4Node);
    PipelineGraph::addNode(envNameNode);
    PipelineGraph::addNode(renderModeNode);
    PipelineGraph::addNode(preprocessNode);
    PipelineGraph::addNode(wrapperNode);
    PipelineGraph::addNode(envAcceptor);
    PipelineGraph::addNode(int1Node);

    PipelineGraph::addLink(gymEnv->inputs[0].id, envNameNode->outputs[0].id);
    PipelineGraph::addLink(gymEnv->inputs[1].id, int4Node->outputs[0].id);
    PipelineGraph::addLink(gymEnv->inputs[2].id, int4Node->outputs[0].id);
    PipelineGraph::addLink(gymEnv->inputs[3].id, renderModeNode->outputs[0].id);
    PipelineGraph::addLink(gymEnv->inputs[7].id, preprocessNode->outputs[0].id);
    PipelineGraph::addLink(gymEnv->inputs[13].id, int1Node->outputs[0].id);

    PipelineGraph::addLink(wrapperNode->inputs[0].id, gymEnv->outputs[0].id);
    PipelineGraph::addLink(envAcceptor->inputs[0].id, wrapperNode->outputs[0].id);


    // Agents
    auto int7Node       = new PipelineGraph::Nodes::PrimitiveIntNode(7);

    auto DQNNode        = new PipelineGraph::Nodes::PythonModuleNode(modules["DQN"]);

    auto agent1PathNode  = new PipelineGraph::Nodes::PrimitiveStringNode(std::string("./py/models/models/dqn_assault_5m.pth"));
    auto agent1TorchNode = new PipelineGraph::Nodes::PythonModuleNode(modules["TorchDQN"]);
    auto agent1Acceptor  = new PipelineGraph::Nodes::AgentAcceptorNode();

    auto agent2PathNode  = new PipelineGraph::Nodes::PrimitiveStringNode(std::string("./py/models/models/dqn_assault_1k.pth"));
    auto agent2TorchNode = new PipelineGraph::Nodes::PythonModuleNode(modules["TorchDQN"]);
    auto agent2Acceptor  = new PipelineGraph::Nodes::AgentAcceptorNode();

    PipelineGraph::addNode(int7Node);
    PipelineGraph::addNode(DQNNode);

    PipelineGraph::addNode(agent1PathNode);
    PipelineGraph::addNode(agent1TorchNode);
    PipelineGraph::addNode(agent1Acceptor);

    PipelineGraph::addNode(agent2PathNode);
    PipelineGraph::addNode(agent2TorchNode);
    PipelineGraph::addNode(agent2Acceptor);

    PipelineGraph::addLink(DQNNode->inputs[0].id, int7Node->outputs[0].id);

    PipelineGraph::addLink(agent1TorchNode->inputs[0].id, agent1PathNode->outputs[0].id);
    PipelineGraph::addLink(agent1TorchNode->inputs[1].id, DQNNode->outputs[0].id);
    PipelineGraph::addLink(agent1TorchNode->inputs[2].id, device->outputs[0].id);
    PipelineGraph::addLink(agent1Acceptor->inputs[0].id, agent1TorchNode->outputs[0].id);

    PipelineGraph::addLink(agent2TorchNode->inputs[0].id, agent2PathNode->outputs[0].id);
    PipelineGraph::addLink(agent2TorchNode->inputs[1].id, DQNNode->outputs[0].id);
    PipelineGraph::addLink(agent2TorchNode->inputs[2].id, device->outputs[0].id);
    PipelineGraph::addLink(agent2Acceptor->inputs[0].id, agent2TorchNode->outputs[0].id);

    // Some QoL
    std::string methodTag = "ShapExplainability";
    std::string agent1Tag = "TorchDQN (5m)";
    std::string agent2Tag = "TorchDQN (1k)";
    std::string envTag    = "GymRLEnv (Assault)";

    strcpy(envAcceptor->_tag, envTag.c_str());
    strcpy(methodAcceptor->_tag, methodTag.c_str());
    strcpy(agent1Acceptor->_tag, agent1Tag.c_str());
    strcpy(agent2Acceptor->_tag, agent2Tag.c_str());
}
