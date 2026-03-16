#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"

#include "Camera.h"
#include "SphereMesh.h"
#include "ShadowMap.h"
#include "StructureLoader.h"
#include "StructureInstanceBuilder.h"
#include "SceneBuffers.h"
#include "Renderer.h"
#include "ui/FileBrowser.h"
#include "ui/ImGuiSetup.h"
#include "ui/PeriodicTableDialog.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdio>

// ---------------------------------------------------------------------------
// Ray-picking helpers
// ---------------------------------------------------------------------------

static glm::vec3 pickRayDir(double mx, double my, int w, int h,
                             const glm::mat4& projection,
                             const glm::mat4& view)
{
    float ndcX =  2.0f * (float)mx / (float)w - 1.0f;
    float ndcY = -2.0f * (float)my / (float)h + 1.0f;

    glm::mat4 invVP = glm::inverse(projection * view);

    glm::vec4 nearP = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 farP  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);

    nearP /= nearP.w;
    farP  /= farP.w;

    return glm::normalize(glm::vec3(farP - nearP));
}

static int pickAtom(const glm::vec3& origin, const glm::vec3& dir,
                    const std::vector<glm::vec3>& positions,
                    const std::vector<float>& radii,
                    float fallbackRadius)
{
    int   best  = -1;
    float bestT = 1e30f;

    for (int i = 0; i < (int)positions.size(); ++i)
    {
        glm::vec3 oc = positions[i] - origin;
        float t  = glm::dot(oc, dir);
        if (t < 0.0f) continue;

        float radius = fallbackRadius;
        if (i < (int)radii.size() && radii[i] > 0.0f)
            radius = radii[i];

        float d2 = glm::dot(oc, oc) - t * t;
        if (d2 < radius * radius && t < bestT)
        {
            best  = i;
            bestT = t;
        }
    }
    return best;
}

enum class PeriodicAction
{
    None,
    Substitute,
    InsertMidpoint,
};

static const char* elementSymbol(int z)
{
    static const char* kSymbols[119] = {
        "",
        "H","He","Li","Be","B","C","N","O","F","Ne",
        "Na","Mg","Al","Si","P","S","Cl","Ar","K","Ca",
        "Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn",
        "Ga","Ge","As","Se","Br","Kr","Rb","Sr","Y","Zr",
        "Nb","Mo","Tc","Ru","Rh","Pd","Ag","Cd","In","Sn",
        "Sb","Te","I","Xe","Cs","Ba","La","Ce","Pr","Nd",
        "Pm","Sm","Eu","Gd","Tb","Dy","Ho","Er","Tm","Yb",
        "Lu","Hf","Ta","W","Re","Os","Ir","Pt","Au","Hg",
        "Tl","Pb","Bi","Po","At","Rn","Fr","Ra","Ac","Th",
        "Pa","U","Np","Pu","Am","Cm","Bk","Cf","Es","Fm",
        "Md","No","Lr","Rf","Db","Sg","Bh","Hs","Mt","Ds",
        "Rg","Cn","Nh","Fl","Mc","Lv","Ts","Og"
    };

    if (z >= 1 && z <= 118)
        return kSymbols[z];
    return "?";
}

