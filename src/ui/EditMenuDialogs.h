#pragma once

#include <vector>
#include <functional>
#include <glm/glm.hpp>

#include "io/StructureLoader.h"

// Owns the per-element radius and colour tables, and draws the Edit menu
// items + the "Atomic Sizes" and "Element Colors" modal dialogs.
struct EditMenuDialogs
{
    // Public state so the updateBuffers lambda in main can read these.
    std::vector<float>     elementRadii;
    std::vector<glm::vec3> elementColors;

    // Initialise radii and colours to literature / CPK defaults.
    EditMenuDialogs();

    // Call once per ImGui frame.  Draws the Edit menu entries and, when open,
    // the two modal popups.  Calls updateBuffers(structure) when the user
    // changes a value.
    void draw(Structure& structure,
              const std::function<void(Structure&)>& updateBuffers);

private:
    int m_selectedRadiusElement = 6;   // Carbon default
    int m_selectedColorElement  = 6;
};
