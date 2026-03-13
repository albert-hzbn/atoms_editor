#include "FileBrowser.h"

#include "imgui.h"
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>

FileBrowser::FileBrowser()
    : showAbout(false),
      openStructurePopup(false),
      openDir("."),
      historyIndex(-1)
{
    allowedExtensions = {".cif", ".mol", ".pdb", ".xyz", ".sdf"};

    driveRoots.push_back("/");
    if (const char* home = std::getenv("HOME"))
        driveRoots.push_back(home);
    driveRoots.push_back("/mnt");
    driveRoots.push_back("/media");

    openFilename[0] = '\0';
}

void FileBrowser::initFromPath(const std::string& initialPath)
{
    openDir = ".";
    auto pos = initialPath.find_last_of("/\\");
    if (pos != std::string::npos)
        openDir = initialPath.substr(0, pos);

    if (openDir.empty())
        openDir = ".";

    pushHistory(openDir);

    std::string initialOpenFilename = initialPath;
    if (pos != std::string::npos)
        initialOpenFilename = initialPath.substr(pos + 1);

    strncpy(openFilename, initialOpenFilename.c_str(), sizeof(openFilename));
    openFilename[sizeof(openFilename) - 1] = '\0';
}

void FileBrowser::draw(Structure& structure, const std::function<void(const Structure&)>& updateBuffers)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open..."))
                openStructurePopup = true;

            if (ImGui::MenuItem("Quit"))
                glfwSetWindowShouldClose(glfwGetCurrentContext(), true);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About"))
                showAbout = true;

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (openStructurePopup)
    {
        ImGui::OpenPopup("Open Structure");
        openStructurePopup = false;
    }

    if (ImGui::BeginPopupModal("Open Structure", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Current folder: %s", openDir.c_str());
        ImGui::SameLine();
        if (ImGui::Button(".."))
        {
            auto pos = openDir.find_last_of("/\\");
            if (pos != std::string::npos)
                openDir = openDir.substr(0, pos);
            else
                openDir = ".";
            pushHistory(openDir);
        }

        ImGui::SameLine();
        if (ImGui::Button("Back") && historyIndex > 0)
        {
            historyIndex--;
            openDir = dirHistory[historyIndex];
        }
        ImGui::SameLine();
        if (ImGui::Button("Forward") && historyIndex + 1 < (int)dirHistory.size())
        {
            historyIndex++;
            openDir = dirHistory[historyIndex];
        }

        ImGui::SameLine();
        if (ImGui::Button("Root"))
        {
            openDir = "/";
            pushHistory(openDir);
        }
        ImGui::SameLine();
        if (ImGui::Button("Home") && !driveRoots.empty())
        {
            openDir = driveRoots[1];
            pushHistory(openDir);
        }

        ImGui::Separator();

        if (ImGui::BeginChild("##filebrowser", ImVec2(500, 300), true))
        {
            DIR* dir = opendir(openDir.c_str());
            if (dir)
            {
                struct dirent* de;
                std::vector<std::pair<std::string, bool>> entries;

                while ((de = readdir(dir)) != nullptr)
                {
                    std::string name(de->d_name);
                    if (name == "." || name == "..")
                        continue;

                    std::string fullPath = openDir + "/" + name;
                    struct stat st;
                    bool isDir = (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode));

                    if (!isDir && !isAllowedFile(name))
                        continue;

                    entries.emplace_back(name, isDir);
                }
                closedir(dir);

                // directories first
                std::sort(entries.begin(), entries.end(),
                          [](const std::pair<std::string, bool>& a,
                             const std::pair<std::string, bool>& b) {
                    if (a.second != b.second)
                        return a.second > b.second;
                    return a.first < b.first;
                });

                for (size_t i = 0; i < entries.size(); ++i)
                {
                    const std::string& name = entries[i].first;
                    bool isDir = entries[i].second;

                    ImGui::PushID((int)i);
                    if (isDir)
                    {
                        std::string label = std::string("[DIR] ") + name + "##" + name;
                        if (ImGui::Selectable(label.c_str()))
                        {
                            if (openDir == ".")
                                openDir = name;
                            else
                                openDir = openDir + "/" + name;
                            pushHistory(openDir);
                        }
                    }
                    else
                    {
                        std::string label = name + "##" + name;
                        bool selected = (std::string(openFilename) == name);
                        if (ImGui::Selectable(label.c_str(), selected))
                        {
                            strncpy(openFilename, name.c_str(), sizeof(openFilename));
                            openFilename[sizeof(openFilename) - 1] = '\0';
                        }
                    }
                    ImGui::PopID();
                }
            }
            else
            {
                ImGui::TextDisabled("Unable to open folder");
            }

            ImGui::EndChild();
        }

        ImGui::InputText("Filename", openFilename, sizeof(openFilename));

        if (ImGui::Button("Load"))
        {
            std::string fullPath = openDir + "/" + openFilename;
            Structure newStructure = loadStructure(fullPath);
            if (!newStructure.atoms.empty())
            {
                structure = std::move(newStructure);
                updateBuffers(structure);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (showAbout)
    {
        ImGui::OpenPopup("About");
        showAbout = false;
    }

    if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Atoms Editor");
        ImGui::Text("Open-source molecular visualization demo.");
        ImGui::Separator();
        ImGui::TextWrapped("Controls:");
        ImGui::BulletText("Left drag: rotate camera");
        ImGui::BulletText("Scroll: zoom");
        ImGui::TextWrapped("Use File -> Open to browse and load structure files.");
        ImGui::Separator();
        if (ImGui::Button("OK"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void FileBrowser::pushHistory(const std::string& dir)
{
    if (historyIndex + 1 < (int)dirHistory.size())
        dirHistory.erase(dirHistory.begin() + historyIndex + 1, dirHistory.end());

    dirHistory.push_back(dir);
    historyIndex = (int)dirHistory.size() - 1;
}

std::string FileBrowser::toLower(const std::string& s)
{
    std::string out = s;
    for (auto& c : out)
        c = (char)std::tolower((unsigned char)c);
    return out;
}

bool FileBrowser::isAllowedFile(const std::string& name) const
{
    auto dot = name.find_last_of('.');
    if (dot == std::string::npos)
        return false;

    std::string ext = toLower(name.substr(dot));
    for (const auto& allowed : allowedExtensions)
    {
        if (ext == allowed)
            return true;
    }
    return false;
}
