//
// Created by xabdomo on 6/12/25.
//

#include "pipeline_graph.hpp"


#include <imgui.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <imgui_internal.h>

#include "ImGuiFileDialog.h"
#include "logger.hpp"
#include "pipeline.hpp"
#include "../font_manager.hpp"
#include "../shared_ui.hpp"
#include "../../backend/py_scope.hpp"
#include "../utility/drawing.h"


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

    template<>
    struct hash<ed::PinId> {
        std::size_t operator()(const ed::PinId& id) const noexcept {
            return std::hash<void*>{}(reinterpret_cast<void*>(id.AsPointer()));
        }
    };
}

namespace PipelineGraph {
    std::vector<Node*> nodes;
    std::vector<Link*> links;

    std::unordered_map<ed::NodeId, Node*> nodeLookup;
    std::unordered_map<ed::PinId , std::vector<Link*>> pinLinkLookup;
    std::unordered_map<ed::PinId , Node*> pinNodeLookup;
    std::unordered_map<ed::PinId , Pin*>  pinLookup;
    std::unordered_map<ed::LinkId, Link*> linkLookup;

    static int nextId = 1;

    int GetNextId() { return nextId++; }

    static ax::NodeEditor::EditorContext* m_Context;

    static std::vector<ed::NodeId> selectedNodeIds;
    static std::vector<ed::LinkId> selectedLinkIds;
    static std::vector<PipelineGraph::Node> clipboardNodes;

    void Node::init() {
        _executed = false;
    }

    bool Node::canExecute() {
       for (auto& input: inputs) {
           auto it = pinLinkLookup.find(input.id);
           if (it != pinLinkLookup.end()) {
               // zero - index because an input pin can only have one link to it
               if (!pinNodeLookup[pinLinkLookup[input.id][0]->outputPinId]->_executed) {
                   return false; // someone before this node didn't execute
               }
           }
        }
        return true;
    }

    static bool isLinked(ax::NodeEditor::PinId pin) {
        auto it = pinLinkLookup.find(pin);
        return it != pinLinkLookup.end();
    }

    static py::object getPinValue(const Pin& p) {
        auto link = pinLinkLookup.find(p.id);
        if (link != pinLinkLookup.end()) {
            // if it's an input pin : then there will only be one link
            // if it's an output pin: all links will have the same outputPinId
            return pinLookup[link->second[0]->outputPinId]->value;
        }
        return py::none();
    }

    ed::NodeId addNode(Node* n) {
        nodes.push_back(n);
        nodeLookup[n->id] = n;

        for (auto& pin : n->inputs) {
            pinNodeLookup[pin.id] = n;
            pinLookup[pin.id] = &pin;
        }
        for (auto& pin : n->outputs) {
            pinNodeLookup[pin.id] = n;
            pinLookup[pin.id] = &pin;
        }

        ed::SetCurrentEditor(m_Context);
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 canvasPos = ed::ScreenToCanvas(mousePos);

        // Set node position in the editor
        ed::SetNodePosition(n->id, canvasPos);
        ed::SetCurrentEditor(nullptr);

        return n->id;
    }


    void removeNode(ed::NodeId id) {
        auto node = nodeLookup.find(id);
        if (node == nodeLookup.end()) {
            Logger::error("Tried to deleted a node that does not exist");
            return;
        }

        clearNode(id);

        ed::DeleteNode(id);
        nodes.erase(std::ranges::find(nodes, node->second));
        delete node->second;
    }

    void clearNode(ed::NodeId id) {
        auto node = nodeLookup.find(id);
        if (node == nodeLookup.end()) {
            Logger::error("Tried to clear a node that does not exist");
            return;
        }
        auto node_ptr = node->second;
        for (auto& in: node_ptr->inputs) {
            if (pinLinkLookup.find(in.id) != pinLinkLookup.end()) {
                auto& links = pinLinkLookup[in.id];
                for (auto& link : links) {
                    removeLink(link->id);
                }
            }
        }

        for (auto& out: node_ptr->outputs) {
            if (pinLinkLookup.find(out.id) != pinLinkLookup.end()) {
                auto& links = pinLinkLookup[out.id];
                for (auto& link : links) {
                    removeLink(link->id);
                }
            }
        }
    }

    Pin* last_err_pin_src;
    Pin* last_err_pin_dst;

