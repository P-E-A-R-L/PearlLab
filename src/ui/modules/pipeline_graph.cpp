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
#include "../font_manager.hpp"
#include "../shared_ui.hpp"
#include "../../backend/py_scope.hpp"


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

    static py::object getPinValue(Pin p) {
        auto link = pinLinkLookup.find(p.id);
        if (link != pinLinkLookup.end()) {
            // if it's an input pin : then there will only be one link
            // if it's an output pin: all links will have the same outputPinId
            return pinLookup[link->second[0]->outputPinId]->value;
        }
        return py::none();
    }

    ed::NodeId addNode(Node * n) {
        nodes.push_back(n);
        nodeLookup[n->id] = n;
        for (auto& pin: n->inputs) {
            pinNodeLookup[pin.id] = n;
            pinLookup[pin.id] = &pin;
        }
        for (auto& pin: n->outputs) {
            pinNodeLookup[pin.id] = n;
            pinLookup[pin.id] = &pin;
        }

        return n->id;
    }

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
        static bool m_FirstFrame = true;

        ImGui::Begin("Pipeline Graph");

        if (ImGui::BeginPopupContextItem("Pipeline Graph Context Menu", ImGuiPopupFlags_MouseButtonRight)) {
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

            ImGui::Separator();
            ImGui::TextDisabled("Arithmetic:");
            if (ImGui::MenuItem("Adder")) {
                addNode(new Nodes::AdderNode());
            }

            ImGui::Separator();
            ImGui::TextDisabled("Objects:");
            for (auto& module : SharedUi::loadedModules) {
                if (ImGui::MenuItem(module.moduleName.c_str())) {
                    if (module.type == SharedUi::Function) {
                        addNode(new Nodes::PythonFunctionNode(&module));
                    } else {
                        addNode(new Nodes::PythonModuleNode(&module));
                    }
                }
            }

            ImGui::Separator();
            ImGui::TextDisabled("Pipeline:");

            ImGui::EndPopup();
        }

        auto& io = ImGui::GetIO();

        ImGui::Text("FPS: %.2f (%.2gms) \t\t %ld node(s)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f, nodes.size());
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
                if (inputPinId && outputPinId){
                    // ed::AcceptNewItem() return true when user release mouse button.
                    auto src = pinLookup[outputPinId];
                    auto dst = pinLookup[inputPinId];

                    if (src->direction != OUTPUT && dst->direction != INPUT) {
                        ed::RejectNewItem();
                    } else if (src->type.is_none() == false && dst->type.is_none() == false) {
                        if (PyScope::isSubclass(*src->type, *dst->type)) {
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
                            }
                        } else {
                            ed::RejectNewItem();
                            try {
                                Logger::warning(
                                    std::format("Link failed: input requires type {} while output is {}.",
                                        std::string(py::str(*dst->type)),
                                        std::string(py::str(*src->type))
                                        ));
                            } catch (...) {
                                Logger::error("Internal error while withing dst / src types");
                            }
                        }
                    } else {
                        Logger::warning("Linking a pin where the input / output type is not defined.");
                    }
                }
            }
        }
        ed::EndCreate();

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
                auto *module = static_cast<SharedUi::LoadedModule *>(payload->Data);
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
        auto elem = outputs[0];

        if (!this->_editing) {
            ImGui::PushID("EditableText");
            if (ImGui::Selectable(this->_buffer, false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(200, 0))) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    this->_editing = true;
                    ImGui::SetKeyboardFocusHere();
                }
            }
            ImGui::PopID();
        } else {
            ImGui::SetNextItemWidth(200);
            if (ImGui::InputText("##edit", this->_buffer, IM_ARRAYSIZE( this->_buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                this->_editing = false;
            }
            if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) {
                this->_editing = false;
            }
        }

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
        ed::BeginPin(elem.id, ed::PinKind::Output);
            ImGui::Text("Out ->");
        ed::EndPin();
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
        auto elem = outputs[0];

        if (!this->_editing) {
            ImGui::PushID("EditableText");
            if (ImGui::Selectable(this->_buffer, false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(200, 0))) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    this->_editing = true;
                    ImGui::SetKeyboardFocusHere();
                }
            }
            ImGui::PopID();
        } else {
            ImGui::SetNextItemWidth(200);
            if (ImGui::InputText("##edit", this->_buffer, IM_ARRAYSIZE( this->_buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                this->_editing = false;
            }
            if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) {
                this->_editing = false;
            }
        }

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
        ed::BeginPin(elem.id, ed::PinKind::Output);
            ImGui::Text("Out ->");
        ed::EndPin();
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
        auto elem = outputs[0];

        if (!this->_editing) {
            ImGui::PushID("EditableText");
            if (ImGui::Selectable(this->_buffer, false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(200, 0))) {
                if (ImGui::IsMouseDoubleClicked(0)) {
                    this->_editing = true;
                    ImGui::SetKeyboardFocusHere();
                }
            }
            ImGui::PopID();
        } else {
            ImGui::SetNextItemWidth(200);
            if (ImGui::InputText("##edit", this->_buffer, IM_ARRAYSIZE( this->_buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                this->_editing = false;
            }
            if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0)) {
                this->_editing = false;
            }
        }


        std::string key = "FileDlgKeyNodes" + std::to_string(this->id.Get());

        if (!_file) {
            ImGui::InputText(("##" + elem.name).c_str(), _value, 1024);
        } else {
            ImGui::Text("%s", _value);
            if (ImGui::Button("Select")) {
                ImGuiFileDialog::Instance()->OpenDialog(key, "Select file", "");
            }
        }

        ImGui::SameLine();
        ed::BeginPin(elem.id, ed::PinKind::Output);
        ImGui::Text("Out ->");
        ed::EndPin();

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

        ImVec2 tableSize(150, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2)); // Tighter layout (optional)

        if (ImGui::BeginTable("pin_layout", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody, tableSize)) {
            ImGui::TableSetupColumn("Inputs", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Outputs", ImGuiTableColumnFlags_WidthFixed, 50.0f);

            ImGui::TableNextRow();

            // Left: Input pins
            ImGui::TableSetColumnIndex(0);
            for (auto& input : inputs) {
                ed::BeginPin(input.id, ed::PinKind::Input);
                ImGui::Text("-> %s", input.name.c_str());
                ed::EndPin();
            }

            // Right: Output pin
            ImGui::TableSetColumnIndex(1);
            ed::BeginPin(outputs[0].id, ed::PinKind::Output);
            ImGui::Text("Out ->");
            ed::EndPin();

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
        output.name = "self";
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
        ImGui::Text(this->name.c_str());

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
        FontManager::pushFont("Light");
        ImGui::Text(this->_name.c_str());
        FontManager::popFont();
        ImGui::PopStyleColor();

        ImVec2 tableSize(150, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));

        if (ImGui::BeginTable("pin_layout", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody, tableSize)) {
            ImGui::TableSetupColumn("Inputs", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Outputs", ImGuiTableColumnFlags_WidthFixed, 50.0f);

            ImGui::TableNextRow();

            // Left: Input pins
            ImGui::TableSetColumnIndex(0);
            for (auto& input : inputs) {
                ed::BeginPin(input.id, ed::PinKind::Input);
                ImGui::Text("-> %s", input.name.c_str());
                ed::EndPin();
            }

            // Right: Output pin
            ImGui::TableSetColumnIndex(1);
            ed::BeginPin(outputs[0].id, ed::PinKind::Output);
            ImGui::Text("Obj ->");
            ed::EndPin();

            ImGui::EndTable();
        }

        ImGui::PopStyleVar();
    }

    Nodes::PythonModuleNode::~PythonModuleNode() = default;

    Nodes::PythonFunctionNode::PythonFunctionNode(SharedUi::LoadedModule* type) {
        _type = type;

        id = GetNextId();
        this->name = "Python Function";
        this->_name = "<" + _type->moduleName + ">";

        inputs.clear();
        outputs.clear();

        Pin output;
        output.id = GetNextId();
        output.name = "self";
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
            inputs.push_back(input);
        }
    }

    void Nodes::PythonFunctionNode::exec() {
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
        ImGui::Text(this->name.c_str());

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
        FontManager::pushFont("Light");
        ImGui::Text(this->_name.c_str());
        FontManager::popFont();
        ImGui::PopStyleColor();

        ImVec2 tableSize(150, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));

        if (ImGui::BeginTable("pin_layout", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody, tableSize)) {
            ImGui::TableSetupColumn("Inputs", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Outputs", ImGuiTableColumnFlags_WidthFixed, 50.0f);

            ImGui::TableNextRow();

            // Left: Input pins
            ImGui::TableSetColumnIndex(0);
            for (auto& input : inputs) {
                ed::BeginPin(input.id, ed::PinKind::Input);
                ImGui::Text("-> %s", input.name.c_str());
                ed::EndPin();
            }

            // Right: Output pin
            ImGui::TableSetColumnIndex(1);
            ed::BeginPin(outputs[0].id, ed::PinKind::Output);
            ImGui::Text("ret ->");
            ed::EndPin();

            ImGui::EndTable();
        }

        ImGui::PopStyleVar();
    }

    Nodes::PythonFunctionNode::~PythonFunctionNode() = default;
}
