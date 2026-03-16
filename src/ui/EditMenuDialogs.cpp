#include "EditMenuDialogs.h"

#include "ElementData.h"
#include "PeriodicTableDialog.h"
#include "io/StructureLoader.h"

#include "imgui.h"

EditMenuDialogs::EditMenuDialogs()
    : elementRadii(makeLiteratureCovalentRadii())
    , elementColors(makeDefaultElementColors())
{}

void EditMenuDialogs::drawMenuItems()
{
    if (ImGui::MenuItem("Atomic Sizes..."))
        m_openAtomicSize = true;

    if (ImGui::MenuItem("Element Colors..."))
        m_openElementColor = true;
}

void EditMenuDialogs::drawPopups(Structure& structure,
                                 const std::function<void(Structure&)>& updateBuffers)
{
    if (m_openAtomicSize)   { ImGui::OpenPopup("Atomic Sizes##edit");   m_openAtomicSize   = false; }
    if (m_openElementColor) { ImGui::OpenPopup("Element Colors##edit"); m_openElementColor = false; }

    // ------------------------------------------------------------------
    // Atomic Sizes modal
    // ------------------------------------------------------------------

    if (ImGui::BeginPopupModal("Atomic Sizes##edit", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Select an element in the periodic table and edit its radius.");
        ImGui::Text("Defaults: Cordero et al., Dalton Trans. 2008.");

        if (ImGui::Button("Reset to Literature Defaults"))
        {
            elementRadii = makeLiteratureCovalentRadii();
            updateBuffers(structure);
        }

        ImGui::SameLine();
        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();

        ImGui::Separator();
        drawPeriodicTableInlineSelector(m_selectedRadiusElement);

        if (m_selectedRadiusElement >= 1 && m_selectedRadiusElement <= 118)
        {
            ImGui::Text("Selected: Z=%d (%s)",
                        m_selectedRadiusElement,
                        elementSymbol(m_selectedRadiusElement));

            float radius = elementRadii[m_selectedRadiusElement];
            if (ImGui::DragFloat("Atomic Radius (A)", &radius,
                                 0.01f, 0.20f, 3.50f, "%.2f A"))
            {
                elementRadii[m_selectedRadiusElement] = radius;
                updateBuffers(structure);
            }

            if (ImGui::Button("Apply Literature Radius To Selected Element"))
            {
                const std::vector<float> defaults = makeLiteratureCovalentRadii();
                elementRadii[m_selectedRadiusElement] =
                    defaults[m_selectedRadiusElement];
                updateBuffers(structure);
            }
        }
        else
        {
            ImGui::Text("Selected: None");
        }

        ImGui::EndPopup();
    }

    // ------------------------------------------------------------------
    // Element Colors modal
    // ------------------------------------------------------------------

    if (ImGui::BeginPopupModal("Element Colors##edit", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Select an element in the periodic table and edit its color.");
        ImGui::Separator();

        drawPeriodicTableInlineSelector(m_selectedColorElement);

        if (m_selectedColorElement >= 1 && m_selectedColorElement <= 118)
        {
            ImGui::Text("Selected: Z=%d (%s)",
                        m_selectedColorElement,
                        elementSymbol(m_selectedColorElement));

            float color[3] = {
                elementColors[m_selectedColorElement].r,
                elementColors[m_selectedColorElement].g,
                elementColors[m_selectedColorElement].b
            };

            if (ImGui::ColorEdit3("Element Color", color))
            {
                elementColors[m_selectedColorElement] =
                    glm::vec3(color[0], color[1], color[2]);
                updateBuffers(structure);
            }

            if (ImGui::Button("Reset Selected Element Color"))
            {
                float r, g, b;
                getDefaultElementColor(m_selectedColorElement, r, g, b);
                elementColors[m_selectedColorElement] = glm::vec3(r, g, b);
                updateBuffers(structure);
            }
        }
        else
        {
            ImGui::Text("Selected: None");
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset All Colors"))
        {
            elementColors = makeDefaultElementColors();
            updateBuffers(structure);
        }

        ImGui::SameLine();
        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}
