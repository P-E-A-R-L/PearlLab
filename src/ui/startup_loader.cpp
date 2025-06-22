#include "startup_loader.hpp"

#include <vector>
#include <string>
#include <map>

#include "shared_ui.hpp"
#include "../backend/py_scope.hpp"
#include "modules/logger.hpp"
#include "modules/pipeline_graph.hpp"

// #define LOAD_INITIAL

static std::map<std::string, PyScope::LoadedModule*> modules;

void StartupLoader::load_modules() {
#ifndef LOAD_INITIAL
    if (true) return;
#endif

    std::vector<std::string> filePaths = {
        "./py/test_lunarlander.py",
    };

    for (const auto& filePath : filePaths) {
        auto result = PyScope::LoadModuleForClasses(filePath);
        for (auto& obj : result) {
            SharedUi::pushModule(obj);
            Logger::info("Loaded module: " + std::string(py::str(obj.attr("__name__"))));
        }
    }

    std::map<std::string, std::string> mapping;
    mapping.insert({"test_lunarlander.REINFORCE_Net", "REINFORCE_Net"});
    mapping.insert({"test_lunarlander.cudaDevice", "Device"});
    mapping.insert({"test_lunarlander.LunarWrapper", "LunarWrapper"});
    mapping.insert({"pearl.agents.TorchPolicy.TorchPolicyAgent", "TorchPolicyAgent"});
    mapping.insert({"pearl.enviroments.GymRLEnv.GymRLEnv", "GymRLEnv"});
    mapping.insert({"pearl.provided.LunarLander.LunarLanderTabularMask", "LunarLanderTabularMask"});
    mapping.insert({"pearl.methods.TabularShapExplainability.TabularShapExplainability", "TabularShapExplainability"});
    mapping.insert({"pearl.methods.TabularLimeExplainability.TabularLimeExplainability", "TabularLimeExplainability"});

    for (auto& mod : SharedUi::loadedModules) {
        auto it = mapping.find(mod.moduleName);
        if (it != mapping.end()) {
            modules[it->second] = &mod;
            Logger::info("Mapped module: " + it->second + " to " + mod.moduleName);
        }
    }
}

