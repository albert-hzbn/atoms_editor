#include "ui/NanoCrystalBuilderDialog.h"
#include "algorithms/NanoCrystalBuilder.h"
#include "util/PathUtils.h"

#include "ElementData.h"
#include "graphics/CylinderMesh.h"
#include "graphics/Renderer.h"
#include "graphics/SphereMesh.h"
#include "graphics/StructureInstanceBuilder.h"
#include "graphics/ShadowMap.h"
#include "io/StructureLoader.h"
#include "app/SceneView.h"
#include "camera/Camera.h"
#include "imgui.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#else
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace
{

// ---------------------------------------------------------------------------
// File filter and directory loading (nano-specific)
// ---------------------------------------------------------------------------

bool nanoIsSupportedFile(const std::string& name)
{
    static const char* exts[] = {
        ".cif",".mol",".pdb",".xyz",".sdf",".vasp",".mol2",".pwi",".gjf",
        nullptr
    };
    std::string low = name;
    for (char& c : low) c = (char)std::tolower((unsigned char)c);
    for (int i = 0; exts[i]; ++i) {
        const std::string ext(exts[i]);
        if (low.size() >= ext.size() &&
            low.compare(low.size() - ext.size(), ext.size(), ext) == 0)
            return true;
    }
    return false;
}

bool parseObjMesh_unused(const std::string& path,
                  std::vector<glm::vec3>& outVertices,
                  std::vector<unsigned int>& outIndices,
                  std::string& error)
{
    std::ifstream in(path.c_str());
    if (!in)
    {
        error = "Cannot open OBJ file.";
        return false;
    }

    outVertices.clear();
    outIndices.clear();
    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty())
            continue;
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "v")
        {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            if (ls >> x >> y >> z)
                outVertices.push_back(glm::vec3(x, y, z));
        }
        else if (tag == "f")
        {
            std::vector<unsigned int> face;
            std::string token;
            while (ls >> token)
            {
                const size_t slashPos = token.find('/');
                const std::string idStr = (slashPos == std::string::npos) ? token : token.substr(0, slashPos);
                if (idStr.empty())
                    continue;
                const int idx = std::atoi(idStr.c_str());
                if (idx <= 0)
                    continue;
                face.push_back((unsigned int)(idx - 1));
            }

            if (face.size() >= 3)
            {
                for (size_t i = 1; i + 1 < face.size(); ++i)
                {
                    outIndices.push_back(face[0]);
                    outIndices.push_back(face[i]);
                    outIndices.push_back(face[i + 1]);
                }
            }
        }
    }

    if (outVertices.empty() || outIndices.empty())
    {
        error = "OBJ has no usable vertices/faces.";
        return false;
    }

    for (unsigned int idx : outIndices)
    {
        if (idx >= outVertices.size())
        {
            error = "OBJ face index out of range.";
            return false;
        }
    }

    return true;
}

bool parseStlMesh_unused(const std::string& path,
                  std::vector<glm::vec3>& outVertices,
                  std::vector<unsigned int>& outIndices,
                  std::string& error)
{
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in)
    {
        error = "Cannot open STL file.";
        return false;
    }

    in.seekg(0, std::ios::end);
    const std::streamoff fileSize = in.tellg();
    in.seekg(0, std::ios::beg);

    char header[80] = {0};
    in.read(header, 80);
    unsigned int triCount = 0;
    in.read(reinterpret_cast<char*>(&triCount), sizeof(unsigned int));

    const std::streamoff expectedBinarySize = 84 + (std::streamoff)triCount * 50;
    const bool looksBinary = (fileSize == expectedBinarySize);

    outVertices.clear();
    outIndices.clear();

    if (looksBinary)
    {
        outVertices.reserve((size_t)triCount * 3);
        outIndices.reserve((size_t)triCount * 3);
        for (unsigned int t = 0; t < triCount; ++t)
        {
            float data[12] = {0.0f};
            unsigned short attr = 0;
            in.read(reinterpret_cast<char*>(data), sizeof(data));
            in.read(reinterpret_cast<char*>(&attr), sizeof(attr));
            if (!in)
            {
                error = "Binary STL parse failed.";
                return false;
            }

            const unsigned int base = (unsigned int)outVertices.size();
            outVertices.push_back(glm::vec3(data[3], data[4], data[5]));
            outVertices.push_back(glm::vec3(data[6], data[7], data[8]));
            outVertices.push_back(glm::vec3(data[9], data[10], data[11]));
            outIndices.push_back(base + 0);
            outIndices.push_back(base + 1);
            outIndices.push_back(base + 2);
        }
        return !outVertices.empty();
    }

    in.clear();
    in.seekg(0, std::ios::beg);

    std::string line;
    std::vector<glm::vec3> triVerts;
    triVerts.reserve(3);
    while (std::getline(in, line))
    {
        std::istringstream ls(line);
        std::string tag;
        ls >> tag;
        if (tag == "vertex")
        {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            if (ls >> x >> y >> z)
            {
                triVerts.push_back(glm::vec3(x, y, z));
                if (triVerts.size() == 3)
                {
                    const unsigned int base = (unsigned int)outVertices.size();
                    outVertices.push_back(triVerts[0]);
                    outVertices.push_back(triVerts[1]);
                    outVertices.push_back(triVerts[2]);
                    outIndices.push_back(base + 0);
                    outIndices.push_back(base + 1);
                    outIndices.push_back(base + 2);
                    triVerts.clear();
                }
            }
        }
    }

    if (outVertices.empty() || outIndices.empty())
    {
        error = "ASCII STL has no usable triangles.";
        return false;
    }

    return true;
}

