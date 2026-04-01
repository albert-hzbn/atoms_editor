#include "ui/VoronoiOverlay.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{

// Compute the volume of a convex polyhedron from its triangulated faces
// using the divergence theorem: V = |1/6 * sum(v0 . (v1 x v2))| per triangle.
float computeCellVolume(const VoronoiCell& cell)
{
    float vol = 0.0f;
    for (const auto& face : cell.faces)
    {
        if (face.vertices.size() < 3)
            continue;
        const glm::vec3& v0 = face.vertices[0];
        for (size_t i = 1; i + 1 < face.vertices.size(); ++i)
        {
            const glm::vec3& v1 = face.vertices[i];
            const glm::vec3& v2 = face.vertices[i + 1];
            vol += glm::dot(v0, glm::cross(v1, v2));
        }
    }
    return std::abs(vol) / 6.0f;
}

bool projectToScreen(const glm::vec3& p,
                     const glm::mat4& projection,
                     const glm::mat4& view,
                     int w,
                     int h,
                     ImVec2& out)
{
    glm::vec4 clip = projection * view * glm::vec4(p, 1.0f);
    if (clip.w <= 1e-5f)
        return false;

    const float invW = 1.0f / clip.w;
    const float nx = clip.x * invW;
    const float ny = clip.y * invW;

    out.x = (nx * 0.5f + 0.5f) * (float)w;
    out.y = (1.0f - (ny * 0.5f + 0.5f)) * (float)h;
    return true;
}

std::vector<glm::vec3> clipPolygonAgainstNearPlane(const std::vector<glm::vec3>& worldPoints,
                                                   const glm::mat4& view,
                                                   float nearDepth)
{
    if (worldPoints.size() < 3)
        return {};

    std::vector<glm::vec3> clipped;
    clipped.reserve(worldPoints.size() + 2);

    for (size_t i = 0; i < worldPoints.size(); ++i)
    {
        const glm::vec3& currentWorld = worldPoints[i];
        const glm::vec3& nextWorld = worldPoints[(i + 1) % worldPoints.size()];

        glm::vec4 currentView = view * glm::vec4(currentWorld, 1.0f);
        glm::vec4 nextView = view * glm::vec4(nextWorld, 1.0f);
        bool currentInside = currentView.z <= -nearDepth;
        bool nextInside = nextView.z <= -nearDepth;

        if (currentInside)
            clipped.push_back(currentWorld);

        if (currentInside != nextInside)
        {
            float denom = nextView.z - currentView.z;
            if (std::abs(denom) > 1e-6f)
            {
                float t = (-nearDepth - currentView.z) / denom;
                t = std::max(0.0f, std::min(1.0f, t));
                clipped.push_back(currentWorld + (nextWorld - currentWorld) * t);
            }
        }
    }

    return (clipped.size() >= 3) ? clipped : std::vector<glm::vec3>();
}
}

void drawVoronoiOverlay(ImDrawList* drawList,
                        const glm::mat4& projection,
                        const glm::mat4& view,
                        int framebufferWidth,
                        int framebufferHeight,
                        const VoronoiDiagram& diagram,
                        const std::vector<int>& selectedIndices,
                        bool enabled)
{
    if (!enabled || drawList == nullptr || diagram.cells.empty())
        return;

    const ImU32 selectedFill = ImColor(1.0f, 0.2f, 0.2f, 0.15f);
    const ImU32 selectedEdge = ImColor(1.0f, 0.2f, 0.2f, 0.85f);

    // Compute volumes for all cells to determine color mapping.
    const size_t numCells = diagram.cells.size();
    std::vector<float> volumes(numCells);
    float minVol = std::numeric_limits<float>::max();
    float maxVol = 0.0f;
    for (size_t ci = 0; ci < numCells; ++ci)
    {
        volumes[ci] = computeCellVolume(diagram.cells[ci]);
        if (volumes[ci] < minVol) minVol = volumes[ci];
        if (volumes[ci] > maxVol) maxVol = volumes[ci];
    }
    const float volRange = (maxVol - minVol > 1e-10f) ? (maxVol - minVol) : 1.0f;

    // Build a fast lookup for selected cell indices.
    std::vector<bool> isSelected(numCells, false);
    for (int idx : selectedIndices)
    {
        if (idx >= 0 && idx < (int)isSelected.size())
            isSelected[idx] = true;
    }

    for (size_t ci = 0; ci < numCells; ++ci)
    {
        const auto& cell = diagram.cells[ci];

        // Map volume to 0..1: 0 = smallest volume (light blue), 1 = largest (dark blue).
        const float t = (volumes[ci] - minVol) / volRange;

        // Light blue (0.6, 0.8, 1.0) for small volume -> dark blue (0.0, 0.1, 0.6) for large volume.
        const float fillR = 0.6f * (1.0f - t);
        const float fillG = 0.8f * (1.0f - t) + 0.1f * t;
        const float fillB = 1.0f * (1.0f - t) + 0.6f * t;
        const float fillA = 0.08f + 0.12f * t; // slightly more opaque for larger cells

        const float edgeR = 0.4f * (1.0f - t);
        const float edgeG = 0.6f * (1.0f - t) + 0.1f * t;
        const float edgeB = 1.0f * (1.0f - t) + 0.7f * t;

        const ImU32 fillColor = isSelected[ci] ? selectedFill : (ImU32)ImColor(fillR, fillG, fillB, fillA);
        const ImU32 edgeColor = isSelected[ci] ? selectedEdge : (ImU32)ImColor(edgeR, edgeG, edgeB, 0.75f);
        for (const auto& face : cell.faces)
        {
            if (face.vertices.size() < 3)
                continue;

            const auto clippedWorld =
                clipPolygonAgainstNearPlane(face.vertices, view, 0.03f);
            if (clippedWorld.size() < 3)
                continue;

            ImVector<ImVec2> screenPoints;
            screenPoints.reserve((int)clippedWorld.size());
            bool allProjected = true;
            for (const auto& v : clippedWorld)
            {
                ImVec2 screen;
                if (!projectToScreen(v, projection, view,
                                     framebufferWidth, framebufferHeight, screen))
                {
                    allProjected = false;
                    break;
                }
                screenPoints.push_back(screen);
            }

            if (!allProjected || screenPoints.Size < 3)
                continue;

            drawList->AddConvexPolyFilled(screenPoints.Data, screenPoints.Size, fillColor);
            drawList->AddPolyline(screenPoints.Data, screenPoints.Size,
                                  edgeColor, ImDrawFlags_Closed, 1.5f);
        }
    }
}