void StartupLoader::load_graph() {
#ifndef LOAD_INITIAL
    if (true) return;
#endif

    // Device
    auto deviceNode = new PipelineGraph::Nodes::PythonFunctionNode(modules["Device"]);
    PipelineGraph::addNode(deviceNode);

    // Mask + Explainability
    auto maskNode = new PipelineGraph::Nodes::PythonModuleNode(modules["LunarLanderTabularMask"]);
    auto limeNode = new PipelineGraph::Nodes::PythonModuleNode(modules["TabularLimeExplainability"]);
    auto shapNode = new PipelineGraph::Nodes::PythonModuleNode(modules["TabularShapExplainability"]);
    auto limeAcceptor = new PipelineGraph::Nodes::MethodAcceptorNode();
    auto shapAcceptor = new PipelineGraph::Nodes::MethodAcceptorNode();
    auto featureNames = new PipelineGraph::Nodes::PrimitiveStringNode(std::string("x,y,vx,vy,angle,angular_velocity,left_leg,right_leg"));

    PipelineGraph::addNode(maskNode);
    PipelineGraph::addNode(limeNode);
    PipelineGraph::addNode(shapNode);
    PipelineGraph::addNode(limeAcceptor);
    PipelineGraph::addNode(shapAcceptor);
    PipelineGraph::addNode(featureNames);

    PipelineGraph::addLink(limeNode->inputs[0].id, deviceNode->outputs[0].id);
    PipelineGraph::addLink(limeNode->inputs[1].id, maskNode->outputs[0].id);
    PipelineGraph::addLink(limeNode->inputs[2].id, featureNames->outputs[0].id); 
    PipelineGraph::addLink(limeAcceptor->inputs[0].id, limeNode->outputs[0].id);

    PipelineGraph::addLink(shapNode->inputs[0].id, deviceNode->outputs[0].id);
    PipelineGraph::addLink(shapNode->inputs[1].id, maskNode->outputs[0].id);
    PipelineGraph::addLink(shapNode->inputs[2].id, featureNames->outputs[0].id);
    PipelineGraph::addLink(shapAcceptor->inputs[0].id, shapNode->outputs[0].id);

    // Environment
    auto envName = new PipelineGraph::Nodes::PrimitiveStringNode(std::string("LunarLander-v3"));
    auto gymEnv  = new PipelineGraph::Nodes::PythonModuleNode(modules["GymRLEnv"]);
    auto wrapper = new PipelineGraph::Nodes::PythonModuleNode(modules["LunarWrapper"]);
    auto renderModeNode = new PipelineGraph::Nodes::PrimitiveStringNode(std::string("rgb_array")); 
    auto envAcceptor = new PipelineGraph::Nodes::EnvAcceptorNode();
    auto int1Node = new PipelineGraph::Nodes::PrimitiveIntNode(1);
    
    PipelineGraph::addNode(envName);
    PipelineGraph::addNode(gymEnv);
    PipelineGraph::addNode(wrapper);
    PipelineGraph::addNode(envAcceptor);
    PipelineGraph::addNode(renderModeNode);
    PipelineGraph::addNode(int1Node);

    PipelineGraph::addLink(gymEnv->inputs[0].id, envName->outputs[0].id);      // env_name
    PipelineGraph::addLink(gymEnv->inputs[1].id, int1Node->outputs[0].id);      // stack_size
    PipelineGraph::addLink(gymEnv->inputs[2].id, int1Node->outputs[0].id);      // frame_skip
    PipelineGraph::addLink(gymEnv->inputs[3].id, renderModeNode->outputs[0].id);      // num_envs
    PipelineGraph::addLink(gymEnv->inputs[13].id, int1Node->outputs[0].id); // num_envs = 1
    PipelineGraph::addLink(gymEnv->inputs[14].id, int1Node->outputs[0].id); // tabular = True

    PipelineGraph::addLink(wrapper->inputs[0].id, gymEnv->outputs[0].id);
    PipelineGraph::addLink(envAcceptor->inputs[0].id, wrapper->outputs[0].id);

    // Agent setup
    auto inputDimNode  = new PipelineGraph::Nodes::PrimitiveIntNode(8);
    auto actionDimNode = new PipelineGraph::Nodes::PrimitiveIntNode(4);
    auto reinforceNet  = new PipelineGraph::Nodes::PythonModuleNode(modules["REINFORCE_Net"]);

    auto agent1PathNode = new PipelineGraph::Nodes::PrimitiveStringNode(std::string("./py/models/lunar_lander_reinforce_2000.pth"));
    auto agent2PathNode = new PipelineGraph::Nodes::PrimitiveStringNode(std::string("./py/models/lunar_lander_reinforce_500.pth"));

    auto agent1Node = new PipelineGraph::Nodes::PythonModuleNode(modules["TorchPolicyAgent"]);
    auto agent2Node = new PipelineGraph::Nodes::PythonModuleNode(modules["TorchPolicyAgent"]);

    auto agent1Acceptor = new PipelineGraph::Nodes::AgentAcceptorNode();
    auto agent2Acceptor = new PipelineGraph::Nodes::AgentAcceptorNode();

    PipelineGraph::addNode(inputDimNode);
    PipelineGraph::addNode(actionDimNode);
    PipelineGraph::addNode(reinforceNet);

    PipelineGraph::addLink(reinforceNet->inputs[0].id, inputDimNode->outputs[0].id);
    PipelineGraph::addLink(reinforceNet->inputs[1].id, actionDimNode->outputs[0].id);

    // Good agent
    PipelineGraph::addNode(agent1PathNode);
    PipelineGraph::addNode(agent1Node);
    PipelineGraph::addNode(agent1Acceptor);

    PipelineGraph::addLink(agent1Node->inputs[0].id, agent1PathNode->outputs[0].id);
    PipelineGraph::addLink(agent1Node->inputs[1].id, reinforceNet->outputs[0].id);
    PipelineGraph::addLink(agent1Node->inputs[2].id, deviceNode->outputs[0].id);
    PipelineGraph::addLink(agent1Acceptor->inputs[0].id, agent1Node->outputs[0].id);

    // Bad agent
    PipelineGraph::addNode(agent2PathNode);
    PipelineGraph::addNode(agent2Node);
    PipelineGraph::addNode(agent2Acceptor);

    PipelineGraph::addLink(agent2Node->inputs[0].id, agent2PathNode->outputs[0].id);
    PipelineGraph::addLink(agent2Node->inputs[1].id, reinforceNet->outputs[0].id);
    PipelineGraph::addLink(agent2Node->inputs[2].id, deviceNode->outputs[0].id);
    PipelineGraph::addLink(agent2Acceptor->inputs[0].id, agent2Node->outputs[0].id);

    // Labels
    strcpy(envAcceptor->_tag, "LunarLander TabularEnv");
    strcpy(agent1Acceptor->_tag, "REINFORCE Agent (2000)");
    strcpy(agent2Acceptor->_tag, "REINFORCE Agent (500)");
    strcpy(limeAcceptor->_tag, "TabularLIME");
    strcpy(shapAcceptor->_tag, "TabularSHAP");
}
