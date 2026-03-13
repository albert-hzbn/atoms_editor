#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Camera.h"
#include "SphereMesh.h"
#include "Shader.h"
#include "ShadowMap.h"
#include "StructureLoader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <iostream>

int main()
{
    if(!glfwInit())
        return -1;

    GLFWwindow* window =
        glfwCreateWindow(1280,800,"Atoms Editor",nullptr,nullptr);

    if(!window)
        return -1;

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;

    if(glewInit()!=GLEW_OK)
        return -1;

    glEnable(GL_DEPTH_TEST);

    // ------------------------------------------------
    // Camera
    // ------------------------------------------------

    Camera camera;
    Camera::instance=&camera;

    glfwSetMouseButtonCallback(window,Camera::mouseButton);
    glfwSetCursorPosCallback(window,Camera::cursor);
    glfwSetScrollCallback(window,Camera::scroll);

    // ------------------------------------------------
    // ImGui
    // ------------------------------------------------

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(window,true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // ------------------------------------------------
    // Geometry
    // ------------------------------------------------

    SphereMesh sphere(40,40);

    // ------------------------------------------------
    // Load structure
    // ------------------------------------------------

    std::string filename="../data/Cu6Sn5.cif";

    Structure structure = loadStructure(filename);

    bool showAbout = false;
    bool openStructurePopup = false;

    std::string openDir = ".";
    auto pos = filename.find_last_of("/\\");
    if (pos != std::string::npos)
        openDir = filename.substr(0, pos);

    std::vector<std::string> dirHistory;
    int historyIndex = -1;

    auto pushHistory = [&](const std::string& dir) {
        if (historyIndex + 1 < (int)dirHistory.size())
            dirHistory.erase(dirHistory.begin() + historyIndex + 1, dirHistory.end());
        dirHistory.push_back(dir);
        historyIndex = (int)dirHistory.size() - 1;
    };

    // Start history with initial folder
    pushHistory(openDir);

    // Drive roots (Unix style) - include home + common mount points
    std::vector<std::string> driveRoots;
    driveRoots.push_back("/");
    if (const char* home = std::getenv("HOME"))
        driveRoots.push_back(home);
    driveRoots.push_back("/mnt");
    driveRoots.push_back("/media");

    // File browser filters
    const std::vector<std::string> allowedExtensions = {".cif", ".mol", ".pdb", ".xyz", ".sdf"};
    auto toLower = [&](const std::string& s) {
        std::string out = s;
        for (auto& c : out)
            c = (char)tolower(c);
        return out;
    };

    char openFilename[1024] = "";
    std::string initialOpenFilename = filename;
    if (pos != std::string::npos)
        initialOpenFilename = filename.substr(pos + 1);

    strncpy(openFilename, initialOpenFilename.c_str(), sizeof(openFilename));
    openFilename[sizeof(openFilename) - 1] = '\0';

    // ------------------------------------------------
    // Instance buffers
    // ------------------------------------------------

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
    size_t atomCount = 0;

    GLuint instanceVBO, colorVBO;

    glBindVertexArray(sphere.vao);

    glGenBuffers(1, &instanceVBO);
    glGenBuffers(1, &colorVBO);

    auto updateBuffers = [&](const Structure& s) {
        positions.clear();
        colors.clear();

        for (const auto& atom : s.atoms)
        {
            positions.emplace_back(atom.x, atom.y, atom.z);
            colors.emplace_back(atom.r, atom.g, atom.b);
        }

        atomCount = s.atoms.size();

        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     positions.size() * sizeof(glm::vec3),
                     positions.data(),
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glVertexAttribDivisor(1, 1);

        glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     colors.size() * sizeof(glm::vec3),
                     colors.data(),
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glVertexAttribDivisor(2, 1);
    };

    updateBuffers(structure);

    // ------------------------------------------------
    // Shader
    // ------------------------------------------------

    const char* vs = R"(

    #version 130

    in vec3 position;
    in vec3 instancePos;
    in vec3 instanceColor;

    uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 lightMVP;

    out vec3 fragColor;
    out vec4 FragPosLight;

    void main()
    {
        vec3 worldPos = position + instancePos;

        gl_Position = projection * view * vec4(worldPos,1.0);

        FragPosLight = lightMVP * vec4(worldPos,1.0);

        fragColor = instanceColor;
    }

    )";

    const char* fs = R"(

    #version 130

    in vec3 fragColor;
    in vec4 FragPosLight;

    uniform sampler2D shadowMap;

    out vec4 color;

    float computeShadow(vec4 pos)
    {
        vec3 proj = pos.xyz / pos.w;
        proj = proj * 0.5 + 0.5;

        float closest = texture(shadowMap,proj.xy).r;
        float current = proj.z;

        float bias = 0.003;

        if(current - bias > closest)
            return 1.0;

        return 0.0;
    }

    void main()
    {
        float shadow = computeShadow(FragPosLight);

        vec3 lighting = (1.0-shadow) * fragColor;

        color = vec4(lighting,1.0);
    }

    )";

    GLuint program = createProgram(vs,fs);

    // ------------------------------------------------
    // Shadow shader
    // ------------------------------------------------

    const char* shadowVS = R"(

    #version 130
    in vec3 position;

    uniform mat4 lightMVP;

    void main()
    {
        gl_Position = lightMVP * vec4(position,1.0);
    }

    )";

    const char* shadowFS = R"(

    #version 130
    void main(){}

    )";

    GLuint shadowProgram = createProgram(shadowVS,shadowFS);

    // ------------------------------------------------
    // Shadow map
    // ------------------------------------------------

    ShadowMap shadow = createShadowMap(1024,1024);

    // ------------------------------------------------
    // Render loop
    // ------------------------------------------------

    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ----------------------
        // Main menu bar
        // ----------------------
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open..."))
                    openStructurePopup = true;

                if (ImGui::MenuItem("Quit"))
                    glfwSetWindowShouldClose(window, true);

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

                        if (!isDir)
                        {
                            std::string ext;
                            auto dot = name.find_last_of('.');
                            if (dot != std::string::npos)
                                ext = toLower(name.substr(dot));

                            bool keep = false;
                            for (const auto& allowed : allowedExtensions)
                            {
                                if (ext == allowed)
                                {
                                    keep = true;
                                    break;
                                }
                            }
                            if (!keep)
                                continue;
                        }

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

        ImGui::Render();

        int w,h;
        glfwGetFramebufferSize(window,&w,&h);

        // --------------------------------------------
        // Light matrices
        // --------------------------------------------

        glm::mat4 lightProj =
            glm::perspective(glm::radians(45.0f),1.0f,0.1f,100.0f);

        glm::mat4 lightView =
            glm::lookAt(glm::vec3(10,10,10),
                        glm::vec3(0,0,0),
                        glm::vec3(0,1,0));

        glm::mat4 lightMVP = lightProj * lightView;

        // --------------------------------------------
        // Shadow pass
        // --------------------------------------------

        beginShadowPass(shadow);

        glUseProgram(shadowProgram);

        GLuint lightLoc =
            glGetUniformLocation(shadowProgram,"lightMVP");

        glUniformMatrix4fv(lightLoc,1,GL_FALSE,
                           glm::value_ptr(lightMVP));

        glBindVertexArray(sphere.vao);

        glDrawArraysInstanced(
            GL_TRIANGLES,
            0,
            sphere.vertexCount,
            structure.atoms.size()
        );

        endShadowPass();

        // --------------------------------------------
        // Normal pass
        // --------------------------------------------

        glViewport(0,0,w,h);

        glClearColor(0.2f,0.2f,0.2f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection =
            glm::perspective(glm::radians(45.0f),
                             (float)w/h,
                             0.1f,
                             1000.0f);

        float yaw   = glm::radians(camera.yaw);
        float pitch = glm::radians(camera.pitch);

        glm::vec3 camPos(
            camera.distance*cos(pitch)*sin(yaw),
            camera.distance*sin(pitch),
            camera.distance*cos(pitch)*cos(yaw)
        );

        glm::mat4 view =
            glm::lookAt(camPos,
                        glm::vec3(0,0,0),
                        glm::vec3(0,1,0));

        glUseProgram(program);

        GLuint projLoc = glGetUniformLocation(program,"projection");
        GLuint viewLoc = glGetUniformLocation(program,"view");
        GLuint lightLoc2 = glGetUniformLocation(program,"lightMVP");

        glUniformMatrix4fv(projLoc,1,GL_FALSE,
                           glm::value_ptr(projection));

        glUniformMatrix4fv(viewLoc,1,GL_FALSE,
                           glm::value_ptr(view));

        glUniformMatrix4fv(lightLoc2,1,GL_FALSE,
                           glm::value_ptr(lightMVP));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,shadow.depthTexture);

        GLuint shadowLoc =
            glGetUniformLocation(program,"shadowMap");

        glUniform1i(shadowLoc,0);

        glBindVertexArray(sphere.vao);

        glDrawArraysInstanced(
            GL_TRIANGLES,
            0,
            sphere.vertexCount,
            structure.atoms.size()
        );

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    glfwTerminate();
}