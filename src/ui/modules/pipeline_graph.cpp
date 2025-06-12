//
// Created by xabdomo on 6/12/25.
//

#include "pipeline_graph.hpp"


#include <imgui.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>


namespace std {
    template<>
    struct hash<ed::NodeId> {
        std::size_t operator()(const ed::NodeId& id) const noexcept {
            return std::hash<void*>{}(reinterpret_cast<void*>(id.AsPointer()));
        }
    };

    template<>
    struct hash<ed::LinkId> {
        std::size_t operator()(const ed::LinkId& id) const noexcept {
            return std::hash<void*>{}(reinterpret_cast<void*>(id.AsPointer()));
        }
    };
}

namespace PipelineGraph {
    std::vector<Node> nodes;
    std::vector<Link> links;
    static std::unordered_map<ed::NodeId, Node*> nodeLookup;
    static std::unordered_map<ed::LinkId, Link*> linkLookup;

    static int nextId = 1;
    int GetNextId() { return nextId++; }

    static ax::NodeEditor::EditorContext* m_Context;

    static std::vector<ed::NodeId> selectedNodeIds;
    static std::vector<ed::LinkId> selectedLinkIds;
    static std::vector<PipelineGraph::Node> clipboardNodes;

    void init() {
        ed::Config config;
        config.SettingsFile = "Pipeline_Graph.json";
        m_Context = ed::CreateEditor(&config);
    }

    void render() {
        ImGui::Begin("Pipeline Graph");

        auto& io = ImGui::GetIO();

        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
        ImGui::Separator();
        ed::SetCurrentEditor(m_Context);
        ed::Begin("PipelineGraph");

        for (auto& node : nodes) {
            ed::BeginNode(node.id);
            ImGui::Text("%s", node.name.c_str());

            ImGui::PushID(&node.id);

            for (auto& input : node.inputs) {
                ed::BeginPin(input.id, ed::PinKind::Input);
                ImGui::Text("-> %s", input.name.c_str());
                ed::EndPin();
            }

            for (auto& output : node.outputs) {
                ed::BeginPin(output.id, ed::PinKind::Output);
                ImGui::Text("%s ->", output.name.c_str());
                ed::EndPin();
            }

            ImGui::PopID();
            ed::EndNode();
        }

        for (const auto& link : links) {
            ed::Link(link.id, link.inputPinId, link.outputPinId);
        }

        // // Handle new links
        if (ed::BeginCreate()) {
            ed::PinId inputPinId, outputPinId;
            if (ed::QueryNewLink(&inputPinId, &outputPinId)) {
                if (inputPinId && outputPinId) {
                    if (ed::AcceptNewItem()) {
                        Link link{ GetNextId(), inputPinId, outputPinId };
                        links.push_back(link);
                        linkLookup[link.id] = &links.back();
                    }
                }
            }
        }
        ed::EndCreate();

        // Handle deletions
        if (ed::BeginDelete()) {
            ed::LinkId deletedLinkId;
            while (ed::QueryDeletedLink(&deletedLinkId)) {
                if (ed::AcceptDeletedItem()) {
                    links.erase(std::remove_if(links.begin(), links.end(),
                        [=](const Link& l) { return l.id == deletedLinkId; }),
                        links.end());
                    linkLookup.erase(deletedLinkId);
                }
            }

            ed::NodeId deletedNodeId;
            while (ed::QueryDeletedNode(&deletedNodeId)) {
                if (ed::AcceptDeletedItem()) {
                    nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                        [=](const Node& n) { return n.id == deletedNodeId; }),
                        nodes.end());
                    nodeLookup.erase(deletedNodeId);
                }
            }
        }

        ed::EndDelete();

        // if (ed::BeginShortcut()) {
        //     if (ed::AcceptCopy()) {
        //         selectedNodeIds.resize(ed::GetSelectedObjectCount());
        //         ed::GetSelectedNodes(selectedNodeIds.data(), selectedNodeIds.size());
        //
        //         clipboardNodes.clear();
        //         for (auto id : selectedNodeIds) {
        //             auto it = nodeLookup.find(id.Get());
        //             if (it != nodeLookup.end()) {
        //                 clipboardNodes.push_back(*it->second);
        //             }
        //         }
        //     }
        //
        //     if (ed::AcceptCut()) {
        //         // Copy first
        //         selectedNodeIds.resize(ed::GetSelectedObjectCount());
        //         ed::GetSelectedNodes(selectedNodeIds.data(), selectedNodeIds.size());
        //
        //         clipboardNodes.clear();
        //         for (auto id : selectedNodeIds) {
        //             auto it = nodeLookup.find(id.Get());
        //             if (it != nodeLookup.end()) {
        //                 clipboardNodes.push_back(*it->second);
        //             }
        //         }
        //
        //         // Then delete
        //         for (auto id : selectedNodeIds) {
        //             nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [&](const Node& n) {
        //                 return n.id == id;
        //             }), nodes.end());
        //             nodeLookup.erase(id.Get());
        //         }
        //
        //         selectedNodeIds.clear();
        //     }
        //
        //     if (ed::AcceptPaste()) {
        //         for (auto& node : clipboardNodes) {
        //             Node newNode = node;
        //             newNode.id = GetNextId();
        //             for (auto& input : newNode.inputs)
        //                 input.id = GetNextId();
        //             for (auto& output : newNode.outputs)
        //                 output.id = GetNextId();
        //
        //             newNode.name += " (copy)";
        //             nodes.push_back(newNode);
        //             nodeLookup[newNode.id.Get()] = &nodes.back();
        //         }
        //     }
        // }
        //
        // ed::EndShortcut();


        ed::End();
        ed::SetCurrentEditor(nullptr);

        ImGui::End();
    }

    void destroy() {
        ed::DestroyEditor(m_Context);
    }
}