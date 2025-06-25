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
#include "../startup_loader.hpp"
#include "../../backend/py_safe_wrapper.hpp"

#include "../utility/drawing.h"

namespace std
{
    template <>
    struct hash<ed::NodeId>
    {
        std::size_t operator()(const ed::NodeId &id) const noexcept
        {
            return std::hash<void *>{}(reinterpret_cast<void *>(id.AsPointer()));
        }
    };

    template <>
    struct hash<ed::LinkId>
    {
        std::size_t operator()(const ed::LinkId &id) const noexcept
        {
            return std::hash<void *>{}(reinterpret_cast<void *>(id.AsPointer()));
        }
    };

    template <>
    struct hash<ed::PinId>
    {
        std::size_t operator()(const ed::PinId &id) const noexcept
        {
            return std::hash<void *>{}(reinterpret_cast<void *>(id.AsPointer()));
        }
    };
}

namespace PipelineGraph
{
    std::vector<Node *> nodes;
    std::vector<Link *> links;

    std::unordered_map<ed::NodeId, Node *> nodeLookup;
    std::unordered_map<ed::PinId, std::vector<Link *>> pinLinkLookup;
    std::unordered_map<ed::PinId, Node *> pinNodeLookup;
    std::unordered_map<ed::PinId, Pin *> pinLookup;
    std::unordered_map<ed::LinkId, Link *> linkLookup;

    size_t nextId = 1;

    int GetNextId() { return nextId++; }

    static ax::NodeEditor::EditorContext *m_Context;

    static std::vector<ed::NodeId> selectedNodeIds;
    static std::vector<ed::LinkId> selectedLinkIds;
    static std::vector<PipelineGraph::Node> clipboardNodes;

