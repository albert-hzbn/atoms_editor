#include "ui/LatticePlaneOverlay.h"

#include "imgui.h"
#include "math/StructureMath.h"

#include <algorithm>
#include <cmath>

namespace
{
struct Edge
{
    int a;
    int b;
};

const glm::vec3 kCubeVertices[8] = {
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(1.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3(1.0f, 1.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 1.0f),
    glm::vec3(1.0f, 0.0f, 1.0f),
    glm::vec3(0.0f, 1.0f, 1.0f),
    glm::vec3(1.0f, 1.0f, 1.0f)
};

const Edge kCubeEdges[12] = {
    {0, 1}, {1, 3}, {3, 2}, {2, 0},
    {4, 5}, {5, 7}, {7, 6}, {6, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

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

    auto toView = [&](const glm::vec3& p) {
        return view * glm::vec4(p, 1.0f);
    };

    auto isInside = [&](const glm::vec4& pv) {
        return pv.z <= -nearDepth;
    };

    for (size_t i = 0; i < worldPoints.size(); ++i)
    {
        const glm::vec3 currentWorld = worldPoints[i];
        const glm::vec3 nextWorld = worldPoints[(i + 1) % worldPoints.size()];

        const glm::vec4 currentView = toView(currentWorld);
        const glm::vec4 nextView = toView(nextWorld);
        const bool currentInside = isInside(currentView);
        const bool nextInside = isInside(nextView);

        if (currentInside)
            clipped.push_back(currentWorld);

        if (currentInside != nextInside)
        {
            const float denom = nextView.z - currentView.z;
            if (std::abs(denom) > 1e-6f)
            {
                const float t = (-nearDepth - currentView.z) / denom;
                clipped.push_back(currentWorld + (nextWorld - currentWorld) * glm::clamp(t, 0.0f, 1.0f));
            }
        }
    }

    return (clipped.size() >= 3) ? clipped : std::vector<glm::vec3>();
}

void addUniquePoint(std::vector<glm::vec3>& points, const glm::vec3& p)
{
    const float eps2 = 1e-8f;
    for (size_t i = 0; i < points.size(); ++i)
    {
        const glm::vec3 d = points[i] - p;
        if (glm::dot(d, d) <= eps2)
            return;
    }
    points.push_back(p);
}

std::vector<glm::vec3> computePlanePolygonFractional(int h, int k, int l, float offset)
{
    std::vector<glm::vec3> points;
    const glm::vec3 n((float)h, (float)k, (float)l);
    const float eps = 1e-6f;

    for (int i = 0; i < 12; ++i)
    {
        const glm::vec3 p0 = kCubeVertices[kCubeEdges[i].a];
        const glm::vec3 p1 = kCubeVertices[kCubeEdges[i].b];

        const float f0 = glm::dot(n, p0) - offset;
        const float f1 = glm::dot(n, p1) - offset;

        if (std::abs(f0) <= eps && std::abs(f1) <= eps)
        {
            addUniquePoint(points, p0);
            addUniquePoint(points, p1);
            continue;
        }

        const float denom = f0 - f1;
        if (std::abs(denom) <= eps)
            continue;

        const float t = f0 / denom;
        if (t < -eps || t > 1.0f + eps)
            continue;

        addUniquePoint(points, p0 + (p1 - p0) * glm::clamp(t, 0.0f, 1.0f));
    }

    if (points.size() < 3)
        return {};

    glm::vec3 center(0.0f);
    for (size_t i = 0; i < points.size(); ++i)
        center += points[i];
    center /= (float)points.size();

    glm::vec3 normal = glm::normalize(n);
    glm::vec3 reference = (std::abs(normal.z) < 0.8f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 axisU = glm::normalize(glm::cross(reference, normal));
    glm::vec3 axisV = glm::cross(normal, axisU);

    std::sort(points.begin(), points.end(),
              [&](const glm::vec3& a, const glm::vec3& b) {
        const glm::vec3 da = a - center;
        const glm::vec3 db = b - center;
        const float angleA = std::atan2(glm::dot(da, axisV), glm::dot(da, axisU));
        const float angleB = std::atan2(glm::dot(db, axisV), glm::dot(db, axisU));
        return angleA < angleB;
    });

    return points;
}
}

void drawLatticePlanesOverlay(ImDrawList* drawList,
                              const glm::mat4& projection,
                              const glm::mat4& view,
                              int framebufferWidth,
                              int framebufferHeight,
                              const Structure& structure,
                              const std::vector<LatticePlane>& planes,
                              bool enabled)
{
    if (!enabled || !structure.hasUnitCell || drawList == nullptr || planes.empty())
        return;

    glm::mat3 cell(1.0f);
    glm::mat3 invCell(1.0f);
    if (!tryMakeCellMatrices(structure, cell, invCell))
        return;
    (void)invCell;

    const glm::vec3 origin((float)structure.cellOffset[0],
                           (float)structure.cellOffset[1],
                           (float)structure.cellOffset[2]);

    for (size_t i = 0; i < planes.size(); ++i)
    {
        const LatticePlane& plane = planes[i];
        if (!plane.visible)
            continue;
        if (plane.h == 0 && plane.k == 0 && plane.l == 0)
            continue;

        const std::vector<glm::vec3> polygonFrac =
            computePlanePolygonFractional(plane.h, plane.k, plane.l, plane.offset);
        if (polygonFrac.size() < 3)
            continue;

        std::vector<glm::vec3> polygonWorld;
        polygonWorld.reserve(polygonFrac.size());
        for (size_t p = 0; p < polygonFrac.size(); ++p)
        {
            polygonWorld.push_back(origin + cell * polygonFrac[p]);
        }

        const std::vector<glm::vec3> clippedWorld =
            clipPolygonAgainstNearPlane(polygonWorld, view, 0.03f);
        if (clippedWorld.size() < 3)
            continue;

        ImVector<ImVec2> screenPoints;
        screenPoints.reserve((int)clippedWorld.size());
        for (size_t p = 0; p < clippedWorld.size(); ++p)
        {
            ImVec2 screen;
            if (!projectToScreen(clippedWorld[p], projection, view, framebufferWidth, framebufferHeight, screen))
                continue;
            screenPoints.push_back(screen);
        }

        if (screenPoints.Size < 3)
            continue;

        const float alpha = glm::clamp(plane.opacity, 0.0f, 1.0f);
        const ImU32 fillColor = ImColor(plane.color[0], plane.color[1], plane.color[2], alpha);
        const ImU32 edgeColor = ImColor(plane.color[0], plane.color[1], plane.color[2], 0.95f);

        drawList->AddConvexPolyFilled(screenPoints.Data, screenPoints.Size, fillColor);
        drawList->AddPolyline(screenPoints.Data, screenPoints.Size, edgeColor, ImDrawFlags_Closed, 2.0f);
    }
}
