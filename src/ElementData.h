#pragma once

#include <vector>
#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// Periodic action requested from the context menu
// ---------------------------------------------------------------------------

enum class PeriodicAction
{
    None,
    Substitute,
    InsertMidpoint,
};

// Return the chemical symbol for atomic number z (1–118), or "?" otherwise.
const char* elementSymbol(int z);

// Covalent radii (Angstrom) from Cordero et al., Dalton Trans. 2008.
// Returns a vector of size 119 indexed by atomic number (index 0 unused).
std::vector<float> makeLiteratureCovalentRadii();

// CPK-style default colours for each element.
// Returns a vector of size 119 indexed by atomic number (index 0 unused).
std::vector<glm::vec3> makeDefaultElementColors();