    void ShowIntegerInputDialog(bool *open, const char *popup_id, const std::function<void(int)> &on_confirm)
    {
        static int input_value = 0;

        if (*open)
        {
            ImGui::OpenPopup(popup_id);
            *open = false; // Reset the flag
        }

        if (ImGui::BeginPopupModal(popup_id, NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter an integer value:");
            ImGui::InputInt("##InputInt", &input_value);

            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                on_confirm(input_value); // Call the lambda with the input value
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void Node::init()
    {
        _executed = false;
    }

    bool Node::canExecute()
    {
        for (auto &input : inputs)
        {
            auto it = pinLinkLookup.find(input.id);
            if (it != pinLinkLookup.end())
            {
                // zero - index because an input pin can only have one link to it
                if (!pinNodeLookup[pinLinkLookup[input.id][0]->outputPinId]->_executed)
                {
                    return false; // someone before this node didn't execute
                }
            }
        }
        return true;
    }

    void Node::save(nlohmann::json &custom_data)
    {
        custom_data["id"] = id.Get();
        std::vector<size_t> inputs_details;
        std::vector<size_t> outputs_details;

        for (auto &in : inputs)
        {
            inputs_details.push_back(in.id.Get());
        }
        custom_data["inputs"] = inputs_details;

        for (auto &in : outputs)
        {
            outputs_details.push_back(in.id.Get());
        }
        custom_data["outputs"] = outputs_details;

        custom_data["tag"] = std::string(this->_tag);
    }

    void Node::load(nlohmann::json &custom_data)
    {
        id = ed::NodeId(custom_data["id"].get<size_t>());

        const auto inputs_details = custom_data["inputs"].get<std::vector<size_t>>();
        const auto outputs_details = custom_data["outputs"].get<std::vector<size_t>>();

        for (int i = 0; i < inputs.size(); ++i)
        {
            inputs[i].id = ed::PinId(inputs_details[i]);
        }

        for (int i = 0; i < outputs.size(); ++i)
        {
            outputs[i].id = ed::PinId(outputs_details[i]);
        }

        if (custom_data.contains("tag"))
        {
            std::string tag = custom_data["tag"].get<std::string>();
            strcpy(this->_tag, tag.c_str());
        }
    }

    static bool isLinked(ax::NodeEditor::PinId pin)
    {
        auto it = pinLinkLookup.find(pin);
        return it != pinLinkLookup.end();
    }

    static py::object getPinValue(const Pin &p)
    {
        auto link = pinLinkLookup.find(p.id);
        if (link != pinLinkLookup.end())
        {
            // if it's an input pin : then there will only be one link
            // if it's an output pin: all links will have the same outputPinId
            return pinLookup[link->second[0]->outputPinId]->value;
        }
        return py::none();
    }

    ed::NodeId _addNode(Node *n)
    {
        nodes.push_back(n);
        nodeLookup[n->id] = n;

        for (auto &pin : n->inputs)
        {
            pinNodeLookup[pin.id] = n;
            pinLookup[pin.id] = &pin;
        }
        for (auto &pin : n->outputs)
        {
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

    ed::NodeId addNode(Node *n)
    {
        py::gil_scoped_acquire acquire{};

        nodes.push_back(n);
        nodeLookup[n->id] = n;

        for (auto &pin : n->inputs)
        {
            pinNodeLookup[pin.id] = n;
            pinLookup[pin.id] = &pin;
        }
        for (auto &pin : n->outputs)
        {
            pinNodeLookup[pin.id] = n;
            pinLookup[pin.id] = &pin;
        }
        return n->id;
    }

    void removeNode(ed::NodeId id)
    {
        py::gil_scoped_acquire acquire{};

        auto node = nodeLookup.find(id);
        if (node == nodeLookup.end())
        {
            Logger::error("Tried to deleted a node that does not exist");
            return;
        }

        clearNode(id);

        ed::DeleteNode(id);
        nodes.erase(std::ranges::find(nodes, node->second));
        delete node->second;
    }

    void clearNode(ed::NodeId id)
    {
        py::gil_scoped_acquire acquire{};

        auto node = nodeLookup.find(id);
        if (node == nodeLookup.end())
        {
            Logger::error("Tried to clear a node that does not exist");
            return;
        }
        auto node_ptr = node->second;
        for (auto &in : node_ptr->inputs)
        {
            if (pinLinkLookup.find(in.id) != pinLinkLookup.end())
            {
                auto &links = pinLinkLookup[in.id];
                for (auto &link : links)
                {
                    removeLink(link->id);
                }
            }
        }

        for (auto &out : node_ptr->outputs)
        {
            if (pinLinkLookup.find(out.id) != pinLinkLookup.end())
            {
                auto &links = pinLinkLookup[out.id];
                for (auto &link : links)
                {
                    removeLink(link->id);
                }
            }
        }
    }

    Pin *last_err_pin_src;
    Pin *last_err_pin_dst;

    ed::LinkId addLink(ed::PinId inputPinId, ed::PinId outputPinId)
    {
        if (inputPinId && outputPinId)
        { // ed::AcceptNewItem() return true when user release mouse button.
            auto src = pinLookup[outputPinId];
            auto dst = pinLookup[inputPinId];
            if (src->direction != OUTPUT || dst->direction != INPUT)
            {
                // ed::RejectNewItem();
            }
            else if (src->type.is_none() == false && dst->type.is_none() == false)
            {
                // no need to check for types if it's from a function call
                if (true)
                {
                    auto input_pin_link = pinLinkLookup.find(inputPinId);
                    auto output_pin_link = pinLinkLookup.find(outputPinId);
                    if (input_pin_link != pinLinkLookup.end())
                    {
                    }
                    else
                    {
                        auto link = new Link{ed::LinkId(GetNextId()), inputPinId, outputPinId};
                        links.push_back(link);

                        linkLookup[link->id] = link;

                        if (output_pin_link == pinLinkLookup.end())
                        {
                            pinLinkLookup[outputPinId] = {};
                        }

                        pinLinkLookup[inputPinId] = {link};
                        pinLinkLookup[outputPinId].push_back(link);
                        return link->id;
                    }
                }
            }
            else
            {
                Logger::warning("Linking a pin where the input / output type is not defined.");
            }
        }

        return ed::LinkId(-1);
    }

    // same as AddLink, but this one is called from UI
    ed::LinkId _addLink(ed::PinId inputPinId, ed::PinId outputPinId)
    {
        py::gil_scoped_acquire acquire{};

        if (inputPinId && outputPinId)
        { // ed::AcceptNewItem() return true when user release mouse button.
            auto src = pinLookup[outputPinId];
            auto dst = pinLookup[inputPinId];
            if (src->direction != OUTPUT || dst->direction != INPUT)
            {
                ed::RejectNewItem();
            }
            else if (src->type.is_none() == false && dst->type.is_none() == false)
            {
                // fixme: I added an object-type pypass rule to avoid drawing the entire graph each time I debug
                //        run the application, I may need to remove it one day.
                if (PyScope::isSubclassOrInstance(*src->type, *dst->type) || src->type.is(PyScope::getInstance().object_type))
                {

                    auto input_pin_link = pinLinkLookup.find(inputPinId);
                    auto output_pin_link = pinLinkLookup.find(outputPinId);
                    if (input_pin_link != pinLinkLookup.end())
                    {
                        // this input pin was previously connected
                        ed::RejectNewItem();
                    }
                    else if (ed::AcceptNewItem())
                    {
                        auto link = new Link{ed::LinkId(GetNextId()), inputPinId, outputPinId};
                        links.push_back(link);

                        linkLookup[link->id] = link;

                        if (output_pin_link == pinLinkLookup.end())
                        {
                            pinLinkLookup[outputPinId] = {};
                        }

                        pinLinkLookup[inputPinId] = {link};
                        pinLinkLookup[outputPinId].push_back(link);
                        return link->id;
                    }
                }
                else
                {
                    ed::RejectNewItem();
                    try
                    {
                        if (last_err_pin_src != src || last_err_pin_dst != dst)
                        { // sometimes the link check is repeated which spams the Logger
                            // Logger::warning(
                            // std::format("Link failed: input requires type {} while output is {}.",
                            // std::string(py::str(*dst->type)),
                            // std::string(py::str(*src->type))
                            // ));
                            std::ostringstream msg;
                            msg << "Link failed: input requires type " << std::string(py::str(*dst->type))
                                << " while output is " << std::string(py::str(*src->type)) << ".";
                            Logger::warning(msg.str());
                        }

                        last_err_pin_dst = dst;
                        last_err_pin_src = src;
                    }
                    catch (...)
                    {
                        Logger::error("Internal error while withing dst / src types");
                    }
                }
            }
            else
            {
                Logger::warning("Linking a pin where the input / output type is not defined.");
            }
        }

        return ed::LinkId(-1);
    }

    void removeLink(ed::LinkId id)
    {
        py::gil_scoped_acquire acquire{};

        auto link = linkLookup.find(id);
        if (link == linkLookup.end())
        {
            Logger::error("Tried to remove a link that does not exist");
            return;
        }

        auto link_ptr = link->second;

        ed::DeleteLink(id);
        linkLookup.erase(id);

        auto src = link_ptr->inputPinId;
        pinLinkLookup[src].erase(std::ranges::find(pinLinkLookup[src], link_ptr));
        if (pinLinkLookup[src].empty())
        {
            pinLinkLookup.erase(src);
        }

        auto dst = link_ptr->outputPinId;
        pinLinkLookup[dst].erase(std::ranges::find(pinLinkLookup[dst], link_ptr));
        if (pinLinkLookup[dst].empty())
        {
            pinLinkLookup.erase(dst);
        }

        links.erase(std::ranges::find(links, link_ptr));
        delete link_ptr;
    }

    static bool _isPearlNode(Node *node)
    {
        // is this node a pearl type node
        auto m_node = dynamic_cast<Nodes::SingleOutputNode *>(node);
        if (!m_node)
            return false;

        auto &output = m_node->outputs[0];
        auto &instance = PyScope::getInstance();

        return PyScope::isSubclassOrInstance(output.type, instance.pearl_agent_type) || PyScope::isSubclassOrInstance(output.type, instance.pearl_env) || PyScope::isSubclassOrInstance(output.type, instance.pearl_method_type);
    }

    static std::optional<ObjectRecipe> _buildFromSource(Node *start)
    {
        if (!start)
        {
            Logger::error("Tried to build from a null node");
            return std::nullopt;
        }

        std::set<Node *> graph;
        std::stack<Node *> stack;
        stack.push(start);
        while (!stack.empty())
        {
            auto top = stack.top();
            stack.pop();
            if (!graph.contains(top))
            {
                graph.insert(top);
                for (auto &pin : top->inputs)
                {
                    auto it = pinLinkLookup.find(pin.id);
                    if (it != pinLinkLookup.end())
                    {
                        for (auto &link : it->second)
                        {
                            auto src_node = pinNodeLookup[link->outputPinId];
                            if (!graph.contains(src_node))
                            {
                                stack.push(src_node);
                            }
                        }
                    }
                }
            }
        }

        std::set<Node *> curr_itr;
        std::set<Node *> next_itr;

        for (const auto node : graph)
        {
            node->_executed = false;
        }

        for (const auto node : graph)
        {
            if (node->canExecute())
                curr_itr.insert(node);
        }

        ObjectRecipe result;

        while (!curr_itr.empty())
        {
            next_itr.clear();

            for (const auto node : curr_itr)
            {
                node->_executed = true;
                result.plan.push_back(node);
            }

            for (const auto node : curr_itr)
            {
                // now add any next node that is ready to execute
                for (auto &pin : node->outputs)
                {
                    auto it = pinLinkLookup.find(pin.id);
                    if (it != pinLinkLookup.end())
                    {
                        for (auto &link : it->second)
                        {
                            auto dst_node = pinNodeLookup[link->inputPinId];
                            if (!dst_node->_executed && !next_itr.contains(dst_node) && dst_node->canExecute())
                            {
                                next_itr.insert(dst_node);
                            }
                        }
                    }
                }
            }

            curr_itr = next_itr;
        }

        for (const auto node : graph)
        {
            if (!node->_executed)
            {
                Logger::error("Tried to build from node: " + std::string(start->_tag) + ", unable to construct a DAG.");
                return std::nullopt;
            }
        }

        result.acceptor = dynamic_cast<Nodes::AcceptorNode *>(start);
        result.type = Agent;

        if (dynamic_cast<Nodes::EnvAcceptorNode *>(result.acceptor))
            result.type = Environment;
        if (dynamic_cast<Nodes::MethodAcceptorNode *>(result.acceptor))
            result.type = Method;

        return result;
    }

    std::vector<ObjectRecipe> build()
    {
        py::gil_scoped_acquire acquire{};

        std::vector<ObjectRecipe> result;
        for (const auto node : nodes)
        {
            if (dynamic_cast<Nodes::AcceptorNode *>(node))
            {
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

    void init(const std::string &graph_file)
    {
        py::gil_scoped_acquire acquire{};

        ed::Config config;
        config.SettingsFile = graph_file.c_str();
        m_Context = ed::CreateEditor(&config);
    }

    void saveSettings(const std::string &graph_file)
    {
        // TODO
    }

    Pin *curr_focus_pin;
    static void switchFocus(Pin *p)
    {
        if (ImGui::IsItemHovered())
        {
            curr_focus_pin = p;
        }
    }

    static void renderPinTooltip()
    {
        if (!curr_focus_pin)
        {
            return;
        }

        auto p = curr_focus_pin;
        ImGui::SetCursorPos({0, 0});
        ImGui::Dummy({0, 0}); // I swear I have no idea how that works ..

        ImGui::BeginTooltip();
        ImGui::Text("%s (%lu)", p->name.c_str(), p->id.Get());
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
        FontManager::pushFont("Light");
        ImGui::Text("%s",p->type_name.c_str());
        FontManager::popFont();
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 150, 150, 255));
        ImGui::Text(p->direction == INPUT ? "Input" : "Output");
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 200, 255));
        if (!p->tooltip.empty())
        {
            ImGui::Text("%s", p->tooltip.c_str());
        }
        ImGui::PopStyleColor();

        ImGui::EndTooltip();
    }

    static void renderDialogs(bool *open_demux_popup)
    {

        ShowIntegerInputDialog(open_demux_popup, "De-Mux Output Count", [](int count)
                               {
                    if (count > 0) {
                        // addNode(new Nodes::DeMuxNode(count));
                    } else {
                        Logger::error("De-Mux input count must be greater than 0");
                    } });
    }

    static void renderMenu(bool *open_demux_popup)
    {
        {
            ImGui::TextDisabled("Primitives:");
            if (ImGui::MenuItem("Integer"))
            {
                _addNode(new Nodes::PrimitiveIntNode(0));
            }

            if (ImGui::MenuItem("Integer (ranged)"))
            {
                _addNode(new Nodes::PrimitiveIntNode(0, 0, 100));
            }

            if (ImGui::MenuItem("Float"))
            {
                _addNode(new Nodes::PrimitiveFloatNode(0));
            }

            if (ImGui::MenuItem("Float (ranged)"))
            {
                _addNode(new Nodes::PrimitiveFloatNode(0, 0.0f, 100.0f));
            }

            if (ImGui::MenuItem("String"))
            {
                _addNode(new Nodes::PrimitiveStringNode(false));
            }

            if (ImGui::MenuItem("File"))
            {
                _addNode(new Nodes::PrimitiveStringNode(true));
            }

            ImGui::Separator();
            ImGui::TextDisabled("Functional:");
            if (ImGui::MenuItem("De-Mux"))
            {
                *open_demux_popup = true;
            }

            ImGui::Separator();
            ImGui::TextDisabled("Pipeline:");

            if (ImGui::MenuItem("Agent"))
            {
                _addNode(new Nodes::AgentAcceptorNode());
            }

            if (ImGui::MenuItem("Environment"))
            {
                _addNode(new Nodes::EnvAcceptorNode());
            }

            // if (ImGui::MenuItem("Mask")) {
            //     addNode(new Nodes::MaskAcceptorNode());
            // }

            if (ImGui::MenuItem("Method"))
            {
                _addNode(new Nodes::MethodAcceptorNode());
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("Objects")) {
                for (auto &module : SharedUi::loadedModules) {
                    if (ImGui::MenuItem(module.moduleName.c_str())) {
                        if (module.type == PyScope::Function) {
                            _addNode(new Nodes::PythonFunctionNode(&module));
                        } else {
                            _addNode(new Nodes::PythonModuleNode(&module));
                        }
                    }
                }

                ImGui::EndMenu();
            }
        }
    }

    void render()
    {
        curr_focus_pin = nullptr;
        static bool first_frame = true;
        ImGui::Begin("Pipeline Graph");

        // dialogs control
        static bool open_demux_popup = false;
        renderDialogs(&open_demux_popup);

        if (ImGui::BeginPopupContextItem("Pipeline Graph Context Menu", ImGuiPopupFlags_MouseButtonRight))
        {
            renderMenu(&open_demux_popup);
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupContextWindow("Pipeline Graph Context Menu - rc", ImGuiPopupFlags_MouseButtonRight))
        {
            renderMenu(&open_demux_popup);
            ImGui::EndPopup();
        }

        ImGui::Text(" %ld node(s)", nodes.size());
        ImGui::SameLine();
        ImGui::Dummy({50, 0});
        ImGui::SameLine();
        auto space = ImGui::GetContentRegionAvail().x;
        auto size = ImGui::CalcTextSize("Build").x + 10;
        auto cursor = ImGui::GetCursorPos();
        cursor.x += space - size;
        ImGui::SetCursorPos(cursor);
        if (ImGui::Button("Build"))
        {
            auto _res = build();
            Pipeline::setRecipes(_res);
        }

        ImGui::Separator();
        ed::SetCurrentEditor(m_Context);
        ed::Begin("Pipeline Graph - Editor", ImVec2(0, 0));

        if (first_frame)
        {
            first_frame = false;
            StartupLoader::load_graph();
        }

        for (auto &node : nodes)
        {
            ed::BeginNode(node->id);
            ImGui::PushID(node->id.AsPointer());
            node->render();
            ImGui::PopID();
            ed::EndNode();
        }

        for (const auto &link : links)
        {
            ed::Link(link->id, link->inputPinId, link->outputPinId);
        }

        if (ed::BeginCreate())
        {
            ed::PinId inputPinId, outputPinId;
            if (ed::QueryNewLink(&outputPinId, &inputPinId))
            {
                _addLink(inputPinId, outputPinId);
            }
        }
        ed::EndCreate();

        if (ed::BeginDelete())
        {
            ed::NodeId deletedNodeId;
            while (ed::QueryDeletedNode(&deletedNodeId))
            {
                removeNode(deletedNodeId);
            }

            ed::LinkId deletedLinkId;
            while (ed::QueryDeletedLink(&deletedLinkId))
            {
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
        ImGui::InvisibleButton("##Dropdown PyScope::LoadedModule", region_size);

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("PyScope::LoadedModule"))
            {
                Logger::info("Dropped a python object into view");
                int index = *static_cast<int *>(payload->Data);
                auto *module = &SharedUi::loadedModules[index];
                if (module)
                {
                    // Create a new node for the dropped module
                    if (module->type == PyScope::Function)
                    {
                        PipelineGraph::_addNode(new Nodes::PythonFunctionNode(module));
                    }
                    else
                    {
                        PipelineGraph::_addNode(new Nodes::PythonModuleNode(module));
                    }
                }
                else
                {
                    Logger::error("Failed to cast payload data to PyScope::LoadedModule");
                }
            }
            ImGui::EndDragDropTarget();
        }

        renderPinTooltip();

        ImGui::End();
    }

    void destroy()
    {
        py::gil_scoped_acquire acquire{};

        ed::DestroyEditor(m_Context);

        for (auto node : nodes) {
            delete node;
        }
        nodes.clear();

        for (auto link : links) {
            delete link;
        }
        links.clear();
    }

    py::object ObjectRecipe::create() const {
        for (auto node : plan) {
            node->init();
        }

        try {
            for (auto node : plan) {
                if (node->canExecute()) {
                    node->exec();
                } else {
                    Logger::error("Node " + std::string(node->_tag) + " cannot be executed, missing inputs or dependencies.");
                    return py::none();
                }
            }
        } catch (py::error_already_set &e) {
            Logger::error("Python error while creating recipe: " + std::string(e.what()));
            return py::none();
        } catch (const std::exception &e) {
            Logger::error("Exception while creating recipe: " + std::string(e.what()));
            return py::none();
        } catch (...) {
            Logger::error("Unknown error while creating recipe");
            return py::none();
        }

        return acceptor->_result;
    }

    static int renderNodeTag(Node *node)
    {
        if (!node->_editing_tag)
        {
            ImGui::PushID("##EditableText");
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(168, 109, 50, 255));
            if (ImGui::Selectable(node->_tag, false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(200, 0)))
            {
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    node->_editing_tag = true;
                    ImGui::SetKeyboardFocusHere();
                }
            }
            ImGui::PopStyleColor();
            ImGui::PopID();
        }
        else
        {
            ImGui::SetNextItemWidth(200);
            if (ImGui::InputText("##edit", node->_tag, IM_ARRAYSIZE(node->_tag), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                node->_editing_tag = false;
            }
            if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0))
            {
                node->_editing_tag = false;
            }
        }

        return 200;
    }

    static void renderNodeAsTable(Node *node, int alloc_size = 0)
    {
        int max_size_in = 1;
        for (auto &input : node->inputs)
        {
            max_size_in = std::max(max_size_in, static_cast<int>(ImGui::CalcTextSize(input.name.c_str()).x));
        }

        int max_size_out = 1;
        for (auto &output : node->outputs)
        {
            max_size_out = std::max(max_size_out, static_cast<int>(ImGui::CalcTextSize(output.name.c_str()).x));
        }

        max_size_in = std::max(max_size_in, alloc_size - max_size_out - (60 + 5 * 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));

        if (ImGui::BeginTable("pin_layout", 2, ImGuiTableFlags_SizingFixedFit, {static_cast<float>(max_size_in + max_size_out + 60 + 5 * 4), 0}))
        {
            // Column 0: auto-fit to content
            ImGui::TableSetupColumn("Inputs", ImGuiTableColumnFlags_WidthFixed, max_size_in + 44);
            // Column 1: fill remaining space
            ImGui::TableSetupColumn("Outputs", ImGuiTableColumnFlags_WidthFixed, max_size_out + 24);
            ImGui::TableNextRow();

            // Left: Input pins
            ImGui::TableSetColumnIndex(0);
            for (auto &input : node->inputs)
            {
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

            for (auto &output : node->outputs)
            {
                float textWidth = ImGui::CalcTextSize(output.name.c_str()).x;
                float iconSize = 20.0f;
                float totalWidth = textWidth + iconSize;

                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availWidth - totalWidth);

                ImGui::Text("%s",output.name.c_str());
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

    Nodes::PrimitiveIntNode::PrimitiveIntNode()
    {
        py::gil_scoped_acquire acquire{};

        id = GetNextId();
        this->name = "Primitive Int";

        inputs.clear();
        Pin output;
        output.id = GetNextId();
        output.name = "value";
        output.tooltip = "Integer value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().int_type;
        output.type_name = py::str(output.type);
        value = 0;
        rangeEnd = -1;
        rangeStart = -1;

        outputs.push_back(output);
    }

    Nodes::PrimitiveIntNode::PrimitiveIntNode(int val)
    {
        py::gil_scoped_acquire acquire{};

        id = GetNextId();
        this->name = "Primitive Int";

        inputs.clear();
        Pin output;
        output.id = GetNextId();
        output.name = "value";
        output.tooltip = "Integer value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().int_type;
        output.type_name = py::str(output.type);
        value = val;
        rangeEnd = -1;
        rangeStart = -1;

        outputs.push_back(output);
    }

    Nodes::PrimitiveIntNode::PrimitiveIntNode(int val, int v_rangeStart, int v_rangeEnd)
    {
        py::gil_scoped_acquire acquire{};

        id = GetNextId();
        this->name = "Primitive Int";

        inputs.clear();
        Pin output;

        output.id = GetNextId();
        output.name = "value";
        output.tooltip = "Integer value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().int_type;
        output.type_name = py::str(output.type);

        value = val;
        rangeStart = v_rangeStart;
        rangeEnd = v_rangeEnd;

        outputs.push_back(output);
    }

    void Nodes::PrimitiveIntNode::exec()
    {
        outputs[0].value = PyScope::getInstance().int_type(value);
        _executed = true;
    }

    void Nodes::PrimitiveIntNode::render()
    {
        ImGui::Text("%s",this->name.c_str());
        auto &elem = outputs[0];

        renderNodeTag(this);

        if (rangeEnd > rangeStart)
        {
            ImGui::SetNextItemWidth(100);
            ImGui::DragInt("##Value", &value, 1.0f, rangeStart, rangeEnd);

            ImGui::PushItemWidth(150);
            ImGui::InputInt("Min", &rangeStart);
            ImGui::InputInt("Max", &rangeEnd);
            ImGui::PopItemWidth();
        }
        else
        {
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

    void Nodes::PrimitiveIntNode::save(nlohmann::json &custom_data)
    {
        Node::save(custom_data);

        custom_data["value"] = value;
        custom_data["rangeStart"] = rangeStart;
        custom_data["rangeEnd"] = rangeEnd;
    }

    void Nodes::PrimitiveIntNode::load(nlohmann::json &custom_data)
    {
        Node::load(custom_data);

        value = custom_data.value("value", 0);
        rangeStart = custom_data.value("rangeStart", 0);
        rangeEnd = custom_data.value("rangeEnd", 0);
    }

    Nodes::PrimitiveFloatNode::PrimitiveFloatNode()
    {
        py::gil_scoped_acquire acquire{};

        id = GetNextId();
        this->name = "Primitive Float";

        inputs.clear();
        Pin output;
        output.id = GetNextId();
        output.name = "value";
        output.tooltip = "Float value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().float_type;
        output.type_name = py::str(output.type);
        value = 0;
        rangeEnd = -1;
        rangeStart = -1;

        outputs.push_back(output);
    }

    Nodes::PrimitiveFloatNode::PrimitiveFloatNode(float val)
    {
        py::gil_scoped_acquire acquire{};

        id = GetNextId();
        this->name = "Primitive Float";

        inputs.clear();
        Pin output;
        output.id = GetNextId();
        output.name = "value";
        output.tooltip = "Float value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().float_type;
        output.type_name = py::str(output.type);
        value = val;
        rangeEnd = -1;
        rangeStart = -1;

        outputs.push_back(output);
    }

    Nodes::PrimitiveFloatNode::PrimitiveFloatNode(float val, float v_rangeStart, float v_rangeEnd)
    {
        py::gil_scoped_acquire acquire{};

        id = GetNextId();
        this->name = "Primitive Float";

        inputs.clear();
        Pin output;

        output.id = GetNextId();
        output.name = "value";
        output.tooltip = "Float value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().float_type;
        output.type_name = py::str(output.type);

        value = val;
        rangeStart = v_rangeStart;
        rangeEnd = v_rangeEnd;

        outputs.push_back(output);
    }

    void Nodes::PrimitiveFloatNode::exec()
    {
        outputs[0].value = PyScope::getInstance().float_type(value);
        _executed = true;
    }

    void Nodes::PrimitiveFloatNode::render()
    {
        ImGui::Text("%s",this->name.c_str());
        auto &elem = outputs[0];

        renderNodeTag(this);

        if (rangeEnd > rangeStart)
        {
            ImGui::SetNextItemWidth(100);
            ImGui::DragFloat("##Value", &value, 1.0f, rangeStart, rangeEnd);

            ImGui::PushItemWidth(150);
            ImGui::InputFloat("Min", &rangeStart);
            ImGui::InputFloat("Max", &rangeEnd);
            ImGui::PopItemWidth();
        }
        else
        {
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

    void Nodes::PrimitiveFloatNode::save(nlohmann::json &custom_data)
    {
        Node::save(custom_data);

        custom_data["value"] = value;
        custom_data["rangeStart"] = rangeStart;
        custom_data["rangeEnd"] = rangeEnd;
    }

    void Nodes::PrimitiveFloatNode::load(nlohmann::json &custom_data)
    {
        Node::load(custom_data);

        value = custom_data.value("value", 0);
        rangeStart = custom_data.value("rangeStart", 0);
        rangeEnd = custom_data.value("rangeEnd", 0);
    }

    Nodes::PrimitiveStringNode::PrimitiveStringNode()
    {
        py::gil_scoped_acquire acquire{};

        _file = false;

        id = GetNextId();
        this->name = "Primitive String";

        inputs.clear();
        Pin output;

        output.id = GetNextId();
        output.name = "value";
        output.tooltip = "String value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().str_type;
        output.type_name = py::str(output.type);

        outputs.push_back(output);
    }

    Nodes::PrimitiveStringNode::PrimitiveStringNode(bool file)
    {
        py::gil_scoped_acquire acquire{};

        _file = file;

        id = GetNextId();
        this->name = "Primitive String";

        inputs.clear();
        Pin output;

        output.id = GetNextId();
        output.name = "value";
        output.tooltip = "String value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().str_type;
        output.type_name = py::str(output.type);

        outputs.push_back(output);
    }

    Nodes::PrimitiveStringNode::PrimitiveStringNode(const std::string &val)
    {
        py::gil_scoped_acquire acquire{};

        _file = false;

        id = GetNextId();
        this->name = "Primitive String";

        // fixme: Not safe
        strcpy(_value, val.c_str());

        inputs.clear();
        Pin output;

        output.id = GetNextId();
        output.name = "value";
        output.tooltip = "String value";
        output.direction = OUTPUT;
        output.type = PyScope::getInstance().str_type;
        output.type_name = py::str(output.type);

        outputs.push_back(output);
    }

    void Nodes::PrimitiveStringNode::exec()
    {
        outputs[0].value = PyScope::getInstance().str_type(_value);
        _executed = true;
    }

    void Nodes::PrimitiveStringNode::render()
    {
        ImGui::Text("%s",this->name.c_str());
        auto &elem = outputs[0];

        renderNodeTag(this);

        std::string key = "FileDlgKeyNodes" + std::to_string(this->id.Get());

        if (!_file)
        {
            ImGui::SetNextItemWidth(200);
            ImGui::InputText(("##" + elem.name).c_str(), _value, 1024);
        }
        else
        {
            ImGui::Text("%s", _value);
            if (ImGui::Button("Select"))
            {
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

        // fixme: dialog is not working correctly
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

    void Nodes::PrimitiveStringNode::save(nlohmann::json &custom_data)
    {
        Node::save(custom_data);
        custom_data["value"] = std::string(_value);
        custom_data["file"] = _file;
    }

    void Nodes::PrimitiveStringNode::load(nlohmann::json &custom_data)
    {
        Node::load(custom_data);

        std::string val = custom_data.value("value", std::string(""));
        _file = custom_data.value("file", false);

        strcpy(this->_value, val.c_str());
    }

    Nodes::AdderNode::AdderNode()
    {
        py::gil_scoped_acquire acquire{};

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

        for (auto& in: inputs) {
            in.type_name = py::str(in.type);
        }

        for (auto& out: outputs) {
            out.type_name = py::str(out.type);
        }
    }

    void Nodes::AdderNode::exec()
    {
        auto &instance = PyScope::getInstance();

        auto a = getPinValue(inputs[0]);
        auto b = getPinValue(inputs[1]);

        if (a.is_none() || b.is_none())
        {
            Logger::warning("Adder node received one or more input with value None");
            if (a.is_none())
                a = instance.int_type(0);
            if (b.is_none())
                b = instance.int_type(0);
        }

        outputs[0].value = a + b;
        _executed = true;
    }

    void Nodes::AdderNode::render()
    {
        ImGui::Text("%s",this->name.c_str());
        renderNodeTag(this);

        ImVec2 tableSize(150, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2)); // Tighter layout (optional)

        if (ImGui::BeginTable("pin_layout", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoBordersInBody, tableSize))
        {
            ImGui::TableSetupColumn("Inputs", ImGuiTableColumnFlags_WidthStretch, 100.0f);
            ImGui::TableSetupColumn("Outputs", ImGuiTableColumnFlags_WidthFixed, 50.0f);

            ImGui::TableNextRow();

            // Left: Input pins
            ImGui::TableSetColumnIndex(0);
            for (auto &input : inputs)
            {
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

    Nodes::PythonModuleNode::PythonModuleNode() : _type(nullptr)
    {
        py::gil_scoped_acquire acquire{};

        id = GetNextId();
        this->name = "Python Module";

        inputs.clear();
        outputs.clear();
    }

    void Nodes::PythonModuleNode::_preparePins()
    {
        Pin output;
        output.id = GetNextId();
        output.name = "obj";
        output.direction = OUTPUT;
        output.tooltip = "The Object";
        output.type = _type->module;
        output.type_name = py::str(_type->module);
        outputs.push_back(output);

        for (auto &param : _type->constructor)
        {
            Pin input;
            input.id = GetNextId();
            input.name = param.attrName;
            input.direction = INPUT;
            input.tooltip = param.disc;
            input.type = param.type;
            inputs.push_back(input);
        }

        for (auto& in: inputs) {
            in.type_name = py::str(in.type);
        }

        for (auto& out: outputs) {
            out.type_name = py::str(out.type);
        }
    }

    Nodes::PythonModuleNode::PythonModuleNode(PyScope::LoadedModule *type)
    {
        py::gil_scoped_acquire acquire{};

        _type = type;

        id = GetNextId();
        this->name = "Python Module";

        inputs.clear();
        outputs.clear();

        _preparePins();
    }

    void Nodes::PythonModuleNode::exec()
    {
        // Set parameters
        std::vector<py::object> constructor_args;
        for (auto &input : inputs)
        {
            if (!isLinked(input.id))
            {
                // Logger::warning(std::format("Input pin '{}' is not connected, using default value (None).", input.name));
                std::ostringstream msg;
                msg << "Input pin '" << input.name << "' is not connected, using default value (None).";
                Logger::warning(msg.str());
            }
            auto val = getPinValue(input);
            constructor_args.push_back(val);
        }

        SafeWrapper::execute([&]
                             {
            py::tuple args(constructor_args.size());
            for (size_t i = 0; i < constructor_args.size(); i++) {
                args[i] = constructor_args[i];
            }

            outputs[0].value = _type->module(*args); // Unpack arguments into Python constructor
            _executed = true; });
    }

    void Nodes::PythonModuleNode::render()
    {
        int max_node_header_size = 0;

        max_node_header_size = std::max(max_node_header_size, static_cast<int>(ImGui::CalcTextSize(this->name.c_str()).x));
        ImGui::Text("%s",this->name.c_str());

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
        FontManager::pushFont("Light");

        std::string name = "<" + _type->moduleName + ">";
        max_node_header_size = std::max(max_node_header_size, static_cast<int>(ImGui::CalcTextSize(name.c_str()).x));
        ImGui::Text("%s",name.c_str());
        FontManager::popFont();
        ImGui::PopStyleColor();

        max_node_header_size = std::max(max_node_header_size, renderNodeTag(this));

        renderNodeAsTable(this, max_node_header_size);
    }

    Nodes::PythonModuleNode::~PythonModuleNode() = default;

    void Nodes::PythonModuleNode::save(nlohmann::json &custom_data)
    {
        SingleOutputNode::save(custom_data);

        custom_data["module"] = _type->moduleName;
    }

    void Nodes::PythonModuleNode::load(nlohmann::json &custom_data)
    {

        std::string moduleName = custom_data.value("module", "");
        if (moduleName == "")
        {
            throw std::runtime_error("No module name specified");
        }

        bool found = false;
        for (auto &mod : SharedUi::loadedModules)
        {
            if (mod.moduleName == moduleName)
            {
                _type = &mod;
                found = true;
                break;
            }
        }

        if (!found)
        {
            throw std::runtime_error("Module: " + moduleName + ", was not loaded.");
        }

        _preparePins();
        SingleOutputNode::load(custom_data);
    }

    Nodes::PythonFunctionNode::PythonFunctionNode()
    {
        py::gil_scoped_acquire acquire{};

        _type = nullptr;
        _pointer = false;

        id = GetNextId();
        this->name = "Python Function";

        inputs.clear();
        outputs.clear();
    }

    Nodes::PythonFunctionNode::PythonFunctionNode(PyScope::LoadedModule *type)
    {
        py::gil_scoped_acquire acquire{};

        _type = type;
        _pointer = false;

        id = GetNextId();
        this->name = "Python Function";

        inputs.clear();
        outputs.clear();

        _preparePins();
    }

    Nodes::PythonFunctionNode::PythonFunctionNode(PyScope::LoadedModule *type, bool pointer)
    {
        py::gil_scoped_acquire acquire{};

        _type = type;
        _pointer = pointer;

        id = GetNextId();
        this->name = "Python Function";

        inputs.clear();
        outputs.clear();

        _preparePins();
    }

    void Nodes::PythonFunctionNode::exec()
    {
        if (_pointer)
        {
            outputs[0].value = _type->module;
            _executed = true;
            return;
        }

        // Set parameters
        std::vector<py::object> constructor_args;
        for (auto &input : inputs)
        {
            if (!isLinked(input.id))
            {
                // Logger::warning(std::format("Input pin '{}' is not connected, using default value (None).", input.name));
                std::ostringstream msg;
                msg << "Input pin '" << input.name << "' is not connected, using default value (None).";
                Logger::warning(msg.str());
            }
            auto val = getPinValue(input);
            constructor_args.push_back(val);
        }

        SafeWrapper::execute([&]
                             {
            py::tuple args(constructor_args.size());
            for (size_t i = 0; i < constructor_args.size(); i++) {
                args[i] = constructor_args[i];
            }

            outputs[0].value = _type->module(*args); // Unpack arguments into Python constructor
            _executed = true; });
    }

    void Nodes::PythonFunctionNode::render()
    {
        int max_node_header_size = 0;

        max_node_header_size = std::max(max_node_header_size, static_cast<int>(ImGui::CalcTextSize(this->name.c_str()).x));
        ImGui::Text("%s",this->name.c_str());

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 255));
        FontManager::pushFont("Light");

        std::string name = "<" + _type->moduleName + ">";
        max_node_header_size = std::max(max_node_header_size, static_cast<int>(ImGui::CalcTextSize(name.c_str()).x));
        ImGui::Text("%s",name.c_str());
        FontManager::popFont();
        ImGui::PopStyleColor();

        max_node_header_size = std::max(max_node_header_size, renderNodeTag(this));

        bool _p = _pointer;
        ImGui::Checkbox("Pointer", &_pointer);

        if (_p != _pointer) {
            // changed, so we should clear all links
            clearNode(this->id);

            py::gil_scoped_acquire acquire{};
            if (_pointer){
                outputs[0].type = _type->module;
                inputs = {};
            } else {
                outputs[0].type = _type->returnType;
                inputs = _inputs;
            }
        }

        if (!_pointer) {
            renderNodeAsTable(this, max_node_header_size);
        } else {
            float textWidth = ImGui::CalcTextSize("func").x;
            int avail_width = std::max(max_node_header_size, (int)textWidth + 20 + 4);
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

    void Nodes::PythonFunctionNode::save(nlohmann::json &custom_data)
    {
        py::gil_scoped_acquire acquire{};

        inputs = _inputs;
        SingleOutputNode::save(custom_data);

        custom_data["module"] = _type->moduleName;
        custom_data["pointer"] = _pointer;
    }

    void Nodes::PythonFunctionNode::load(nlohmann::json &custom_data)
    {
        std::string moduleName = custom_data.value("module", "");
        if (moduleName == "") {
            throw std::runtime_error("No module name specified");
        }

        bool found = false;
        for (auto &mod : SharedUi::loadedModules) {
            if (mod.moduleName == moduleName) {
                _type = &mod;
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("Module: " + moduleName + ", was not loaded.");
        }

        this->_pointer = custom_data["pointer"];

        _preparePins();
        SingleOutputNode::load(custom_data);

        py::gil_scoped_acquire acquire{};
        if (_pointer){
            outputs[0].type = _type->module;
            inputs = {};
        } else {
            outputs[0].type = _type->returnType;
            inputs = _inputs;
        }
    }

    void Nodes::PythonFunctionNode::_preparePins() {
        Pin output;
        output.id = GetNextId();
        output.name = "ret";
        output.direction = OUTPUT;
        output.tooltip = "Function return value or pointer (depending on the mode)";
        output.type = _type->returnType;
        outputs.push_back(output);

        for (auto &param : _type->constructor)
        {
            Pin input;
            input.id = GetNextId();
            input.name = param.attrName;
            input.direction = INPUT;
            input.tooltip = param.disc;
            input.type = param.type;
            _inputs.push_back(input);
        }

        inputs = _inputs;

        for (auto& in: inputs) {
            in.type_name = py::str(in.type);
        }

        for (auto& out: outputs) {
            out.type_name = py::str(out.type);
        }
    }

    Nodes::AcceptorNode::AcceptorNode() = default;

    void Nodes::AcceptorNode::exec()
    {
        _result = getPinValue(inputs[0]);
    }

    void Nodes::AcceptorNode::render()
    {
        ImGui::Text("%s",this->name.c_str());
        renderNodeTag(this);
        ed::BeginPin(inputs[0].id, ed::PinKind::Input);
        ax::Drawing::DrawIcon(ImVec2(20, 20), ax::Drawing::IconType::Circle, pinLinkLookup.contains(inputs[0].id));
        ImGui::Dummy({20, 20});
        ed::EndPin();
        switchFocus(&inputs[0]);
    }

    Nodes::AcceptorNode::~AcceptorNode() = default;

    Nodes::DeMuxNode::DeMuxNode(int numOutputs)
    {
        id = GetNextId();
        this->name = "Demux";
    }

    void Nodes::DeMuxNode::exec()
    {
        py::object inputValue = getPinValue(inputs[0]);
        if (inputValue.is_none())
        {
            Logger::warning("DeMux input is None, cannot demux.");
            return;
        }

        try
        {
            if (py::isinstance<py::list>(inputValue) || py::isinstance<py::tuple>(inputValue))
            {
                py::sequence seq = inputValue;
                for (size_t i = 0; i < std::min(seq.size(), outputs.size()); ++i)
                {
                    py::object item = seq[i];
                    outputs[i].value = item;
                }
            }
            else
            {
                Logger::error("DeMux input is not iterable.");
            }
        }
        catch (...)
        {
            Logger::error("DeMux failed due to unknown error.");
        }
    }

    void Nodes::DeMuxNode::render()
    {
        int max_node_header_size = 0;
        max_node_header_size = std::max(max_node_header_size, static_cast<int>(ImGui::CalcTextSize(this->name.c_str()).x));
        ImGui::Text("%s",this->name.c_str());
        max_node_header_size = std::max(max_node_header_size, renderNodeTag(this));
        renderNodeAsTable(this, max_node_header_size);
    }

    Nodes::DeMuxNode::~DeMuxNode() = default;

    void Nodes::DeMuxNode::save(nlohmann::json &custom_data)
    {
        Node::save(custom_data);

        custom_data["numOutputs"] = static_cast<int>(outputs.size());
    }

    void Nodes::DeMuxNode::load(nlohmann::json &custom_data)
    {
        int pins = custom_data.value("numOutputs", 0);
        _preparePins(pins);
        Node::load(custom_data);
    }

    void Nodes::DeMuxNode::_preparePins(int numOutputs)
    {
        py::gil_scoped_acquire acquire{};

        Pin input;
        input.id = GetNextId();
        input.name = "obj";
        input.direction = INPUT;
        input.tooltip = "The type / list / indexed-object to demux";
        input.type = PyScope::getInstance().object_type;
        inputs.push_back(input);

        for (int i = 0; i < numOutputs; ++i) {
            Pin output;
            output.id = GetNextId();
            // output.name = std::format("obj[{}]", i + 1);
            output.name = "obj[" + std::to_string(i + 1) + "]";
            output.direction = OUTPUT;
            // output.tooltip = std::format("Output #{}", i + 1);
            output.tooltip = "Output #" + std::to_string(i + 1);
            output.type = PyScope::getInstance().object_type;
            outputs.push_back(output);
        }

        for (auto& in: inputs) {
            in.type_name = py::str(in.type);
        }

        for (auto& out: outputs) {
            out.type_name = py::str(out.type);
        }
    }

    Nodes::AgentAcceptorNode::AgentAcceptorNode()
    {
        py::gil_scoped_acquire acquire{};

        id = GetNextId();
        this->name = "Agent Acceptor";

        Pin input;
        input.id = GetNextId();
        input.name = "agent";
        input.direction = INPUT;
        input.tooltip = "The Agent to add to pipeline";
        input.type = PyScope::getInstance().pearl_agent_type;
        inputs.push_back(input);


        for (auto& in: inputs) {
            in.type_name = py::str(in.type);
        }

        for (auto& out: outputs) {
            out.type_name = py::str(out.type);
        }
    }

    Nodes::EnvAcceptorNode::EnvAcceptorNode()
    {
        py::gil_scoped_acquire acquire{};

        id = GetNextId();
        this->name = "Env Acceptor";

        Pin input;
        input.id = GetNextId();
        input.name = "Environment";
        input.direction = INPUT;
        input.tooltip = "The environment to add to pipeline";
        input.type = PyScope::getInstance().pearl_env_type;
        inputs.push_back(input);


        for (auto& in: inputs) {
            in.type_name = py::str(in.type);
        }

        for (auto& out: outputs) {
            out.type_name = py::str(out.type);
        }
    }

    Nodes::MaskAcceptorNode::MaskAcceptorNode()
    {
        py::gil_scoped_acquire acquire{};

        id = GetNextId();
        this->name = "Mask Acceptor";

        Pin input;
        input.id = GetNextId();
        input.name = "mask";
        input.direction = INPUT;
        input.tooltip = "The mask to add to pipeline";
        input.type = PyScope::getInstance().pearl_mask_type;
        inputs.push_back(input);


        for (auto& in: inputs) {
            in.type_name = py::str(in.type);
        }

        for (auto& out: outputs) {
            out.type_name = py::str(out.type);
        }
    }

    Nodes::MethodAcceptorNode::MethodAcceptorNode()
    {
        py::gil_scoped_acquire acquire{};

        id = GetNextId();
        this->name = "Method Acceptor";

        Pin input;
        input.id = GetNextId();
        input.name = "method";
        input.direction = INPUT;
        input.tooltip = "The method to add to pipeline";
        input.type = PyScope::getInstance().pearl_method_type;
        inputs.push_back(input);


        for (auto& in: inputs) {
            in.type_name = py::str(in.type);
        }

        for (auto& out: outputs) {
            out.type_name = py::str(out.type);
        }
    }
}