    ed::LinkId addLink(ed::PinId inputPinId, ed::PinId outputPinId) {
        if (inputPinId && outputPinId){// ed::AcceptNewItem() return true when user release mouse button.
            auto src = pinLookup[outputPinId];
            auto dst = pinLookup[inputPinId];
            if (src->direction != OUTPUT || dst->direction != INPUT) {
                ed::RejectNewItem();
            } else if (src->type.is_none() == false && dst->type.is_none() == false) {
                if (PyScope::isSubclassOrInstance(*src->type, *dst->type)) {
                    auto input_pin_link = pinLinkLookup.find(inputPinId);
                    auto output_pin_link = pinLinkLookup.find(outputPinId);
                    if (input_pin_link != pinLinkLookup.end()) {
                        // this input pin was previously connected
                        ed::RejectNewItem();
                    } else if (ed::AcceptNewItem()){
                        auto link = new Link{ ed::LinkId(GetNextId()), inputPinId, outputPinId };
                        links.push_back( link );

                        linkLookup[link->id]       = link;

                        if (output_pin_link == pinLinkLookup.end()) {
                            pinLinkLookup[outputPinId] = {};
                        }

                        pinLinkLookup[inputPinId]  = { link };
                        pinLinkLookup[outputPinId].push_back(link);
                        return link->id;
                    }
                } else {
                    ed::RejectNewItem();
                    try {
                        if (last_err_pin_src != src || last_err_pin_dst != dst) { // sometimes the link check is repeated which spams the Logger
                            Logger::warning(
                            std::format("Link failed: input requires type {} while output is {}.",
                            std::string(py::str(*dst->type)),
                            std::string(py::str(*src->type))
                            ));
                        }

                        last_err_pin_dst = dst;
                        last_err_pin_src = src;
                    } catch (...) {
                        Logger::error("Internal error while withing dst / src types");
                    }
                }
            } else {
                Logger::warning("Linking a pin where the input / output type is not defined.");
            }
        }

        return ed::LinkId(-1);
    }

    void removeLink(ed::LinkId id) {
        auto link = linkLookup.find(id);
        if (link == linkLookup.end()) {
            Logger::error("Tried to remove a link that does not exist");
            return;
        }

        auto link_ptr = link->second;

        ed::DeleteLink(id);
        linkLookup.erase(id);

        auto src = link_ptr->inputPinId;
        pinLinkLookup[src].erase(std::ranges::find(pinLinkLookup[src], link_ptr));
        if (pinLinkLookup[src].empty()) {
            pinLinkLookup.erase(src);
        }

        auto dst = link_ptr->outputPinId;
        pinLinkLookup[dst].erase(std::ranges::find(pinLinkLookup[dst], link_ptr));
        if (pinLinkLookup[dst].empty()) {
            pinLinkLookup.erase(dst);
        }

        links.erase(std::ranges::find(links, link_ptr));
        delete link_ptr;
    }


    static bool _isPearlNode(Node* node) {
        // is this node a pearl type node
        auto m_node = dynamic_cast<Nodes::SingleOutputNode*>(node);
        if (!m_node) return false;

        auto& output = m_node->outputs[0];
        auto& instance = PyScope::getInstance();

        return
            PyScope::isSubclassOrInstance(output.type, instance.pearl_agent_type)
        ||  PyScope::isSubclassOrInstance(output.type, instance.pearl_env)
        ||  PyScope::isSubclassOrInstance(output.type, instance.pearl_method_type);
    }

    static std::optional<ObjectRecipe> _buildFromSource(Node* start) {
        if (!start) {
            Logger::error("Tried to build from a null node");
            return std::nullopt;
        }


        std::set<Node*> graph;
        std::stack<Node*> stack;
        stack.push(start);
        while (!stack.empty()) {
            auto top = stack.top(); stack.pop();
            if (!graph.contains(top)) {
                graph.insert(top);
                for (auto& pin: top->inputs) {
                    auto it = pinLinkLookup.find(pin.id);
                    if (it != pinLinkLookup.end()) {
                        for (auto& link: it->second) {
                            auto src_node = pinNodeLookup[link->outputPinId];
                            if (!graph.contains(src_node)) {
                                stack.push(src_node);
                            }
                        }
                    }
                }
            }
        }

        std::set<Node*> curr_itr;
        std::set<Node*> next_itr;

        for (const auto node: graph) {
            node->_executed = false;
        }

        for (const auto node: graph) {
            if (node->canExecute()) curr_itr.insert(node);
        }

        ObjectRecipe result;

        while (!curr_itr.empty()) {
            next_itr.clear();

            for (const auto node: curr_itr) {
                node->_executed = true;
                result.plan.push_back(node);
            }

            for (const auto node: curr_itr) {
                // now add any next node that is ready to execute
                for (auto& pin: node->outputs) {
                    auto it = pinLinkLookup.find(pin.id);
                    if (it != pinLinkLookup.end()) {
                        for (auto& link: it->second) {
                            auto dst_node = pinNodeLookup[link->inputPinId];
                            if (!dst_node->_executed && !next_itr.contains(dst_node) && dst_node->canExecute()) {
                                next_itr.insert(dst_node);
                            }
                        }
                    }
                }
            }

            curr_itr = next_itr;
        }

        for (const auto node: graph) {
            if (!node->_executed) {
                Logger::error("Tried to build from node: " + std::string(start->_tag) + ", unable to construct a DAG.");
                return std::nullopt;
            }
        }

        result.acceptor = dynamic_cast<Nodes::AcceptorNode*>(start);

        return result;
    }