void nanoLoadDirEntries(const std::string& dir,
                        std::vector<std::pair<std::string, bool>>& entries)
{
    entries.clear();
#ifdef _WIN32
    std::string pat = dir;
    if (!pat.empty() && pat.back() != '/' && pat.back() != '\\')
        pat.push_back('\\');
    pat.push_back('*');
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pat.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        std::string nm(fd.cFileName);
        if (nm == "." || nm == "..") continue;
        bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (!isDir && !nanoIsSupportedFile(nm)) continue;
        entries.emplace_back(nm, isDir);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR* d = opendir(dir.empty() ? "." : dir.c_str());
    if (!d) return;
    struct dirent* de;
    while ((de = readdir(d)) != nullptr) {
        std::string nm(de->d_name);
        if (nm == "." || nm == "..") continue;
        std::string full = joinPath(dir, nm);
        struct stat st;
        bool isDir = (stat(full.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
        if (!isDir && !nanoIsSupportedFile(nm)) continue;
        entries.emplace_back(nm, isDir);
    }
    closedir(d);
#endif
    std::sort(entries.begin(), entries.end(),
              [](const std::pair<std::string,bool>& a,
                 const std::pair<std::string,bool>& b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });
}

// ---------------------------------------------------------------------------
// Local helper
// ---------------------------------------------------------------------------

static float safeLen3(const glm::vec3& v)
{
    return std::sqrt(glm::dot(v, v));
}

// ---------------------------------------------------------------------------
// Shape parameter UI helpers
void drawShapeParameters(NanoParams& params)
{
    switch (params.shape) {
        case NanoShape::Sphere:
            ImGui::SetNextItemWidth(180.0f);
            ImGui::DragFloat("Radius (A)##sph", &params.sphereRadius, 0.5f, 0.1f, 500.0f, "%.2f");
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Drag to adjust, Ctrl+click to type");
            if (params.sphereRadius < 0.1f) params.sphereRadius = 0.1f;
            break;

        case NanoShape::Ellipsoid:
            ImGui::SetNextItemWidth(180.0f);
            ImGui::DragFloat("Semi-axis a (A)##ell", &params.ellipRx, 0.5f, 0.1f, 500.0f, "%.2f");
            ImGui::SetNextItemWidth(180.0f);
            ImGui::DragFloat("Semi-axis b (A)##ell", &params.ellipRy, 0.5f, 0.1f, 500.0f, "%.2f");
            ImGui::SetNextItemWidth(180.0f);
            ImGui::DragFloat("Semi-axis c (A)##ell", &params.ellipRz, 0.5f, 0.1f, 500.0f, "%.2f");
            if (params.ellipRx < 0.1f) params.ellipRx = 0.1f;
            if (params.ellipRy < 0.1f) params.ellipRy = 0.1f;
            if (params.ellipRz < 0.1f) params.ellipRz = 0.1f;
            break;

        case NanoShape::Box:
            ImGui::SetNextItemWidth(180.0f);
            ImGui::DragFloat("Half-size X (A)##box", &params.boxHx, 0.5f, 0.1f, 500.0f, "%.2f");
            ImGui::SetNextItemWidth(180.0f);
            ImGui::DragFloat("Half-size Y (A)##box", &params.boxHy, 0.5f, 0.1f, 500.0f, "%.2f");
            ImGui::SetNextItemWidth(180.0f);
            ImGui::DragFloat("Half-size Z (A)##box", &params.boxHz, 0.5f, 0.1f, 500.0f, "%.2f");
            ImGui::TextDisabled("Full box: %.2f x %.2f x %.2f A",
                                2.f*params.boxHx, 2.f*params.boxHy, 2.f*params.boxHz);
            if (params.boxHx < 0.1f) params.boxHx = 0.1f;
            if (params.boxHy < 0.1f) params.boxHy = 0.1f;
            if (params.boxHz < 0.1f) params.boxHz = 0.1f;
            break;

        case NanoShape::Cylinder: {
            const char* axisNames[] = {"X", "Y", "Z"};
            ImGui::SetNextItemWidth(180.0f);
            ImGui::DragFloat("Radius (A)##cyl", &params.cylRadius, 0.5f, 0.1f, 500.0f, "%.2f");
            ImGui::SetNextItemWidth(180.0f);
            ImGui::DragFloat("Height (A)##cyl", &params.cylHeight, 0.5f, 0.1f, 1000.0f, "%.2f");
            ImGui::SetNextItemWidth(120.0f);
            ImGui::Combo("Axis##cyl", &params.cylAxis, axisNames, 3);
            if (params.cylRadius < 0.1f) params.cylRadius = 0.1f;
            if (params.cylHeight < 0.1f) params.cylHeight = 0.1f;
            break;
        }

        case NanoShape::Octahedron:
            ImGui::SetNextItemWidth(180.0f);
            ImGui::DragFloat("Radius (A)##oct", &params.octRadius, 0.5f, 0.1f, 500.0f, "%.2f");
            ImGui::TextDisabled("Condition: |x|+|y|+|z| <= R");
            if (params.octRadius < 0.1f) params.octRadius = 0.1f;
            break;

        case NanoShape::TruncatedOctahedron:
            ImGui::SetNextItemWidth(180.0f);
            ImGui::DragFloat("Octahedron R (A)##trunc", &params.truncOctRadius, 0.5f, 0.1f, 500.0f, "%.2f");
            ImGui::SetNextItemWidth(180.0f);
            ImGui::DragFloat("Truncation R (A)##trunc", &params.truncOctTrunc,  0.5f, 0.1f, 500.0f, "%.2f");
            ImGui::TextDisabled("Cond: |x|+|y|+|z|<=R_oct  AND  max(|x|,|y|,|z|)<=R_trunc");
            if (params.truncOctRadius < 0.1f) params.truncOctRadius = 0.1f;
            if (params.truncOctTrunc  < 0.1f) params.truncOctTrunc  = 0.1f;
            break;

        case NanoShape::Cuboctahedron:
            ImGui::SetNextItemWidth(180.0f);
            ImGui::DragFloat("Radius (A)##cubo", &params.cuboRadius, 0.5f, 0.1f, 500.0f, "%.2f");
            ImGui::TextDisabled("Cond: |x|+|y|, |y|+|z|, |x|+|z| <= R");
            if (params.cuboRadius < 0.1f) params.cuboRadius = 0.1f;
            break;

        default:
            break;
    }
}

void drawReplicationPreview(const NanoParams& params, const Structure& reference)
{
    if (!reference.hasUnitCell) return;
    const auto& cv = reference.cellVectors;
    const glm::vec3 a((float)cv[0][0],(float)cv[0][1],(float)cv[0][2]);
    const glm::vec3 b((float)cv[1][0],(float)cv[1][1],(float)cv[1][2]);
    const glm::vec3 c((float)cv[2][0],(float)cv[2][1],(float)cv[2][2]);
    const float la = safeLen3(a), lb = safeLen3(b), lc = safeLen3(c);
    if (la < 1e-8f || lb < 1e-8f || lc < 1e-8f) return;

    const float maxR = computeBoundingRadius(params);
    const int kMax = 40;
    bool clamped = false;
    auto rep = [&](float L) -> int {
        int n = (int)std::ceil(maxR / L) + 2;
        if (n > kMax) { clamped = true; n = kMax; }
        return n;
    };
    int nA, nB, nC;
    if (params.autoReplicate) {
        nA = rep(la); nB = rep(lb); nC = rep(lc);
    } else {
        nA = std::max(1, params.repA);
        nB = std::max(1, params.repB);
        nC = std::max(1, params.repC);
    }
    const long long tested =
        (long long)(2*nA+1)*(long long)(2*nB+1)*(long long)(2*nC+1)
        * (long long)reference.atoms.size();
    ImGui::TextDisabled("Tiling: %dx%dx%d  (%lld atoms tested)",
                        2*nA+1, 2*nB+1, 2*nC+1, (long long)tested);
    if (clamped)
        ImGui::TextColored(ImVec4(1,0.7f,0,1), "Warning: replication clamped to %d.", kMax);
    if (tested > 8000000LL)
        ImGui::TextColored(ImVec4(1,0.3f,0.3f,1),
                           "Exceeds 8M limit -- build will be refused.");
}

void drawNanoBuildResult(const NanoBuildResult& result)
{
    if (result.message.empty()) return;
    ImGui::TextWrapped("Status: %s", result.message.c_str());
    if (!result.success) return;
    ImGui::Separator();
    ImGui::Text("Shape:          %s", shapeLabel(result.shape));
    ImGui::Text("Input atoms:    %d", result.inputAtoms);
    ImGui::Text("Output atoms:   %d", result.outputAtoms);
    ImGui::Text("Est. diameter:  %.2f A", result.estimatedDiameter);
    if (result.tilingUsed)
        ImGui::Text("Tiling: %dx%dx%d (%s)",
                    2*result.repA+1, 2*result.repB+1, 2*result.repC+1,
                    result.repClamped ? "clamped" : "auto");
    else
        ImGui::Text("Tiling: none (no unit cell in reference)");
}

} // anonymous namespace

// ===========================================================================
// NanoCrystalBuilderDialog – public / private implementation
// ===========================================================================

NanoCrystalBuilderDialog::NanoCrystalBuilderDialog()
    : m_browsEntryDirty(true)
    , m_browsShowPanel(false)
    , m_glReady(false)
    , m_previewBufDirty(true)
    , m_camYaw(45.0f)
    , m_camPitch(35.0f)
    , m_camDistance(10.0f)
{
    m_browsFilename[0] = '\0';
    m_browsStatusMsg[0] = '\0';
    m_browsDir = detectHomePath();
}

NanoCrystalBuilderDialog::~NanoCrystalBuilderDialog()
{
    if (m_previewFBO)      { glDeleteFramebuffers(1,  &m_previewFBO);    m_previewFBO = 0; }
    if (m_previewColorTex) { glDeleteTextures(1,      &m_previewColorTex); m_previewColorTex = 0; }
    if (m_previewDepthRbo) { glDeleteRenderbuffers(1, &m_previewDepthRbo); m_previewDepthRbo = 0; }

    if (m_previewShadow.depthFBO)
        glDeleteFramebuffers(1, &m_previewShadow.depthFBO);
    if (m_previewShadow.depthTexture)
        glDeleteTextures(1, &m_previewShadow.depthTexture);

    delete m_previewSphere;
    delete m_previewCylinder;
}

void NanoCrystalBuilderDialog::initRenderResources(Renderer& renderer)
{
    m_renderer = &renderer;

    // Independent mesh objects so VAO state does not conflict with the main
    // SceneBuffers.
    m_previewSphere   = new SphereMesh(24, 24);
    m_previewCylinder = new CylinderMesh(16);
    m_previewBuffers.init(m_previewSphere->vao, m_previewCylinder->vao);

    // A 1×1 shadow map satisfies the sampler binding without drawing shadows.
    m_previewShadow = createShadowMap(1, 1);

    m_glReady = true;
}

void NanoCrystalBuilderDialog::feedDroppedFile(const std::string& path)
{
    m_pendingDropPath = path;
}

void NanoCrystalBuilderDialog::refreshBrowserEntries()
{
    nanoLoadDirEntries(m_browsDir, m_browsEntries);
    m_browsEntryDirty = false;
}

bool NanoCrystalBuilderDialog::tryLoadFile(const std::string& path,
                                            const std::vector<float>& radii,
                                            const std::vector<float>& shininess)
{
    Structure loaded;
    std::string err;
    if (!loadStructureFromFile(path, loaded, err)) {
        std::snprintf(m_browsStatusMsg, sizeof(m_browsStatusMsg),
                      "Load failed: %s", err.c_str());
        return false;
    }
    m_reference = std::move(loaded);
    std::snprintf(m_browsStatusMsg, sizeof(m_browsStatusMsg),
                  "Loaded: %d atoms", (int)m_reference.atoms.size());
    m_previewBufDirty = true;
    if (m_glReady) {
        rebuildPreviewBuffers(radii, shininess);
        autoFitPreviewCamera();
    }
    std::cout << "[NanoCrystalBuilder] Reference loaded: " << path
              << " (" << m_reference.atoms.size() << " atoms)" << std::endl;
    return true;
}

void NanoCrystalBuilderDialog::ensurePreviewFBO(int w, int h)
{
    if (w == m_previewW && h == m_previewH && m_previewFBO != 0)
        return;

    if (m_previewFBO)      { glDeleteFramebuffers(1,  &m_previewFBO);    m_previewFBO = 0; }
    if (m_previewColorTex) { glDeleteTextures(1,      &m_previewColorTex); m_previewColorTex = 0; }
    if (m_previewDepthRbo) { glDeleteRenderbuffers(1, &m_previewDepthRbo); m_previewDepthRbo = 0; }

    glGenTextures(1, &m_previewColorTex);
    glBindTexture(GL_TEXTURE_2D, m_previewColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &m_previewDepthRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_previewDepthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &m_previewFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_previewFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_previewColorTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, m_previewDepthRbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_previewW = w;
    m_previewH = h;
}

void NanoCrystalBuilderDialog::rebuildPreviewBuffers(const std::vector<float>& radii,
                                                      const std::vector<float>& shininess)
{
    if (!m_glReady || m_reference.atoms.empty())
        return;

    static const int kIdent[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
    StructureInstanceData data = buildStructureInstanceData(
        m_reference, false, kIdent, radii, shininess);

    std::array<bool,119> noFilter = {};
    m_previewBuffers.upload(data, false, noFilter);
    m_previewBufDirty = false;
}

void NanoCrystalBuilderDialog::autoFitPreviewCamera()
{
    m_camYaw   = 45.0f;
    m_camPitch = 35.0f;

    if (m_previewBuffers.atomCount == 0) {
        m_camDistance = 10.0f;
        return;
    }

    float maxR = 0.0f;
    for (size_t i = 0; i < m_previewBuffers.atomPositions.size(); ++i) {
        float r = (i < m_previewBuffers.atomRadii.size())
                  ? m_previewBuffers.atomRadii[i] : 0.0f;
        float d = glm::length(m_previewBuffers.atomPositions[i]
                              - m_previewBuffers.orbitCenter) + r;
        maxR = std::max(maxR, d);
    }
    maxR = std::max(maxR, 1.0f);

    const float halfFov = glm::radians(22.5f); // half of 45 deg fov
    float dist = maxR / std::sin(halfFov) * 1.15f;
    dist = std::max(Camera::kMinDistance, std::min(Camera::kMaxDistance, dist));
    m_camDistance = dist;
}

void NanoCrystalBuilderDialog::renderPreviewToFBO(int w, int h)
{
    if (!m_glReady || !m_renderer || m_previewBuffers.atomCount == 0)
        return;

    ensurePreviewFBO(w, h);

    // Save current GL state
    GLint prevFbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
    GLint prevVP[4];
    glGetIntegerv(GL_VIEWPORT, prevVP);

    // Build frame matrices using a temporary Camera
    Camera cam;
    cam.yaw      = m_camYaw;
    cam.pitch    = m_camPitch;
    cam.distance = m_camDistance;

    FrameView frame;
    frame.framebufferWidth  = w;
    frame.framebufferHeight = h;
    buildFrameView(cam, m_previewBuffers, true, frame);

    // Draw into preview FBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_previewFBO);
    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.09f, 0.11f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_renderer->drawBonds(frame.projection, frame.view,
                          frame.lightPosition, frame.cameraPosition,
                          *m_previewCylinder, m_previewBuffers.bondCount);

    m_renderer->drawAtoms(frame.projection, frame.view,
                          frame.lightMVP, frame.lightPosition, frame.cameraPosition,
                          m_previewShadow, *m_previewSphere,
                          m_previewBuffers.atomCount);

    m_renderer->drawBoxLines(frame.projection, frame.view,
                             m_previewBuffers.lineVAO,
                             m_previewBuffers.boxLines.size());

    // Restore GL state
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prevFbo);
    glViewport(prevVP[0], prevVP[1], prevVP[2], prevVP[3]);
}

// ---------------------------------------------------------------------------
// drawMenuItem
// ---------------------------------------------------------------------------

void NanoCrystalBuilderDialog::drawMenuItem(bool enabled)
{
    if (ImGui::MenuItem("Nanocrystal", NULL, false, enabled))
        m_openRequested = true;
}

// ---------------------------------------------------------------------------
// drawDialog  – the main ImGui popup
// ---------------------------------------------------------------------------

void NanoCrystalBuilderDialog::drawDialog(
    Structure& structure,
    const std::vector<glm::vec3>& elementColors,
    const std::vector<float>& elementRadii,
    const std::vector<float>& elementShininess,
    const std::function<void(Structure&)>& updateBuffers)
{
    static NanoParams      params;
    static NanoBuildResult lastResult;

    // Consume any pending file drop
    if (!m_pendingDropPath.empty()) {
        tryLoadFile(m_pendingDropPath, elementRadii, elementShininess);
        m_pendingDropPath.clear();
    }

    if (m_openRequested) {
        ImGui::OpenPopup("Build Nanocrystal");
        lastResult      = {};
        if (m_reference.atoms.empty() && !structure.atoms.empty()) {
            m_reference = structure;
            m_previewBufDirty = true;
        }
        m_openRequested = false;
    }

    m_isOpen = ImGui::IsPopupOpen("Build Nanocrystal");

    ImGui::SetNextWindowSize(ImVec2(950.0f, 840.0f), ImGuiCond_FirstUseEver);
    bool dialogOpen = true;
    if (!ImGui::BeginPopupModal("Build Nanocrystal", &dialogOpen, 0)) {
        m_isOpen = false;
        return;
    }
    m_isOpen = true;

    ImGui::TextDisabled("Load a reference crystal, choose a shape, then click Build."
        "  Drag-and-drop .cif/.xyz/.pdb/.vasp/etc. onto the preview panel.");
    ImGui::Separator();

    // =========================================================================
    // Layout: Left = Structure view | Right = Builder options
    // =========================================================================

    constexpr float kPrevW   = 420.0f;
    constexpr float kPrevH   = 360.0f;
    constexpr float kSideH   = 500.0f;

    // ---- LEFT: Structure view with drag-and-drop ----
    ImGui::BeginChild("##nanoStructureView", ImVec2(kPrevW, kSideH), true);

    ImGui::Text("Reference Structure");
    ImGui::Separator();

    // Show the actual load status message (set by tryLoadFile)
    if (m_browsStatusMsg[0] != '\0')
        ImGui::TextWrapped("%s", m_browsStatusMsg);
    else
        ImGui::TextDisabled("No reference loaded");

    ImGui::Spacing();
    if (ImGui::Button("Load File...##nanoLoadBtn", ImVec2(120.0f, 0.0f)))
        m_browsShowPanel = !m_browsShowPanel;
    if (!m_reference.atoms.empty()) {
        ImGui::SameLine();
        if (ImGui::Button("Clear##nanoClearRef", ImVec2(70.0f, 0.0f))) {
            m_reference       = {};
            m_previewBufDirty = true;
            m_browsStatusMsg[0] = '\0';
        }
    }
    ImGui::Spacing();

    // Drag-and-drop target using InvisibleButton
    const float dropH = kPrevH - ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y + 6.0f;
    ImGui::InvisibleButton("##nanoRefDropZone", ImVec2(-1.0f, dropH > 60.0f ? dropH : 60.0f));

    // Draw drop zone border
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 dropMin = ImGui::GetItemRectMin();
    ImVec2 dropMax = ImGui::GetItemRectMax();
    const bool dropZoneHovered = ImGui::IsItemHovered();
    const bool dropZoneActive = ImGui::IsItemActive();
    dl->AddRect(dropMin, dropMax, ImGui::GetColorU32(ImGuiCol_Border), 2.0f);

    if (m_reference.atoms.empty()) {
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload("DROP_FILE", ImGuiDragDropFlags_AcceptPeekOnly)) {
                const char* files = (const char*)payload->Data;
                tryLoadFile(files, elementRadii, elementShininess);
            }
            ImGui::EndDragDropTarget();
        }

        // Show centered drop hint (two lines)
        const char* hint1 = "Drop a structure file here";
        const char* hint2 = "(.cif .xyz .pdb .sdf .mol .vasp ...)"; 
        const float lh = ImGui::GetTextLineHeight();
        const float w1 = ImGui::CalcTextSize(hint1).x;
        const float w2 = ImGui::CalcTextSize(hint2).x;
        const ImVec2 mid((dropMin.x + dropMax.x) * 0.5f, (dropMin.y + dropMax.y) * 0.5f);
        const ImU32 hintCol = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        dl->AddText(ImVec2(mid.x - w1 * 0.5f, mid.y - lh * 1.1f), hintCol, hint1);
        dl->AddText(ImVec2(mid.x - w2 * 0.5f, mid.y + lh * 0.1f), hintCol, hint2);
    } else {
        // Show 3D preview
        if (m_glReady) {
            if (m_previewBufDirty)
                rebuildPreviewBuffers(elementRadii, elementShininess);

            const float previewPadding = 5.0f;
            const ImVec2 previewSize(dropMax.x - dropMin.x - 2.0f * previewPadding,
                                     dropMax.y - dropMin.y - 2.0f * previewPadding);
            const int previewW = std::max(1, (int)previewSize.x);
            const int previewH = std::max(1, (int)previewSize.y);

            renderPreviewToFBO(previewW, previewH);

            const ImVec2 previewMin(dropMin.x + previewPadding, dropMin.y + previewPadding);
            const ImVec2 previewMax(previewMin.x + previewSize.x, previewMin.y + previewSize.y);

            dl->AddImage((ImTextureID)(intptr_t)m_previewColorTex,
                         previewMin,
                         previewMax,
                         ImVec2(0, 1), ImVec2(1, 0));

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload =
                        ImGui::AcceptDragDropPayload("DROP_FILE", ImGuiDragDropFlags_AcceptPeekOnly)) {
                    const char* files = (const char*)payload->Data;
                    tryLoadFile(files, elementRadii, elementShininess);
                }
                ImGui::EndDragDropTarget();
            }

            if (dropZoneActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f)) {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                m_camYaw   -= delta.x * 0.5f;
                m_camPitch += delta.y * 0.5f;
            }

            if (dropZoneHovered) {
                float wheel = ImGui::GetIO().MouseWheel;
                if (wheel != 0.0f) {
                    m_camDistance -= wheel * m_camDistance * 0.1f;
                    m_camDistance  = std::max(Camera::kMinDistance,
                                              std::min(Camera::kMaxDistance, m_camDistance));
                }
            }

            ImGui::TextDisabled("Left-drag = orbit   Scroll = zoom");
        }
    }

    // Reference info bar
    if (!m_reference.atoms.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        if (m_reference.hasUnitCell) {
            const auto& cv = m_reference.cellVectors;
            float la = safeLen3(glm::vec3((float)cv[0][0],(float)cv[0][1],(float)cv[0][2]));
            float lb = safeLen3(glm::vec3((float)cv[1][0],(float)cv[1][1],(float)cv[1][2]));
            float lc = safeLen3(glm::vec3((float)cv[2][0],(float)cv[2][1],(float)cv[2][2]));
            ImGui::TextDisabled("%d atoms  |  a=%.3f  b=%.3f  c=%.3f A",
                                (int)m_reference.atoms.size(), la, lb, lc);
            ImGui::TextDisabled("Periodic crystal -- supercell tiling available");
        } else {
            ImGui::TextDisabled("%d atoms  |  No unit cell -- shape carving only",
                                (int)m_reference.atoms.size());
        }
    }

    ImGui::EndChild(); // ##nanoStructureView

    ImGui::SameLine();

    // ---- RIGHT: Builder options ----
    ImGui::BeginChild("##nanoBuilderOptions", ImVec2(0, kSideH), true);

    ImGui::Text("Builder Options");
    ImGui::Separator();

    ImGui::Text("Shape");
    static const char* kShapeLabels[] = {
        "Sphere","Ellipsoid","Box","Cylinder",
        "Octahedron","Truncated Octahedron","Cuboctahedron"
    };
    constexpr int kNumShapeOptions = 7;
    int shapeInt = (int)params.shape;
    if (shapeInt >= kNumShapeOptions) { shapeInt = 0; params.shape = NanoShape::Sphere; }
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::Combo("##nanoShapeCombo", &shapeInt, kShapeLabels, kNumShapeOptions))
        params.shape = (NanoShape)shapeInt;

    ImGui::Spacing();
    ImGui::Separator();

    // Scrollable parameters area
    ImGui::BeginChild("##nanoParamsScroll", ImVec2(-1, -50), true);
    
    drawShapeParameters(params);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Carving center");
    ImGui::Checkbox("Auto-center from atoms##nano", &params.autoCenterFromAtoms);
    if (!params.autoCenterFromAtoms) {
        ImGui::SetNextItemWidth(130.0f);
        ImGui::DragFloat("X (A)##cnano", &params.cx, 0.1f, -1000.f, 1000.f, "%.3f");
        ImGui::SetNextItemWidth(130.0f);
        ImGui::DragFloat("Y (A)##cnano", &params.cy, 0.1f, -1000.f, 1000.f, "%.3f");
        ImGui::SetNextItemWidth(130.0f);
        ImGui::DragFloat("Z (A)##cnano", &params.cz, 0.1f, -1000.f, 1000.f, "%.3f");
    }

    if (m_reference.hasUnitCell) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Supercell replication");
        ImGui::Checkbox("Auto-replicate##nano", &params.autoReplicate);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Automatically tile the unit cell to fully cover the shape");
        if (!params.autoReplicate) {
            ImGui::SetNextItemWidth(80.0f);
            ImGui::DragInt("Along a##nano", &params.repA, 0.2f, 1, 40);
            ImGui::SetNextItemWidth(80.0f);
            ImGui::DragInt("Along b##nano", &params.repB, 0.2f, 1, 40);
            ImGui::SetNextItemWidth(80.0f);
            ImGui::DragInt("Along c##nano", &params.repC, 0.2f, 1, 40);
            if (params.repA < 1) params.repA = 1;
            if (params.repB < 1) params.repB = 1;
            if (params.repC < 1) params.repC = 1;
        }
        drawReplicationPreview(params, m_reference);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Output cell");
    ImGui::Checkbox("Rectangular cell##nano", &params.setOutputCell);
    if (params.setOutputCell) {
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputFloat("Vacuum (A)##nano",
                          &params.vacuumPadding, 0.f, 0.f, "%.2f");
        if (params.vacuumPadding < 0.f) params.vacuumPadding = 0.f;

        const HalfExtents he = computeShapeHalfExtents(params);
        ImGui::TextDisabled("%.2fx%.2fx%.2f A",
                            2.f*(he.hx+params.vacuumPadding),
                            2.f*(he.hy+params.vacuumPadding),
                            2.f*(he.hz+params.vacuumPadding));
    }

    ImGui::EndChild(); // ##nanoParamsScroll

    ImGui::Separator();
    ImGui::Text("Result");
    drawNanoBuildResult(lastResult);

    ImGui::EndChild(); // ##nanoBuilderOptions

    // =========================================================================
    // Embedded file browser (shown when Load File... button pressed)
    // =========================================================================
    if (m_browsShowPanel)
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Load Reference File");
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f);
        if (ImGui::SmallButton("Close##nanoCloseBrows"))
            m_browsShowPanel = false;

        // Nav bar
        const float nb = 28.0f;
        if (ImGui::Button("^##nanoUp", ImVec2(nb, 0.0f))) {
            m_browsDir = parentPath(m_browsDir);
            m_browsEntryDirty = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Up one level");
        ImGui::SameLine(0.0f, 4.0f);
        if (ImGui::Button("Home##nanoBrowsHome", ImVec2(0.0f, 0.0f))) {
            m_browsDir = detectHomePath();
            m_browsEntryDirty = true;
        }
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        {
            static char s_nanoDirBuf[2048] = {};
            static std::string s_prevNanoDir;
            if (s_prevNanoDir != m_browsDir) {
                std::snprintf(s_nanoDirBuf, sizeof(s_nanoDirBuf), "%s", m_browsDir.c_str());
                s_prevNanoDir = m_browsDir;
            }
            if (ImGui::InputText("##nanoDirBar", s_nanoDirBuf, sizeof(s_nanoDirBuf),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                m_browsDir = normalizePathSeparators(std::string(s_nanoDirBuf));
                m_browsEntryDirty = true;
            }
        }

        if (m_browsEntryDirty)
            refreshBrowserEntries();

        if (ImGui::BeginChild("##nanoBrowsList", ImVec2(-1.0f, 200.0f), true)) {
            bool fileDoubleClicked = false;
            drawDirectoryEntries(m_browsEntries, m_browsFilename, 50000,
                [&](const std::string& name) {
                    m_browsDir = joinPath(m_browsDir, name);
                    m_browsEntryDirty = true;
                }, &fileDoubleClicked);
            if (fileDoubleClicked) {
                tryLoadFile(joinPath(m_browsDir, m_browsFilename), elementRadii, elementShininess);
                m_browsShowPanel = false;
            }
            ImGui::EndChild();
        }

        static const char* kNanoExtHint = ".cif  .xyz  .pdb  .sdf  .mol  .vasp  .mol2";
        const float extHintW = ImGui::CalcTextSize(kNanoExtHint).x;
        const float labelW   = ImGui::CalcTextSize("Filename").x + ImGui::GetStyle().ItemInnerSpacing.x * 2.0f;
        const float inW      = ImGui::GetContentRegionAvail().x - labelW - extHintW - ImGui::GetStyle().ItemSpacing.x * 2.0f;
        ImGui::SetNextItemWidth(inW > 100.0f ? inW : 100.0f);
        bool enterPressed = ImGui::InputText("Filename##nanoBrowsInput", m_browsFilename,
                                             sizeof(m_browsFilename),
                                             ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%s", kNanoExtHint);

        if (enterPressed)
        {
            tryLoadFile(joinPath(m_browsDir, m_browsFilename), elementRadii, elementShininess);
            m_browsShowPanel = false;
        }
        if (ImGui::Button("Load##nanoBrowsGo", ImVec2(110.0f, 0.0f)))
        {
            tryLoadFile(joinPath(m_browsDir, m_browsFilename), elementRadii, elementShininess);
            m_browsShowPanel = false;
        }
        ImGui::SameLine(0.0f, 8.0f);
        if (ImGui::Button("Cancel##nanoBrowsCancel", ImVec2(90.0f, 0.0f)))
            m_browsShowPanel = false;
    }

    // =========================================================================
    // Action bar
    // =========================================================================

    const bool canBuild = !m_reference.atoms.empty();
    if (!canBuild) ImGui::BeginDisabled();
    if (ImGui::Button("Build##nano", ImVec2(100.0f, 0.0f))) {
        static const std::vector<glm::vec3>       kNoVerts;
        static const std::vector<unsigned int>    kNoIdx;
        lastResult = buildNanocrystal(structure,
                                      m_reference,
                                      params,
                                      elementColors,
                                      kNoVerts,
                                      kNoIdx);
        if (lastResult.success)
            updateBuffers(structure);
    }
    if (!canBuild) ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Close##nano", ImVec2(80.0f, 0.0f))) {
        lastResult  = {};
        m_isOpen    = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}
