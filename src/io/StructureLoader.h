#pragma once
#include <array>
#include <vector>
#include <string>

struct AtomSite
{
    std::string symbol;
    int atomicNumber;

    double x;
    double y;
    double z;

    float r;
    float g;
    float b;
};

struct Structure
{
    std::vector<AtomSite> atoms;

    // Optional unit cell information; when present we can render
    // an accurate lattice-registered bounding box and show all atoms
    // in the full unit cell.
    bool hasUnitCell = false;
    std::array<std::array<double, 3>, 3> cellVectors;
    std::array<double, 3> cellOffset = {0.0, 0.0, 0.0};
};

void getDefaultElementColor(int atomicNumber, float& r, float& g, float& b);

Structure loadStructure(const std::string& filename);

// Save structure to file. format is an OpenBabel format string (e.g. "xyz", "cif", "vasp").
// Returns true on success.
bool saveStructure(const Structure& structure, const std::string& filename, const std::string& format);