//
// Created by xabdomo on 6/22/25.
//

#include "project_manager.hpp"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

#include "shared_ui.hpp"
#include "../backend/py_safe_wrapper.hpp"
#include "modules/logger.hpp"
#include "modules/pipeline_graph.hpp"

namespace fs = std::filesystem;

static std::string ReadFileToString(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);

    if (!file) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

ProjectManager::ProjectDetails ProjectManager::loadProject(const std::string &p_path) {
    ProjectDetails projectDetails;
    fs::path base_path = p_path;
    projectDetails.graph_file = base_path / std::string("graph.json");
    projectDetails.imgui_file = base_path / std::string("imgui.ini");
    projectDetails.project_path = base_path.string();

    fs::path graph_details = base_path / std::string("graph.json.d");
    fs::path modules_file  = base_path / "modules.json";

    if (!fs::exists(modules_file)) {
        Logger::error("Didn't find modules.json in the project directory: " + base_path.string());
    } else {
        // now load modules
        if (!SafeWrapper::execute([&] {
            auto modules_json = ReadFileToString(modules_file.string());
            auto modules_data = nlohmann::json::parse(modules_json);

            for (std::string module : modules_data) {
                auto result = PyScope::LoadModuleForClasses(module);
                for (auto& obj : result) {
                    SharedUi::pushModule(obj);
                    Logger::info("Loaded module: " + std::string(py::str(obj.attr("__name__"))));
                }
            }
        })) {
            Logger::error("Failed to load modules from: " + modules_file.string());
        }
    }

    if (!fs::exists(graph_details)) {
        Logger::error("Didn't find graph details file: " + graph_details.string());
    } else {
        // load graph
        if (!SafeWrapper::execute([&] {
            auto graph_json = ReadFileToString(graph_details.string());
            auto graph_data = nlohmann::json::parse(graph_json);
            auto nodes_data = graph_data["nodes"];
            auto links_data = graph_data["links"];
            auto next_id = graph_data["next_id"].get<size_t>();

            for (auto node: nodes_data) {
                std::string type = node["type"];
                auto        data   = node["data"];

                if (type == "PipelineGraph.Nodes.PrimitiveIntNode") {
                    auto* primitiveNode = new PipelineGraph::Nodes::PrimitiveIntNode();
                    primitiveNode->load(data);
                    PipelineGraph::addNode(primitiveNode);
                } else if (type == "PipelineGraph.Nodes.PrimitiveFloatNode") {
                    auto* pythonModuleNode = new PipelineGraph::Nodes::PrimitiveFloatNode();
                    pythonModuleNode->load(data);
                    PipelineGraph::addNode(pythonModuleNode);
                } else if (type == "PipelineGraph.Nodes.PrimitiveStringNode") {
                    auto* pythonModuleNode = new PipelineGraph::Nodes::PrimitiveStringNode();
                    pythonModuleNode->load(data);
                    PipelineGraph::addNode(pythonModuleNode);
                } else if (type == "PipelineGraph.Nodes.PythonModuleNode") {
                    auto* pythonModuleNode = new PipelineGraph::Nodes::PythonModuleNode();
                    pythonModuleNode->load(data);
                    PipelineGraph::addNode(pythonModuleNode);
                } else if (type == "PipelineGraph.Nodes.PythonFunctionNode") {
                    auto* pythonFunctionNode = new PipelineGraph::Nodes::PythonFunctionNode();
                    pythonFunctionNode->load(data);
                    PipelineGraph::addNode(pythonFunctionNode);
                } else if (type == "PipelineGraph.Nodes.AgentAcceptorNode") {
                    auto* acceptorNode = new PipelineGraph::Nodes::AgentAcceptorNode();
                    acceptorNode->load(data);
                    PipelineGraph::addNode(acceptorNode);
                } else if (type == "PipelineGraph.Nodes.MethodAcceptorNode") {
                    auto* acceptorNode = new PipelineGraph::Nodes::MethodAcceptorNode();
                    acceptorNode->load(data);
                    PipelineGraph::addNode(acceptorNode);
                } else if (type == "PipelineGraph.Nodes.EnvAcceptorNode") {
                    auto* acceptorNode = new PipelineGraph::Nodes::EnvAcceptorNode();
                    acceptorNode->load(data);
                    PipelineGraph::addNode(acceptorNode);
                } else if (type == "PipelineGraph.Nodes.MaskAcceptorNode") {
                    auto* acceptorNode = new PipelineGraph::Nodes::MaskAcceptorNode();
                    acceptorNode->load(data);
                    PipelineGraph::addNode(acceptorNode);
                } else {
                    Logger::error("Unknown node type: " + type);
                }
            }

            for (auto link: links_data) {
                auto src_pin = link["src"].get<size_t>();
                auto dst_pin = link["dst"].get<size_t>();
                PipelineGraph::addLink(dst_pin, src_pin);
            }

            PipelineGraph::nextId = next_id;
        })) {
            Logger::error("Failed to load graph from: " + graph_details.string());
        }
    }

    return projectDetails;
}

