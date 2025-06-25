#include "objects_panel.hpp"

#include <imgui.h>

#include "../shared_ui.hpp"

void ObjectsPanel::init()
{
}

void ObjectsPanel::render()
{
    static char nameFilter[128] = "";
    static bool showAgents = true;
    static bool showEnvironments = true;
    static bool showMethods = true;
    static bool showMasks = true;
    static bool showFunctions = true;
    static bool showOthers = true;

    ImGui::Begin("Objects");

    // Right-click context menu on window header
    if (ImGui::BeginPopupContextItem("Objects", ImGuiPopupFlags_MouseButtonRight))
    {
        ImGui::Text("Filter by Type:");
        ImGui::Separator();
        ImGui::Checkbox("Agent (A)", &showAgents);
        ImGui::Checkbox("Environment (E)", &showEnvironments);
        ImGui::Checkbox("Method (M)", &showMethods);
        ImGui::Checkbox("Mask (K)", &showMasks);
        ImGui::Checkbox("Function (F)", &showFunctions);
        ImGui::Checkbox("Other (U)", &showOthers);
        ImGui::EndPopup();
    }

    // Search by name
    ImGui::InputText("##Filter", nameFilter, IM_ARRAYSIZE(nameFilter));
    ImGui::SameLine();
    ImGui::TextUnformatted("Name Filter");

    ImGui::Separator();

    auto &objects = SharedUi::loadedModules;
    int size = objects.size();

    std::string filterStr = nameFilter;
    std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

    if (size == 0)
    {
        ImGui::Text("No objects loaded.");
    }
    else
    {
        int visibleCount = 0;
        for (int i = 0; i < size; ++i)
        {
            PyScope::LoadedModule &module = objects[i];

            // Filter by type
            bool visibleByType =
                (module.type == PyScope::Agent && showAgents) ||
                (module.type == PyScope::Environment && showEnvironments) ||
                (module.type == PyScope::Method && showMethods) ||
                (module.type == PyScope::Mask && showMasks) ||
                (module.type == PyScope::Function && showFunctions) ||
                (module.type == PyScope::Other && showOthers);

            if (!visibleByType)
                continue;

            std::string moduleNameLower = module.moduleName;
            std::transform(moduleNameLower.begin(), moduleNameLower.end(), moduleNameLower.begin(), ::tolower);
            if (!filterStr.empty() && moduleNameLower.find(filterStr) == std::string::npos)
                continue;

            std::string typeName = "U";
            if (module.type == PyScope::Agent)
                typeName = "A";
            else if (module.type == PyScope::Environment)
                typeName = "E";
            else if (module.type == PyScope::Method)
                typeName = "M";
            else if (module.type == PyScope::Mask)
                typeName = "K";
            else if (module.type == PyScope::Function)
                typeName = "F";

            ImGui::Text("%s", typeName.c_str());
            ImGui::SameLine();

            ImGui::SetCursorPos({30, ImGui::GetCursorPos().y});

            ImGui::Selectable(module.moduleName.c_str());

            // Drag-and-drop support
            if (ImGui::BeginDragDropSource())
            {
                ImGui::SetDragDropPayload("PyScope::LoadedModule", &i, sizeof(int));
                ImGui::Text("Dragging: %s", module.moduleName.c_str());
                ImGui::EndDragDropSource();
            }

            visibleCount++;
        }

        if (visibleCount == 0)
            ImGui::Text("No objects match filters.");
    }

    ImGui::End();
}

void ObjectsPanel::destroy()
{
}
