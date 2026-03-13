#pragma once

#include "StructureLoader.h"

#include <array>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

struct FileBrowser
{
    FileBrowser();

    // Initialize browser state from a starting path.
    void initFromPath(const std::string& initialPath);

    // Draw the File -> Open UI and handle loading.
    // updateBuffers is called whenever a new structure is loaded.
    void draw(Structure& structure, const std::function<void(const Structure&)>& updateBuffers);

    bool isSupercellEnabled() const { return showSupercell; }
    bool isTransformMatrixEnabled() const { return useTransformMatrix; }
    const int (&getTransformMatrix() const)[3][3] { return transformMatrix; }

private:
    void pushHistory(const std::string& dir);
    static std::string toLower(const std::string& s);
    bool isAllowedFile(const std::string& name) const;

    bool showAbout;
    bool showEditColors;
    bool showSupercell;
    bool showTransformDialog;
    bool openStructurePopup;

    std::string openDir;
    std::vector<std::string> dirHistory;
    int historyIndex;

    int selectedAtomicNumber;

    // Map of atomic number to user-adjusted color
    std::unordered_map<int, std::array<float, 3>> elementColorOverrides;

    std::vector<std::string> driveRoots;
    std::vector<std::string> allowedExtensions;

    char openFilename[1024];

    // 3x3 transformation matrix for supercell
    int transformMatrix[3][3] = {
        {3,0,0},
        {0,3,0},
        {0,0,3}
    };
    bool useTransformMatrix = false;
};