bool ProjectManager::saveProject(const std::string &path) {

    fs::path base_path = path;
    if (!fs::exists(base_path)) {
        Logger::error("Project path does not exist: " + base_path.string());
        return false;
    }

    fs::path graph_file = base_path / "graph.json";
    fs::path imgui_file = base_path / "imgui.ini";
    fs::path graph_details = base_path / "graph.json.d";
    fs::path modules_file  = base_path / "modules.json";

    ImGui::SaveIniSettingsToDisk(imgui_file.string().c_str());

    nlohmann::json graph_json;
    nlohmann::json nodes_data;
    nlohmann::json links_data;

    for (auto node: PipelineGraph::nodes) {
        nlohmann::json node_data;
        node->save(node_data);

        std::string node_type;
        if (dynamic_cast<PipelineGraph::Nodes::PrimitiveIntNode*>(node)) {
            node_type = "PipelineGraph.Nodes.PrimitiveIntNode";
        } else if (dynamic_cast<PipelineGraph::Nodes::PrimitiveFloatNode*>(node)) {
            node_type = "PipelineGraph.Nodes.PrimitiveFloatNode";
        } else if (dynamic_cast<PipelineGraph::Nodes::PrimitiveStringNode*>(node)) {
            node_type = "PipelineGraph.Nodes.PrimitiveStringNode";
        } else if (dynamic_cast<PipelineGraph::Nodes::PythonModuleNode*>(node)) {
            node_type = "PipelineGraph.Nodes.PythonModuleNode";
        } else if (dynamic_cast<PipelineGraph::Nodes::PythonFunctionNode*>(node)) {
            node_type = "PipelineGraph.Nodes.PythonFunctionNode";
        } else if (dynamic_cast<PipelineGraph::Nodes::AgentAcceptorNode*>(node)) {
            node_type = "PipelineGraph.Nodes.AgentAcceptorNode";
        } else if (dynamic_cast<PipelineGraph::Nodes::MethodAcceptorNode*>(node)) {
            node_type = "PipelineGraph.Nodes.MethodAcceptorNode";
        } else if (dynamic_cast<PipelineGraph::Nodes::EnvAcceptorNode*>(node)) {
            node_type = "PipelineGraph.Nodes.EnvAcceptorNode";
        } else if (dynamic_cast<PipelineGraph::Nodes::MaskAcceptorNode*>(node)) {
            node_type = "PipelineGraph.Nodes.MaskAcceptorNode";
        } else {
            Logger::error("Unknown node type during save: " + std::string(typeid(*node).name()));
            continue;
        }

        nodes_data.push_back({
            {"type", node_type},
            {"data", node_data}
        });
    }

    for (auto link: PipelineGraph::links) {
        links_data.push_back({
            {"src", link->outputPinId.Get()},
            {"dst", link->inputPinId.Get()}
        });
    }

    graph_json["nodes"] = nodes_data;
    graph_json["links"] = links_data;
    graph_json["next_id"] = PipelineGraph::nextId;

    std::ofstream graph_file_details_stream(graph_details);
    graph_file_details_stream << graph_json.dump(4);
    graph_file_details_stream.close();

    nlohmann::json modules_json = PyScope::getInstance().modulesPaths;
    std::ofstream modules_file_stream(modules_file);
    modules_file_stream << modules_json.dump(4);

    return true;
}