    std::vector<ObjectRecipe> build() {
        std::vector<ObjectRecipe> result;
        for (const auto node: nodes) {
            if (dynamic_cast<Nodes::AcceptorNode*>(node)) {
                auto k = _buildFromSource(node);
                if (k == std::nullopt) {
                    Logger::error("Failed to create build recipe from node: " + std::string(node->_tag));
                    continue;
                }
                result.push_back(*k);
            }
        }

        return result;
    }

    void init() {
        ed::Config config;
        config.SettingsFile = "Pipeline_Graph.json";
        m_Context = ed::CreateEditor(&config);
    }

    Pin* curr_focus_pin;
    static void switchFocus(Pin* p) {
        if (ImGui::IsItemHovered()) {
            curr_focus_pin = p;
        }
    }


    static void renderPinTooltip() {
        if (!curr_focus_pin) {
            return;
        }

        auto p = curr_focus_pin;
        ImGui::SetCursorPos({0, 0});
        ImGui::Dummy({0, 0}); // I swear I have no idea how that works ..

        ImGui::BeginTooltip();
        ImGui::Text("%s (%ul)", p->name.c_str(), p->id.Get());
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
        FontManager::pushFont("Light");
        std::string py_type_name = py::str(p->type);
        ImGui::Text(py_type_name.c_str());
        FontManager::popFont();
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 150, 150, 255));
        ImGui::Text(p->direction == INPUT ? "Input" : "Output");
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 200, 255));
        if (!p->tooltip.empty()) {
            ImGui::Text("%s", p->tooltip.c_str());
        }
        ImGui::PopStyleColor();


        ImGui::EndTooltip();
    }

    static void renderMenu() {
        {
            ImGui::TextDisabled("Primitives:");
            if (ImGui::MenuItem("Integer")) {
                addNode(new Nodes::PrimitiveIntNode(0));
            }

            if (ImGui::MenuItem("Integer (ranged)")) {
                addNode(new Nodes::PrimitiveIntNode(0, 0, 100));
            }

            if (ImGui::MenuItem("Float")) {
                addNode(new Nodes::PrimitiveFloatNode(0));
            }

            if (ImGui::MenuItem("Float (ranged)")) {
                addNode(new Nodes::PrimitiveFloatNode(0, 0.0f, 100.0f));
            }

            if (ImGui::MenuItem("String")) {
                addNode(new Nodes::PrimitiveStringNode(false));
            }

            if (ImGui::MenuItem("File")) {
                addNode(new Nodes::PrimitiveStringNode(true));
            }

            // ImGui::Separator();
            // ImGui::TextDisabled("Arithmetic:");
            // if (ImGui::MenuItem("Adder")) {
            //     addNode(new Nodes::AdderNode());
            // }

            ImGui::Separator();
            ImGui::TextDisabled("Pipeline:");

            if (ImGui::MenuItem("Agent")) {
                addNode(new Nodes::AgentAcceptorNode());
            }

            if (ImGui::MenuItem("Environment")) {
                addNode(new Nodes::EnvAcceptorNode());
            }

            // if (ImGui::MenuItem("Mask")) {
            //     addNode(new Nodes::MaskAcceptorNode());
            // }

            if (ImGui::MenuItem("Method")) {
                addNode(new Nodes::MethodAcceptorNode());
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("Objects")) {
                for (auto& module : SharedUi::loadedModules) {
                    if (ImGui::MenuItem(module.moduleName.c_str())) {
                        if (module.type == SharedUi::Function) {
                            addNode(new Nodes::PythonFunctionNode(&module));
                        } else {
                            addNode(new Nodes::PythonModuleNode(&module));
                        }
                    }
                }

                ImGui::EndMenu();
            }


        }
    }


    void render() {
        static bool m_FirstFrame = true;
        curr_focus_pin = nullptr;

        ImGui::Begin("Pipeline Graph");

        if (ImGui::BeginPopupContextItem("Pipeline Graph Context Menu", ImGuiPopupFlags_MouseButtonRight)) {
            renderMenu();
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupContextWindow("Pipeline Graph Context Menu - rc", ImGuiPopupFlags_MouseButtonRight)) {
            renderMenu();
            ImGui::EndPopup();
        }


        auto& io = ImGui::GetIO();

        ImGui::Text("FPS: %.2f (%.2gms) \t\t %ld node(s)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f, nodes.size());
        ImGui::SameLine();
        ImGui::Dummy({50, 0});
        ImGui::SameLine();
        auto space = ImGui::GetContentRegionAvail().x;
        auto size = ImGui::CalcTextSize("Build").x + 10;
        auto cursor = ImGui::GetCursorPos();
        cursor.x += space - size;
        ImGui::SetCursorPos(cursor);
        if (ImGui::Button("Build")) {
            auto _res = build();
            Pipeline::recipes = _res;
        }

        ImGui::Separator();
        ed::SetCurrentEditor(m_Context);
        ed::Begin("Pipeline Graph - Editor", ImVec2(0, 0));

        for (auto& node : nodes) {
            ed::BeginNode(node->id);
            ImGui::PushID(node->id.AsPointer());
            node->render();
            ImGui::PopID();
            ed::EndNode();
        }

        for (const auto& link : links) {
            ed::Link(link->id, link->inputPinId, link->outputPinId);
        }

        if (ed::BeginCreate()) {
            ed::PinId inputPinId, outputPinId;
            if (ed::QueryNewLink(&outputPinId, &inputPinId)){
                addLink(inputPinId, outputPinId);
            }
        }
        ed::EndCreate();

        if (ed::BeginDelete()) {
            ed::NodeId deletedNodeId;
            while (ed::QueryDeletedNode(&deletedNodeId)) {
                removeNode(deletedNodeId);
            }

            ed::LinkId deletedLinkId;
            while (ed::QueryDeletedLink(&deletedLinkId)) {
                removeLink(deletedLinkId);
            }
        }
        ed::EndDelete();


        ed::End();
        ed::SetCurrentEditor(nullptr);


        ImVec2 region_min = ImGui::GetWindowContentRegionMin();
        ImVec2 region_max = ImGui::GetWindowContentRegionMax();
        ImVec2 window_pos = ImGui::GetWindowPos();

        ImVec2 abs_min = ImVec2(window_pos.x + region_min.x, window_pos.y + region_min.y); // top-left
        ImVec2 abs_max = ImVec2(window_pos.x + region_max.x, window_pos.y + region_max.y); // bot-right

        ImVec2 region_size = ImVec2(region_max.x - region_min.x, region_max.y - region_min.y);

        // Create a full-window invisible button (must render something for drag-drop to bind)
        // Doesn't steal other events tho so that's nice
        ImGui::SetCursorScreenPos(abs_min);
        ImGui::InvisibleButton("##FullWindowDropTarget", region_size);

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SharedUi::LoadedModule")) {
                Logger::info("Dropped a python object into view");
                int index = *static_cast<int*>(payload->Data);
                auto *module = &SharedUi::loadedModules[index];
                if (module) {
                    // Create a new node for the dropped module
                    if (module->type == SharedUi::Function) {
                        PipelineGraph::addNode(new Nodes::PythonFunctionNode(module));
                    } else {
                        PipelineGraph::addNode(new Nodes::PythonModuleNode(module));
                    }
                } else {
                    Logger::error("Failed to cast payload data to SharedUi::LoadedModule");
                }
            }
            ImGui::EndDragDropTarget();
        }

        renderPinTooltip();

        ImGui::End();
        m_FirstFrame = false;
    }

    void destroy() {
        ed::DestroyEditor(m_Context);

        for (auto node: nodes) {
            delete node;
        }
        nodes.clear();

        for (auto link : links) {
            delete link;
        }
        links.clear();
    }

    static int renderNodeTag(Node* node) {
        if (!node->_editing_tag) {
            ImGui::PushID("##EditableText");
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(168, 109, 50, 255));
            if (ImGui::Selectable(node->_tag, false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(200, 0))) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    node->_editing_tag = true;
                    ImGui::SetKeyboardFocusHere();
                }
            }
            ImGui::PopStyleColor();
            ImGui::PopID();
        } else {
            ImGui::SetNextItemWidth(200);
            if (ImGui::InputText("##edit", node->_tag, IM_ARRAYSIZE(node->_tag), ImGuiInputTextFlags_EnterReturnsTrue)) {
                node->_editing_tag = false;
            }
            if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) {
                node->_editing_tag = false;
            }
        }

        return 200;
    }

    static void renderNodeAsTable(Node* node, int alloc_size = 0) {
        int max_size_in = 1;
        for (auto& input : node->inputs) {
            max_size_in = std::max(max_size_in, static_cast<int>(ImGui::CalcTextSize(input.name.c_str()).x));
        }

        int max_size_out = 1;
        for (auto& output : node->outputs) {
            max_size_out = std::max(max_size_out, static_cast<int>(ImGui::CalcTextSize(output.name.c_str()).x));
        }

        max_size_in = std::max(max_size_in, alloc_size - max_size_out - (60 + 5 * 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));

        if (ImGui::BeginTable("pin_layout", 2, ImGuiTableFlags_SizingFixedFit, {static_cast<float>(max_size_in + max_size_out + 60 + 5 * 4), 0})) {
            // Column 0: auto-fit to content
            ImGui::TableSetupColumn("Inputs", ImGuiTableColumnFlags_WidthFixed, max_size_in + 44);
            // Column 1: fill remaining space
            ImGui::TableSetupColumn("Outputs", ImGuiTableColumnFlags_WidthFixed, max_size_out + 24);
            ImGui::TableNextRow();

            // Left: Input pins
            ImGui::TableSetColumnIndex(0);
            for (auto& input : node->inputs) {
                ed::BeginPin(input.id, ed::PinKind::Input);
                ax::Drawing::DrawIcon(ImVec2(20, 20), ax::Drawing::IconType::Flow, pinLinkLookup.contains(input.id));
                ImGui::Dummy({20, 20});
                ed::EndPin();
                ImGui::SameLine();
                ImGui::Text("%s", input.name.c_str());
                switchFocus(&input);
            }

            // Right: Output pin
            ImGui::TableSetColumnIndex(1);

            // Align right
            float availWidth = max_size_out + 20; // 20 for the icon

            for (auto& output: node->outputs) {
                float textWidth = ImGui::CalcTextSize(output.name.c_str()).x;
                float iconSize = 20.0f;
                float totalWidth = textWidth + iconSize;

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availWidth - totalWidth);

                ImGui::Text(output.name.c_str());
                ImGui::SameLine();
                ed::BeginPin(output.id, ed::PinKind::Output);
                ax::Drawing::DrawIcon(ImVec2(20, 20), ax::Drawing::IconType::Circle, pinLinkLookup.contains(output.id));
                ImGui::Dummy({20, 20});
                ed::EndPin();
                switchFocus(&output);
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
    }


    Nodes::PrimitiveIntNode::PrimitiveIntNode(int val) {
        id = GetNextId();
        this->name = "Primitive Int";

        inputs.clear();
        Pin output;
        output.id = GetNextId();
        output.name = "value";
        output.tooltip = "Integer value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().int_type;
        value   = val;
        rangeEnd    = -1;
        rangeStart  = -1;

        outputs.push_back(output);
    }

    Nodes::PrimitiveIntNode::PrimitiveIntNode(int val, int v_rangeStart, int v_rangeEnd) {
        id = GetNextId();
        this->name = "Primitive Int";

        inputs.clear();
        Pin output;

        output.id    = GetNextId();
        output.name    = "value";
        output.tooltip = "Integer value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().int_type;

        value      = val;
        rangeStart = v_rangeStart;
        rangeEnd   = v_rangeEnd;

        outputs.push_back(output);
    }

    void Nodes::PrimitiveIntNode::exec() {
        outputs[0].value = PyScope::getInstance().int_type(value);
        _executed = true;
    }

    void Nodes::PrimitiveIntNode::render() {
        ImGui::Text(this->name.c_str());
        auto& elem = outputs[0];

        renderNodeTag(this);

        if (rangeEnd > rangeStart) {
            ImGui::SetNextItemWidth(100);
            ImGui::DragInt("##Value", &value, 1.0f, rangeStart, rangeEnd);

            ImGui::PushItemWidth(150);
            ImGui::InputInt("Min", &rangeStart);
            ImGui::InputInt("Max", &rangeEnd);
            ImGui::PopItemWidth();
        } else {
            ImGui::SetNextItemWidth(150);
            ImGui::InputInt(("##" + elem.name).c_str(), &value);
        }

        ImGui::SameLine();
        ImGui::Text("Out");
        ImGui::SameLine();
        ed::BeginPin(elem.id, ed::PinKind::Output);
        ax::Drawing::DrawIcon(ImVec2(20, 20), ax::Drawing::IconType::Circle, pinLinkLookup.contains(elem.id));
        ImGui::Dummy({20, 20});
        ed::EndPin();
        switchFocus(&elem);
    }

    Nodes::PrimitiveIntNode::~PrimitiveIntNode() = default;

    Nodes::PrimitiveFloatNode::PrimitiveFloatNode(float val) {
        id = GetNextId();
        this->name = "Primitive Float";

        inputs.clear();
        Pin output;
        output.id = GetNextId();
        output.name = "value";
        output.tooltip = "Float value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().float_type;
        value   = val;
        rangeEnd    = -1;
        rangeStart  = -1;

        outputs.push_back(output);
    }

    Nodes::PrimitiveFloatNode::PrimitiveFloatNode(float val, float v_rangeStart, float v_rangeEnd) {
        id = GetNextId();
        this->name = "Primitive Float";

        inputs.clear();
        Pin output;

        output.id    = GetNextId();
        output.name    = "value";
        output.tooltip = "Float value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().float_type;

        value      = val;
        rangeStart = v_rangeStart;
        rangeEnd   = v_rangeEnd;

        outputs.push_back(output);
    }

    void Nodes::PrimitiveFloatNode::exec() {
        outputs[0].value = PyScope::getInstance().float_type(value);
        _executed = true;
    }

    void Nodes::PrimitiveFloatNode::render() {
        ImGui::Text(this->name.c_str());
        auto& elem = outputs[0];

        renderNodeTag(this);

        if (rangeEnd > rangeStart) {
            ImGui::SetNextItemWidth(100);
            ImGui::DragFloat("##Value", &value, 1.0f, rangeStart, rangeEnd);

            ImGui::PushItemWidth(150);
            ImGui::InputFloat("Min", &rangeStart);
            ImGui::InputFloat("Max", &rangeEnd);
            ImGui::PopItemWidth();
        } else {
            ImGui::SetNextItemWidth(150);
            ImGui::InputFloat(("##" + elem.name).c_str(), &value);
        }


        ImGui::SameLine();
        ImGui::Text("Out");
        ImGui::SameLine();
        ed::BeginPin(elem.id, ed::PinKind::Output);
        ax::Drawing::DrawIcon(ImVec2(20, 20), ax::Drawing::IconType::Circle, pinLinkLookup.contains(elem.id));
        ImGui::Dummy({20, 20});
        ed::EndPin();
        switchFocus(&elem);
    }

    Nodes::PrimitiveFloatNode::~PrimitiveFloatNode() = default;

    Nodes::PrimitiveStringNode::PrimitiveStringNode(bool file) {
        _file = file;

        id = GetNextId();
        this->name = "Primitive String";

        inputs.clear();
        Pin output;

        output.id    = GetNextId();
        output.name    = "value";
        output.tooltip = "String value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().str_type;

        outputs.push_back(output);
    }

    void Nodes::PrimitiveStringNode::exec() {
        outputs[0].value = PyScope::getInstance().str_type(_value);
        _executed = true;
    }

    void Nodes::PrimitiveStringNode::render() {
        ImGui::Text(this->name.c_str());
        auto& elem = outputs[0];

        renderNodeTag(this);

        std::string key = "FileDlgKeyNodes" + std::to_string(this->id.Get());

        if (!_file) {
            ImGui::SetNextItemWidth(200);
            ImGui::InputText(("##" + elem.name).c_str(), _value, 1024);
        } else {
            ImGui::Text("%s", _value);
            if (ImGui::Button("Select")) {
                ImGuiFileDialog::Instance()->OpenDialog(key, "Select file", "");
            }
        }

        ImGui::SameLine();
        ImGui::Text("Out");
        ImGui::SameLine();
        ed::BeginPin(elem.id, ed::PinKind::Output);
        ax::Drawing::DrawIcon(ImVec2(20, 20), ax::Drawing::IconType::Circle, pinLinkLookup.contains(elem.id));
        ImGui::Dummy({20, 20});
        ed::EndPin();
        switchFocus(&elem);

        //fixme: dialog is not working correctly
        if (ImGuiFileDialog::Instance()->Display(key))
        {
            if (ImGuiFileDialog::Instance()->IsOk()) // If user selects a file
            {
                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                strcpy(_value, filePath.c_str());
            }
            ImGuiFileDialog::Instance()->Close();
        }
    }

    Nodes::PrimitiveStringNode::~PrimitiveStringNode() = default;

    Nodes::AdderNode::AdderNode() {
        id = GetNextId();
        this->name = "Adder";

        inputs.clear();
        outputs.clear();

        Pin input1;
        input1.id = GetNextId();
        input1.name = "A";
        input1.direction = INPUT;
        input1.tooltip = "First input value";
        input1.type = PyScope::getInstance().number_type;

        Pin input2;
        input2.id = GetNextId();
        input2.name = "B";
        input2.direction = INPUT;
        input2.tooltip = "Second input value";
        input2.type = PyScope::getInstance().number_type;

        Pin output;
        output.id = GetNextId();
        output.name = "Sum";
        output.direction = OUTPUT;
        output.tooltip = "Sum of A and B";
        output.type = PyScope::getInstance().number_type;

        inputs.push_back(input1);
        inputs.push_back(input2);
        outputs.push_back(output);
    }

    void Nodes::AdderNode::exec() {
        auto& instance = PyScope::getInstance();

        auto a = getPinValue(inputs[0]);
        auto b = getPinValue(inputs[1]);

        if (a.is_none() || b.is_none()) {
            Logger::warning("Adder node received one or more input with value None");
            if (a.is_none()) a = instance.int_type(0);
            if (b.is_none()) b = instance.int_type(0);
        }

        outputs[0].value = a + b;
        _executed = true;
    }

    void Nodes::AdderNode::render() {
        ImGui::Text(this->name.c_str());
        renderNodeTag(this);

        ImVec2 tableSize(150, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2)); // Tighter layout (optional)

        if (ImGui::BeginTable("pin_layout", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody, tableSize)) {
            ImGui::TableSetupColumn("Inputs",  ImGuiTableColumnFlags_WidthStretch, 100.0f);
            ImGui::TableSetupColumn("Outputs", ImGuiTableColumnFlags_WidthFixed, 50.0f);

            ImGui::TableNextRow();

            // Left: Input pins
            ImGui::TableSetColumnIndex(0);
            for (auto& input : inputs) {
                // ed::BeginPin(input.id, ed::PinKind::Input);
                // ImGui::Text("-> %s", input.name.c_str());
                // ed::EndPin();
                // renderPinTooltip(&input);

                ed::BeginPin(input.id, ed::PinKind::Input);
                ax::Drawing::DrawIcon(ImVec2(20, 20), ax::Drawing::IconType::Flow, pinLinkLookup.contains(input.id));
                ImGui::Dummy({20, 20});
                ed::EndPin();
                ImGui::SameLine();
                ImGui::Text("%s", input.name.c_str());
                switchFocus(&input);
            }

            // Right: Output pin
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Out");
            ImGui::SameLine();
            ed::BeginPin(outputs[0].id, ed::PinKind::Output);
            ax::Drawing::DrawIcon(ImVec2(20, 20), ax::Drawing::IconType::Circle, pinLinkLookup.contains(outputs[0].id));
            ImGui::Dummy({20, 20});
            ed::EndPin();
            switchFocus(&outputs[0]);

            ImGui::EndTable();
        }

        ImGui::PopStyleVar();
    }

    Nodes::AdderNode::~AdderNode() = default;

    Nodes::PythonModuleNode::PythonModuleNode(SharedUi::LoadedModule* type) {
        _type = type;

        id = GetNextId();
        this->name = "Python Module";
        this->_name = "<" + _type->moduleName + ">";

        inputs.clear();
        outputs.clear();

        Pin output;
        output.id = GetNextId();
        output.name = "obj";
        output.direction = OUTPUT;
        output.tooltip = "The Object";
        output.type = _type->module;
        outputs.push_back(output);

        for (auto& param: _type->constructor) {
            Pin input;
            input.id = GetNextId();
            input.name = param.attrName;
            input.direction = INPUT;
            input.tooltip = param.disc;
            input.type = param.type;
            inputs.push_back(input);
        }
    }

    void Nodes::PythonModuleNode::exec() {
        // Set parameters
        std::vector<py::object> constructor_args;
        for (auto & input : inputs) {
            if (!isLinked(input.id)) {
                Logger::warning(std::format("Input pin '{}' is not connected, using default value (None).", input.name));
            }
            auto val = getPinValue(input);
            constructor_args.push_back(val);
        }

        try {
            py::tuple args = py::cast(constructor_args);
            outputs[0].value = _type->module(*args); // un-pack them into the python object
            _executed = true;
        } catch (...) {
            Logger::error("Python Module returned an error");
        }
    }

    void Nodes::PythonModuleNode::render() {
        int max_node_header_size = 0;

        max_node_header_size = std::max(max_node_header_size, static_cast<int>(ImGui::CalcTextSize(this->name.c_str()).x));
        ImGui::Text(this->name.c_str());

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
        FontManager::pushFont("Light");
        max_node_header_size = std::max(max_node_header_size, static_cast<int>(ImGui::CalcTextSize(this->_name.c_str()).x));
        ImGui::Text(this->_name.c_str());
        FontManager::popFont();
        ImGui::PopStyleColor();

        max_node_header_size = std::max(max_node_header_size, renderNodeTag(this));

        renderNodeAsTable(this, max_node_header_size);
    }

    Nodes::PythonModuleNode::~PythonModuleNode() = default;

    Nodes::PythonFunctionNode::PythonFunctionNode(SharedUi::LoadedModule* type) {
        _type    = type;
        _pointer = false;

        id = GetNextId();
        this->name = "Python Function";
        this->_name = "<" + _type->moduleName + ">";

        inputs.clear();
        outputs.clear();

        Pin output;
        output.id = GetNextId();
        output.name = "ret";
        output.direction = OUTPUT;
        output.tooltip = "The Object";
        output.type = _type->returnType;
        outputs.push_back(output);

        for (auto& param: _type->constructor) {
            Pin input;
            input.id = GetNextId();
            input.name = param.attrName;
            input.direction = INPUT;
            input.tooltip = param.disc;
            input.type = param.type;
            _inputs.push_back(input);
        }
    }

    Nodes::PythonFunctionNode::PythonFunctionNode(SharedUi::LoadedModule* type, bool pointer) {
        _type    = type;
        _pointer = pointer;

        id = GetNextId();
        this->name = "Python Function";
        this->_name = "<" + _type->moduleName + ">";

        inputs.clear();
        outputs.clear();

        Pin output;
        output.id = GetNextId();
        output.name = "ret";
        output.direction = OUTPUT;
        output.tooltip = "Function pointer if enabled, otherwise the return value";
        output.type = _type->returnType;
        outputs.push_back(output);

        for (auto& param: _type->constructor) {
            Pin input;
            input.id = GetNextId();
            input.name = param.attrName;
            input.direction = INPUT;
            input.tooltip = param.disc;
            input.type = param.type;
            _inputs.push_back(input);
        }
    }

    void Nodes::PythonFunctionNode::exec() {

        if (_pointer) {
            outputs[0].value = _type->module;
            _executed = true;
            return;
        }

        // Set parameters
        std::vector<py::object> constructor_args;
        for (auto & input : inputs) {
            if (!isLinked(input.id)) {
                Logger::warning(std::format("Input pin '{}' is not connected, using default value (None).", input.name));
            }
            auto val = getPinValue(input);
            constructor_args.push_back(val);
        }

        try {
            py::tuple args = py::cast(constructor_args);
            outputs[0].value = _type->module(*args); // un-pack them into the python object
            _executed = true;
        } catch (...) {
            Logger::error("Python function returned an error");
        }
    }

    void Nodes::PythonFunctionNode::render() {
        int max_node_header_size = 0;

        max_node_header_size = std::max(max_node_header_size, static_cast<int>(ImGui::CalcTextSize(this->name.c_str()).x));
        ImGui::Text(this->name.c_str());

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
        FontManager::pushFont("Light");
        max_node_header_size = std::max(max_node_header_size, static_cast<int>(ImGui::CalcTextSize(this->_name.c_str()).x));
        ImGui::Text(this->_name.c_str());
        FontManager::popFont();
        ImGui::PopStyleColor();

        max_node_header_size = std::max(max_node_header_size, renderNodeTag(this));

        bool _p = _pointer;
        ImGui::Checkbox("Pointer", &_pointer);
        if (_p != _pointer) {
            // changed, so we should clear all links
            clearNode(this->id);
        }

        if (_pointer) {
            outputs[0].type = _type->module;
            inputs = {};
        } else {
            outputs[0].type = _type->returnType;
            inputs = _inputs;
        }

        if (!_pointer) {
            renderNodeAsTable(this, max_node_header_size);
        } else {
            float textWidth = ImGui::CalcTextSize("func").x;
            int avail_width = std::max(max_node_header_size, (int) textWidth + 20 + 4);
            float iconSize = 20.0f;
            float spacing = 4.0f; // spacing between text and icon
            float totalWidth = textWidth + spacing + iconSize;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail_width - totalWidth);

            ImGui::Text("func");
            ImGui::SameLine();
            ed::BeginPin(outputs[0].id, ed::PinKind::Output);
            ax::Drawing::DrawIcon(ImVec2(20, 20), ax::Drawing::IconType::Circle, pinLinkLookup.contains(outputs[0].id));
            ImGui::Dummy({20, 20});
            ed::EndPin();
            switchFocus(&outputs[0]);
        }
    }

    Nodes::PythonFunctionNode::~PythonFunctionNode() = default;

    Nodes::AcceptorNode::AcceptorNode() = default;

    void Nodes::AcceptorNode::exec() {
        _result = getPinValue(inputs[0]);
    }

    void Nodes::AcceptorNode::render() {
        ImGui::Text(this->name.c_str());
        renderNodeTag(this);
        ed::BeginPin(inputs[0].id, ed::PinKind::Input);
        ax::Drawing::DrawIcon(ImVec2(20, 20), ax::Drawing::IconType::Circle, pinLinkLookup.contains(inputs[0].id));
        ImGui::Dummy({20, 20});
        ed::EndPin();
        switchFocus(&inputs[0]);
    }

    Nodes::AcceptorNode::~AcceptorNode() = default;

    Nodes::AgentAcceptorNode::AgentAcceptorNode() {

        id = GetNextId();
        this->name = "Agent Acceptor";

        Pin input;
        input.id = GetNextId();
        input.name = "agent";
        input.direction = INPUT;
        input.tooltip = "The Agent to add to pipeline";
        input.type = PyScope::getInstance().pearl_agent_type;
        inputs.push_back(input);
    }

    Nodes::EnvAcceptorNode::EnvAcceptorNode() {

        id = GetNextId();
        this->name = "Env Acceptor";

        Pin input;
        input.id = GetNextId();
        input.name = "Environment";
        input.direction = INPUT;
        input.tooltip = "The environment to add to pipeline";
        input.type = PyScope::getInstance().pearl_env_type;
        inputs.push_back(input);
    }

    Nodes::MaskAcceptorNode::MaskAcceptorNode() {
        id = GetNextId();
        this->name = "Mask Acceptor";

        Pin input;
        input.id = GetNextId();
        input.name = "mask";
        input.direction = INPUT;
        input.tooltip = "The mask to add to pipeline";
        input.type = PyScope::getInstance().pearl_mask_type;
        inputs.push_back(input);
    }

    Nodes::MethodAcceptorNode::MethodAcceptorNode() {
        id = GetNextId();
        this->name = "Method Acceptor";

        Pin input;
        input.id = GetNextId();
        input.name = "method";
        input.direction = INPUT;
        input.tooltip = "The method to add to pipeline";
        input.type = PyScope::getInstance().pearl_method_type;
        inputs.push_back(input);
    }
}
