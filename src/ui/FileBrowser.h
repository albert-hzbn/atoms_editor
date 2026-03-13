#pragma once

#include "StructureLoader.h"

#include <functional>
#include <string>
#include <vector>

struct FileBrowser
{
    FileBrowser();

    // Initialize browser state from a starting path.
    void initFromPath(const std::string& initialPath);

    // Draw the File -> Open UI and handle loading.
    // updateBuffers is called whenever a new structure is loaded.
    void draw(Structure& structure, const std::function<void(const Structure&)>& updateBuffers);

private:
    void pushHistory(const std::string& dir);
    static std::string toLower(const std::string& s);
    bool isAllowedFile(const std::string& name) const;

    bool showAbout;
    bool openStructurePopup;

    std::string openDir;
    std::vector<std::string> dirHistory;
    int historyIndex;

    std::vector<std::string> driveRoots;
    std::vector<std::string> allowedExtensions;

    char openFilename[1024];
};