static std::vector<float> makeLiteratureCovalentRadii()
{
    // Covalent radii defaults (Angstrom) from Cordero et al., Dalton Trans. 2008.
    std::vector<float> radii(119, 1.60f);
    radii[0] = 1.0f;

    radii[1] = 0.31f; radii[2] = 0.28f; radii[3] = 1.28f; radii[4] = 0.96f; radii[5] = 0.84f;
    radii[6] = 0.76f; radii[7] = 0.71f; radii[8] = 0.66f; radii[9] = 0.57f; radii[10] = 0.58f;
    radii[11] = 1.66f; radii[12] = 1.41f; radii[13] = 1.21f; radii[14] = 1.11f; radii[15] = 1.07f;
    radii[16] = 1.05f; radii[17] = 1.02f; radii[18] = 1.06f; radii[19] = 2.03f; radii[20] = 1.76f;
    radii[21] = 1.70f; radii[22] = 1.60f; radii[23] = 1.53f; radii[24] = 1.39f; radii[25] = 1.39f;
    radii[26] = 1.32f; radii[27] = 1.26f; radii[28] = 1.24f; radii[29] = 1.32f; radii[30] = 1.22f;
    radii[31] = 1.22f; radii[32] = 1.20f; radii[33] = 1.19f; radii[34] = 1.20f; radii[35] = 1.20f;
    radii[36] = 1.16f; radii[37] = 2.20f; radii[38] = 1.95f; radii[39] = 1.90f; radii[40] = 1.75f;
    radii[41] = 1.64f; radii[42] = 1.54f; radii[43] = 1.47f; radii[44] = 1.46f; radii[45] = 1.42f;
    radii[46] = 1.39f; radii[47] = 1.45f; radii[48] = 1.44f; radii[49] = 1.42f; radii[50] = 1.39f;
    radii[51] = 1.39f; radii[52] = 1.38f; radii[53] = 1.39f; radii[54] = 1.40f; radii[55] = 2.44f;
    radii[56] = 2.15f; radii[57] = 2.07f; radii[58] = 2.04f; radii[59] = 2.03f; radii[60] = 2.01f;
    radii[61] = 1.99f; radii[62] = 1.98f; radii[63] = 1.98f; radii[64] = 1.96f; radii[65] = 1.94f;
    radii[66] = 1.92f; radii[67] = 1.92f; radii[68] = 1.89f; radii[69] = 1.90f; radii[70] = 1.87f;
    radii[71] = 1.87f; radii[72] = 1.75f; radii[73] = 1.70f; radii[74] = 1.62f; radii[75] = 1.51f;
    radii[76] = 1.44f; radii[77] = 1.41f; radii[78] = 1.36f; radii[79] = 1.36f; radii[80] = 1.32f;
    radii[81] = 1.45f; radii[82] = 1.46f; radii[83] = 1.48f; radii[84] = 1.40f; radii[85] = 1.50f;
    radii[86] = 1.50f; radii[87] = 2.60f; radii[88] = 2.21f; radii[89] = 2.15f; radii[90] = 2.06f;
    radii[91] = 2.00f; radii[92] = 1.96f; radii[93] = 1.90f; radii[94] = 1.87f; radii[95] = 1.80f;
    radii[96] = 1.69f;

    return radii;
}

static std::vector<glm::vec3> makeDefaultElementColors()
{
    std::vector<glm::vec3> colors(119, glm::vec3(0.7f, 0.7f, 0.7f));
    colors[0] = glm::vec3(0.7f, 0.7f, 0.7f);
    for (int z = 1; z <= 118; ++z)
    {
        float r, g, b;
        getDefaultElementColor(z, r, g, b);
        colors[z] = glm::vec3(r, g, b);
    }
    return colors;
}

// ---------------------------------------------------------------------------
int main()
{
    // ----------------------------------------------------------------
    // Window
    // ----------------------------------------------------------------

    if (!glfwInit())
        return -1;

    GLFWwindow* window =
        glfwCreateWindow(1280, 800, "Atoms Editor", nullptr, nullptr);

    if (!window)
        return -1;

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
        return -1;

    glEnable(GL_DEPTH_TEST);

    // ----------------------------------------------------------------
    // Camera
    // ----------------------------------------------------------------

    Camera camera;
    Camera::instance = &camera;

    glfwSetMouseButtonCallback(window, Camera::mouseButton);
    glfwSetCursorPosCallback(window,   Camera::cursor);
    glfwSetScrollCallback(window,      Camera::scroll);

    // ----------------------------------------------------------------
    // ImGui
    // ----------------------------------------------------------------

    initImGui(window);

    // ----------------------------------------------------------------
    // Geometry, structure, UI
    // ----------------------------------------------------------------

    SphereMesh sphere(40, 40);

    std::string filename = "";
    Structure structure = loadStructure(filename);

    FileBrowser fileBrowser;
    fileBrowser.initFromPath(filename);

    // ----------------------------------------------------------------
    // GPU resources
    // ----------------------------------------------------------------

    SceneBuffers sceneBuffers;
    sceneBuffers.init(sphere.vao);

    Renderer renderer;
    renderer.init();

    ShadowMap shadow = createShadowMap(1024, 1024);

    // ----------------------------------------------------------------
    // Picking / selection state (declared before lambda so it is captured)
    // ----------------------------------------------------------------

    std::vector<int> selectedInstanceIndices;  // Multiple selected atoms
    bool openContextMenu     = false;
    PeriodicAction pendingPeriodicAction = PeriodicAction::None;
    std::vector<float> elementRadii = makeLiteratureCovalentRadii();
    std::vector<glm::vec3> elementColors = makeDefaultElementColors();
    int selectedRadiusElement = 6; // Carbon by default
    int selectedColorElement = 6;  // Carbon by default

    // ----------------------------------------------------------------
    // Buffer update helper
    // ----------------------------------------------------------------

    auto updateBuffers = [&](Structure& s) {
        for (auto& atom : s.atoms)
        {
            int z = atom.atomicNumber;
            if (z >= 0 && z < (int)elementColors.size())
            {
                atom.r = elementColors[z].r;
                atom.g = elementColors[z].g;
                atom.b = elementColors[z].b;
            }
        }

        StructureInstanceData data = buildStructureInstanceData(
            s,
            fileBrowser.isTransformMatrixEnabled(),
            fileBrowser.getTransformMatrix(),
            elementRadii);

        sceneBuffers.upload(data);
        selectedInstanceIndices.clear();  // clear selection whenever structure rebuilds
    };

    updateBuffers(structure);

    // ----------------------------------------------------------------
    // Render loop
    // ----------------------------------------------------------------

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        if (w == 0 || h == 0) { glfwSwapBuffers(window); continue; }

        // ------------------------------------------------------------
        // Matrices  (computed early so picking can use them)
        // ------------------------------------------------------------

        glm::mat4 projection =
            glm::perspective(glm::radians(45.0f), (float)w / h, 0.1f, 1000.0f);

        float yaw   = glm::radians(camera.yaw);
        float pitch = glm::radians(camera.pitch);

        glm::vec3 camOffset(
            camera.distance * std::cos(pitch) * std::sin(yaw),
            camera.distance * std::sin(pitch),
            camera.distance * std::cos(pitch) * std::cos(yaw));

        glm::vec3 camPos = sceneBuffers.orbitCenter + camOffset;

        glm::mat4 view =
            glm::lookAt(camPos, sceneBuffers.orbitCenter, glm::vec3(0, 1, 0));

        // use window size (logical pixels) to match GLFW cursor coordinates
        int winW, winH;
        glfwGetWindowSize(window, &winW, &winH);
        if (winW == 0) winW = w;
        if (winH == 0) winH = h;

        glm::mat4 lightProj =
            glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 1000.0f);

        glm::vec3 lightPos = sceneBuffers.orbitCenter + glm::vec3(40.0f, 40.0f, 40.0f);
        glm::mat4 lightMVP =
            lightProj * glm::lookAt(lightPos, sceneBuffers.orbitCenter, glm::vec3(0, 1, 0));

        // ------------------------------------------------------------
        // Atom picking  (left click without drag)
        // ------------------------------------------------------------

        if (camera.pendingClick)
        {
            camera.pendingClick = false;
            glm::vec3 ray = pickRayDir(camera.clickX, camera.clickY,
                                       winW, winH, projection, view);
            int newIdx = pickAtom(camPos, ray,
                                  sceneBuffers.atomPositions,
                                  sceneBuffers.atomRadii,
                                  1.0f);
            
            if (newIdx >= 0)
            {
                // Check if Ctrl is held for multi-select
                bool ctrlHeld = ImGui::GetIO().KeyCtrl;
                
                if (ctrlHeld)
                {
                    // Multi-select: toggle this atom in selection
                    auto it = std::find(selectedInstanceIndices.begin(), 
                                       selectedInstanceIndices.end(), newIdx);
                    if (it != selectedInstanceIndices.end())
                    {
                        // Already selected, deselect it
                        sceneBuffers.restoreAtomColor(newIdx);
                        selectedInstanceIndices.erase(it);
                    }
                    else
                    {
                        // Not selected, add it
                        selectedInstanceIndices.push_back(newIdx);
                    }
                }
                else
                {
                    // Regular click: replace selection
                    // Restore colors of previously selected atoms
                    for (int idx : selectedInstanceIndices)
                        sceneBuffers.restoreAtomColor(idx);
                    
                    selectedInstanceIndices.clear();
                    selectedInstanceIndices.push_back(newIdx);
                }
            }
            else
            {
                // Clicked on empty space: clear selection
                for (int idx : selectedInstanceIndices)
                    sceneBuffers.restoreAtomColor(idx);
                selectedInstanceIndices.clear();
            }
        }

        // ------------------------------------------------------------
        // Right-click  → open context menu if an atom is selected
        // ------------------------------------------------------------

        if (camera.pendingRightClick)
        {
            camera.pendingRightClick = false;
            if (!selectedInstanceIndices.empty())
                openContextMenu = true;
        }

        // ------------------------------------------------------------
        // ImGui frame
        // ------------------------------------------------------------

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ------------------------------------------------------------
        bool doDeleteSelected = false;
        bool doOpenPeriodicTable = false;
        // Keyboard shortcuts
        // ------------------------------------------------------------

        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && !selectedInstanceIndices.empty())
        {
            doDeleteSelected = true;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_D) && ImGui::GetIO().KeyCtrl && !selectedInstanceIndices.empty())
        {
            // Ctrl+D: Deselect all
            for (int idx : selectedInstanceIndices)
                sceneBuffers.restoreAtomColor(idx);
            selectedInstanceIndices.clear();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape) && !selectedInstanceIndices.empty())
        {
            // Escape: Deselect all
            for (int idx : selectedInstanceIndices)
                sceneBuffers.restoreAtomColor(idx);
            selectedInstanceIndices.clear();
        }

        // Ctrl+A: Select all atoms
        if (ImGui::IsKeyPressed(ImGuiKey_A) && ImGui::GetIO().KeyCtrl && structure.atoms.size() > 0)
        {
            selectedInstanceIndices.clear();
            for (int i = 0; i < (int)sceneBuffers.atomIndices.size(); ++i)
            {
                selectedInstanceIndices.push_back(i);
            }
        }

        // Ctrl+O: Open file dialog
        if (ImGui::IsKeyPressed(ImGuiKey_O) && ImGui::GetIO().KeyCtrl)
        {
            fileBrowser.openFileDialog();
        }

        bool openAtomicSizeDialog = false;
        bool openElementColorDialog = false;
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Atomic Sizes..."))
                    openAtomicSizeDialog = true;

                if (ImGui::MenuItem("Element Colors..."))
                    openElementColorDialog = true;

                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (openAtomicSizeDialog)
            ImGui::OpenPopup("Atomic Sizes##edit");
        if (openElementColorDialog)
            ImGui::OpenPopup("Element Colors##edit");

        if (ImGui::BeginPopupModal("Atomic Sizes##edit", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
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
            drawPeriodicTableInlineSelector(selectedRadiusElement);

            if (selectedRadiusElement >= 1 && selectedRadiusElement <= 118)
            {
                ImGui::Text("Selected: Z=%d (%s)", selectedRadiusElement, elementSymbol(selectedRadiusElement));
                float radius = elementRadii[selectedRadiusElement];
                if (ImGui::DragFloat("Atomic Radius (A)", &radius, 0.01f, 0.20f, 3.50f, "%.2f A"))
                {
                    elementRadii[selectedRadiusElement] = radius;
                    updateBuffers(structure);
                }
            }
            else
            {
                ImGui::Text("Selected: None");
            }

            if (ImGui::Button("Apply Literature Radius To Selected Element") &&
                selectedRadiusElement >= 1 && selectedRadiusElement <= 118)
            {
                const std::vector<float> defaults = makeLiteratureCovalentRadii();
                elementRadii[selectedRadiusElement] = defaults[selectedRadiusElement];
                updateBuffers(structure);
            }

            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Element Colors##edit", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Select an element in the periodic table and edit its color.");
            ImGui::Separator();

            drawPeriodicTableInlineSelector(selectedColorElement);

            if (selectedColorElement >= 1 && selectedColorElement <= 118)
            {
                ImGui::Text("Selected: Z=%d (%s)", selectedColorElement, elementSymbol(selectedColorElement));

                float color[3] = {
                    elementColors[selectedColorElement].r,
                    elementColors[selectedColorElement].g,
                    elementColors[selectedColorElement].b
                };

                if (ImGui::ColorEdit3("Element Color", color))
                {
                    elementColors[selectedColorElement] = glm::vec3(color[0], color[1], color[2]);
                    updateBuffers(structure);
                }

                if (ImGui::Button("Reset Selected Element Color"))
                {
                    float r, g, b;
                    getDefaultElementColor(selectedColorElement, r, g, b);
                    elementColors[selectedColorElement] = glm::vec3(r, g, b);
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

        fileBrowser.draw(structure, updateBuffers);

        // Context menu (must be outside any Begin/End window block)
        if (openContextMenu)
        {
            ImGui::OpenPopup("##atomCtx");
            openContextMenu = false;
        }
        if (ImGui::BeginPopup("##atomCtx"))
        {
            if (ImGui::MenuItem("Substitute Atom..."))
            {
                pendingPeriodicAction = PeriodicAction::Substitute;
                doOpenPeriodicTable = true;
            }

            bool canInsertAtMidpoint = selectedInstanceIndices.size() >= 2;
            if (!canInsertAtMidpoint)
                ImGui::BeginDisabled();
            if (ImGui::MenuItem("Insert Atom at Midpoint..."))
            {
                pendingPeriodicAction = PeriodicAction::InsertMidpoint;
                doOpenPeriodicTable = true;
            }
            if (!canInsertAtMidpoint)
                ImGui::EndDisabled();

            if (ImGui::MenuItem("Delete"))
                doDeleteSelected = true;
            if (ImGui::MenuItem("Deselect"))
            {
                for (int idx : selectedInstanceIndices)
                    sceneBuffers.restoreAtomColor(idx);
                selectedInstanceIndices.clear();
            }
            ImGui::EndPopup();
        }
        // Open periodic table at top-level (outside any popup scope)
        if (doOpenPeriodicTable)
            openPeriodicTable();

        // Periodic table picker + apply selected action
        {
            std::vector<ElementSelection> selections;
            if (drawPeriodicTable(selections))
            {
                if (!selections.empty())
                {
                    const auto& sel = selections[0];

                    if (pendingPeriodicAction == PeriodicAction::Substitute && !selectedInstanceIndices.empty())
                    {
                        // Apply selected element to all selected atoms
                        for (int selectedIdx : selectedInstanceIndices)
                        {
                            if (selectedIdx >= 0 && selectedIdx < (int)sceneBuffers.atomIndices.size())
                            {
                                int baseIdx = sceneBuffers.atomIndices[selectedIdx];
                                if (baseIdx >= 0 && baseIdx < (int)structure.atoms.size())
                                {
                                    structure.atoms[baseIdx].symbol      = sel.symbol;
                                    structure.atoms[baseIdx].atomicNumber = sel.atomicNumber;
                                    if (sel.atomicNumber >= 0 && sel.atomicNumber < (int)elementColors.size())
                                    {
                                        structure.atoms[baseIdx].r = elementColors[sel.atomicNumber].r;
                                        structure.atoms[baseIdx].g = elementColors[sel.atomicNumber].g;
                                        structure.atoms[baseIdx].b = elementColors[sel.atomicNumber].b;
                                    }
                                }
                            }
                        }
                        updateBuffers(structure);
                    }

                    if (pendingPeriodicAction == PeriodicAction::InsertMidpoint && selectedInstanceIndices.size() >= 2)
                    {
                        // Insert one new atom at the centroid of all selected atoms.
                        double sumX = 0.0;
                        double sumY = 0.0;
                        double sumZ = 0.0;
                        int validCount = 0;

                        for (int selectedIdx : selectedInstanceIndices)
                        {
                            if (selectedIdx >= 0 && selectedIdx < (int)sceneBuffers.atomIndices.size())
                            {
                                int baseIdx = sceneBuffers.atomIndices[selectedIdx];
                                if (baseIdx >= 0 && baseIdx < (int)structure.atoms.size())
                                {
                                    sumX += structure.atoms[baseIdx].x;
                                    sumY += structure.atoms[baseIdx].y;
                                    sumZ += structure.atoms[baseIdx].z;
                                    ++validCount;
                                }
                            }
                        }

                        if (validCount >= 2)
                        {
                            AtomSite newAtom;
                            newAtom.symbol = sel.symbol;
                            newAtom.atomicNumber = sel.atomicNumber;
                            newAtom.x = sumX / (double)validCount;
                            newAtom.y = sumY / (double)validCount;
                            newAtom.z = sumZ / (double)validCount;
                            if (sel.atomicNumber >= 0 && sel.atomicNumber < (int)elementColors.size())
                            {
                                newAtom.r = elementColors[sel.atomicNumber].r;
                                newAtom.g = elementColors[sel.atomicNumber].g;
                                newAtom.b = elementColors[sel.atomicNumber].b;
                            }
                            else
                            {
                                getDefaultElementColor(sel.atomicNumber, newAtom.r, newAtom.g, newAtom.b);
                            }
                            structure.atoms.push_back(newAtom);
                            updateBuffers(structure);
                        }
                    }
                }

                pendingPeriodicAction = PeriodicAction::None;
            }
        }

        if (pendingPeriodicAction != PeriodicAction::None &&
            !ImGui::IsPopupOpen("Periodic Table##picker"))
        {
            pendingPeriodicAction = PeriodicAction::None;
        }

        // Handle deletion of selected atoms
        if (doDeleteSelected && !selectedInstanceIndices.empty())
        {
            // Collect base indices and sort in reverse order for safe deletion
            std::vector<int> baseIndicesToDelete;
            for (int selectedIdx : selectedInstanceIndices)
            {
                if (selectedIdx >= 0 && selectedIdx < (int)sceneBuffers.atomIndices.size())
                {
                    int baseIdx = sceneBuffers.atomIndices[selectedIdx];
                    if (baseIdx >= 0 && baseIdx < (int)structure.atoms.size())
                    {
                        baseIndicesToDelete.push_back(baseIdx);
                    }
                }
            }
            
            // Sort in reverse order to delete from end to start (avoid index shifting)
            std::sort(baseIndicesToDelete.begin(), baseIndicesToDelete.end(), std::greater<int>());
            
            // Remove duplicates
            baseIndicesToDelete.erase(
                std::unique(baseIndicesToDelete.begin(), baseIndicesToDelete.end()),
                baseIndicesToDelete.end()
            );

            // Delete atoms in reverse order
            for (int baseIdx : baseIndicesToDelete)
            {
                if (baseIdx >= 0 && baseIdx < (int)structure.atoms.size())
                {
                    structure.atoms.erase(structure.atoms.begin() + baseIdx);
                }
            }

            // Rebuild the scene with updated atom list
            updateBuffers(structure);
        }

        ImGui::Render();

        // Apply / maintain highlight for all selected atoms
        for (int& idx : selectedInstanceIndices)
        {
            if (idx >= (int)sceneBuffers.atomCount)
            {
                sceneBuffers.restoreAtomColor(idx);
                idx = -1;
            }
            else if (idx >= 0)
            {
                sceneBuffers.highlightAtom(idx, glm::vec3(1.0f, 1.0f, 0.0f));
            }
        }
        // Remove -1 entries (failed atoms)
        selectedInstanceIndices.erase(
            std::remove(selectedInstanceIndices.begin(), selectedInstanceIndices.end(), -1),
            selectedInstanceIndices.end()
        );

        // ------------------------------------------------------------
        // Draw
        // ------------------------------------------------------------
        renderer.drawShadowPass(shadow, sphere, lightMVP, sceneBuffers.atomCount);

        glViewport(0, 0, w, h);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.drawAtoms(projection, view, lightMVP,
                           shadow, sphere, sceneBuffers.atomCount);

        renderer.drawBoxLines(projection, view,
                              sceneBuffers.lineVAO,
                              sceneBuffers.boxLines.size());

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // ----------------------------------------------------------------
    // Cleanup
    // ----------------------------------------------------------------

    shutdownImGui();
    glfwTerminate();
}

