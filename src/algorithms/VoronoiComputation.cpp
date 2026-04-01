#include "algorithms/VoronoiComputation.h"
#include "math/StructureMath.h"

#include <algorithm>
#include <cmath>

namespace
{
// A convex polyhedron represented as a set of faces (convex polygons).
struct ConvexPoly
{
    std::vector<VoronoiFace> faces;
};

// Order polygon vertices around their centroid so they form a convex loop.
void sortFaceVertices(VoronoiFace& face)
{
    if (face.vertices.size() < 3)
        return;

    glm::vec3 center(0.0f);
    for (const auto& v : face.vertices)
        center += v;
    center /= (float)face.vertices.size();

    // Compute face normal from first three vertices.
    glm::vec3 normal = glm::normalize(
        glm::cross(face.vertices[1] - face.vertices[0],
                    face.vertices[2] - face.vertices[0]));

    glm::vec3 ref = glm::normalize(face.vertices[0] - center);
    glm::vec3 binormal = glm::cross(normal, ref);

    std::sort(face.vertices.begin(), face.vertices.end(),
        [&](const glm::vec3& a, const glm::vec3& b) {
            glm::vec3 da = a - center;
            glm::vec3 db = b - center;
            float angA = std::atan2(glm::dot(da, binormal), glm::dot(da, ref));
            float angB = std::atan2(glm::dot(db, binormal), glm::dot(db, ref));
            return angA < angB;
        });
}

// Add point if not already close to an existing one.
void addUnique(std::vector<glm::vec3>& pts, const glm::vec3& p, float eps2 = 1e-8f)
{
    for (const auto& q : pts)
    {
        if (glm::dot(p - q, p - q) <= eps2)
            return;
    }
    pts.push_back(p);
}

// Clip a convex polygon (list of vertices in order) against a half-plane:
// keeps the side where dot(v - planePoint, planeNormal) <= 0.
std::vector<glm::vec3> clipPolygonByPlane(const std::vector<glm::vec3>& poly,
                                          const glm::vec3& planePoint,
                                          const glm::vec3& planeNormal)
{
    if (poly.size() < 3)
        return {};

    std::vector<glm::vec3> out;
    out.reserve(poly.size() + 2);

    for (size_t i = 0; i < poly.size(); ++i)
    {
        const glm::vec3& curr = poly[i];
        const glm::vec3& next = poly[(i + 1) % poly.size()];

        float dCurr = glm::dot(curr - planePoint, planeNormal);
        float dNext = glm::dot(next - planePoint, planeNormal);

        bool currInside = dCurr <= 1e-6f;
        bool nextInside = dNext <= 1e-6f;

        if (currInside)
            out.push_back(curr);

        if (currInside != nextInside)
        {
            float denom = dCurr - dNext;
            if (std::abs(denom) > 1e-10f)
            {
                float t = dCurr / denom;
                t = std::max(0.0f, std::min(1.0f, t));
                out.push_back(curr + t * (next - curr));
            }
        }
    }

    return (out.size() >= 3) ? out : std::vector<glm::vec3>();
}

// Clip a convex polyhedron against a half-plane (dot(v - planePoint, planeNormal) <= 0).
// Returns the clipped polyhedron. The cut face (new face from the clipping) is added.
ConvexPoly clipPolyhedronByPlane(const ConvexPoly& poly,
                                 const glm::vec3& planePoint,
                                 const glm::vec3& planeNormal)
{
    ConvexPoly result;
    std::vector<glm::vec3> cutEdgePoints;

    for (const auto& face : poly.faces)
    {
        auto clipped = clipPolygonByPlane(face.vertices, planePoint, planeNormal);
        if (clipped.size() >= 3)
        {
            VoronoiFace f;
            f.vertices = std::move(clipped);
            result.faces.push_back(std::move(f));
        }
    }

    // Collect intersection points on the cutting plane from all clipped faces.
    for (const auto& face : result.faces)
    {
        for (const auto& v : face.vertices)
        {
            float d = glm::dot(v - planePoint, planeNormal);
            if (std::abs(d) < 1e-4f)
                addUnique(cutEdgePoints, v);
        }
    }

    // Create the new cap face from the cutting plane.
    if (cutEdgePoints.size() >= 3)
    {
        VoronoiFace capFace;
        capFace.vertices = cutEdgePoints;
        sortFaceVertices(capFace);
        result.faces.push_back(std::move(capFace));
    }

    return result;
}

// Build a box-shaped convex polyhedron (6 faces, axis-aligned).
ConvexPoly makeBox(const glm::vec3& lo, const glm::vec3& hi)
{
    ConvexPoly box;
    // bottom (y = lo.y)
    box.faces.push_back({{
        {lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z},
        {hi.x, lo.y, hi.z}, {lo.x, lo.y, hi.z}}});
    // top (y = hi.y)
    box.faces.push_back({{
        {lo.x, hi.y, lo.z}, {lo.x, hi.y, hi.z},
        {hi.x, hi.y, hi.z}, {hi.x, hi.y, lo.z}}});
    // front (z = lo.z)
    box.faces.push_back({{
        {lo.x, lo.y, lo.z}, {lo.x, hi.y, lo.z},
        {hi.x, hi.y, lo.z}, {hi.x, lo.y, lo.z}}});
    // back (z = hi.z)
    box.faces.push_back({{
        {lo.x, lo.y, hi.z}, {hi.x, lo.y, hi.z},
        {hi.x, hi.y, hi.z}, {lo.x, hi.y, hi.z}}});
    // left (x = lo.x)
    box.faces.push_back({{
        {lo.x, lo.y, lo.z}, {lo.x, lo.y, hi.z},
        {lo.x, hi.y, hi.z}, {lo.x, hi.y, lo.z}}});
    // right (x = hi.x)
    box.faces.push_back({{
        {hi.x, lo.y, lo.z}, {hi.x, hi.y, lo.z},
        {hi.x, hi.y, hi.z}, {hi.x, lo.y, hi.z}}});
    return box;
}
}

VoronoiDiagram computeVoronoi(const Structure& structure)
{
    VoronoiDiagram diagram;
    if (structure.atoms.empty())
        return diagram;

    const bool usePbc = structure.hasUnitCell;

    glm::mat3 cell(1.0f);
    glm::mat3 invCell(1.0f);
    glm::vec3 origin(0.0f);

    if (usePbc)
    {
        if (!tryMakeCellMatrices(structure, cell, invCell))
            return diagram;

        origin = glm::vec3(
            (float)structure.cellOffset[0],
            (float)structure.cellOffset[1],
            (float)structure.cellOffset[2]);
    }

    const size_t N = structure.atoms.size();
    std::vector<glm::vec3> positions(N);
    for (size_t i = 0; i < N; ++i)
    {
        positions[i] = glm::vec3(
            (float)structure.atoms[i].x,
            (float)structure.atoms[i].y,
            (float)structure.atoms[i].z);
    }

    // For PBC, wrap atom positions to match the renderer's
    // wrapAndSnapFractional: map to [0,1), snap near-0 and near-1 to 0.
    const float kSnapTol = 1e-4f;
    if (usePbc)
    {
        for (size_t i = 0; i < N; ++i)
        {
            glm::vec3 frac = invCell * (positions[i] - origin);
            frac.x -= std::floor(frac.x);
            frac.y -= std::floor(frac.y);
            frac.z -= std::floor(frac.z);
            if (std::abs(frac.x) <= kSnapTol)       frac.x = 0.0f;
            if (std::abs(1.0f - frac.x) <= kSnapTol) frac.x = 0.0f;
            if (std::abs(frac.y) <= kSnapTol)       frac.y = 0.0f;
            if (std::abs(1.0f - frac.y) <= kSnapTol) frac.y = 0.0f;
            if (std::abs(frac.z) <= kSnapTol)       frac.z = 0.0f;
            if (std::abs(1.0f - frac.z) <= kSnapTol) frac.z = 0.0f;
            positions[i] = origin + cell * frac;
        }
    }

    // Bounding box for non-PBC case.
    glm::vec3 bboxLo(1e30f), bboxHi(-1e30f);
    if (!usePbc)
    {
        for (const auto& p : positions)
        {
            bboxLo = glm::min(bboxLo, p);
            bboxHi = glm::max(bboxHi, p);
        }
        glm::vec3 extent = bboxHi - bboxLo;
        float pad = std::max({extent.x, extent.y, extent.z}) * 0.5f + 2.0f;
        bboxLo -= glm::vec3(pad);
        bboxHi += glm::vec3(pad);
    }

    const int imageRange = 1;

    // For PBC, the atom-centred initial box half-width must be large enough
    // to contain the full Voronoi cell.  The Voronoi cell radius is at most
    // max(|a|,|b|,|c|) (one full cell vector length) which is a safe upper
    // bound; sorted-neighbour early termination keeps clipping fast.
    float maxCellLen = 0.0f;
    if (usePbc)
        maxCellLen = std::max({glm::length(cell[0]), glm::length(cell[1]), glm::length(cell[2])});

    struct Neighbor { float dist2; glm::vec3 pos; };

    diagram.cells.resize(N);

    for (size_t i = 0; i < N; ++i)
    {
        const glm::vec3& pi = positions[i];

        // Build neighbour list.
        std::vector<Neighbor> neighbors;

        if (usePbc)
        {
            for (size_t j = 0; j < N; ++j)
            {
                for (int da = -imageRange; da <= imageRange; ++da)
                for (int db = -imageRange; db <= imageRange; ++db)
                for (int dc = -imageRange; dc <= imageRange; ++dc)
                {
                    if (j == i && da == 0 && db == 0 && dc == 0)
                        continue;
                    glm::vec3 shift = cell * glm::vec3((float)da, (float)db, (float)dc);
                    glm::vec3 pj = positions[j] + shift;
                    glm::vec3 diff = pj - pi;
                    float d2 = glm::dot(diff, diff);
                    // Skip coincident atoms (can happen in grain boundaries).
                    if (d2 < 1e-6f)
                        continue;
                    neighbors.push_back({d2, pj});
                }
            }
        }
        else
        {
            neighbors.reserve(N);
            for (size_t j = 0; j < N; ++j)
            {
                if (j == i)
                    continue;
                glm::vec3 diff = positions[j] - pi;
                neighbors.push_back({glm::dot(diff, diff), positions[j]});
            }
        }

        // Sort nearest first for fast convergence.
        std::sort(neighbors.begin(), neighbors.end(),
            [](const Neighbor& a, const Neighbor& b) { return a.dist2 < b.dist2; });

        // Start with an atom-centred AABB large enough to contain the full
        // Voronoi cell.  Bisector clipping will shrink it down quickly.
        ConvexPoly cell_poly;
        if (usePbc)
            cell_poly = makeBox(pi - glm::vec3(maxCellLen), pi + glm::vec3(maxCellLen));
        else
            cell_poly = makeBox(bboxLo, bboxHi);

        // Track the maximum squared distance from the atom to any vertex.
        float rmax2 = 0.0f;
        for (const auto& face : cell_poly.faces)
            for (const auto& v : face.vertices)
            {
                float d2 = glm::dot(v - pi, v - pi);
                if (d2 > rmax2) rmax2 = d2;
            }

        // Clip against bisector planes, nearest neighbours first.
        for (const auto& nb : neighbors)
        {
            if (cell_poly.faces.empty())
                break;

            // Early termination: the bisector sits at dist/2 from pi.
            // If dist/2 > rmax, no remaining neighbour can clip the cell.
            if (nb.dist2 > 4.0f * rmax2)
                break;

            float len = std::sqrt(nb.dist2);
            glm::vec3 midpoint = 0.5f * (pi + nb.pos);
            glm::vec3 normal = (nb.pos - pi) / len;

            cell_poly = clipPolyhedronByPlane(cell_poly, midpoint, normal);

            // Recompute rmax2 after clip.
            rmax2 = 0.0f;
            for (const auto& face : cell_poly.faces)
                for (const auto& v : face.vertices)
                {
                    float d2 = glm::dot(v - pi, v - pi);
                    if (d2 > rmax2) rmax2 = d2;
                }
        }

        for (auto& face : cell_poly.faces)
            sortFaceVertices(face);

        VoronoiCell vc;
        vc.faces = std::move(cell_poly.faces);
        diagram.cells[i] = std::move(vc);
    }

    // Replicate Voronoi cells for boundary atoms to match the periodic
    // image atoms rendered by appendPbcBoundaryImages.
    // Use the same tolerance as the renderer (pbcBoundaryTol or kSnapTol).
    // Atoms with fractional coordinate near 0 get a copy shifted by +1.
    if (usePbc)
    {
        const float tol = structure.pbcBoundaryTol > 0.0f
                        ? structure.pbcBoundaryTol : kSnapTol;

        // Cap the tolerance: in fractional coords it should never exceed 0.5
        // (otherwise interior atoms get replicated for no reason).
        const float safeTol = std::min(tol, 0.5f);

        // Collect replicated cells in a separate vector to avoid
        // invalidating references into diagram.cells during push_back.
        std::vector<VoronoiCell> replicatedCells;

        for (size_t i = 0; i < N; ++i)
        {
            const auto& baseCell = diagram.cells[i];
            if (baseCell.faces.empty())
                continue;

            glm::vec3 frac = invCell * (positions[i] - origin);

            std::vector<int> sx = {0}, sy = {0}, sz = {0};
            if (std::abs(frac.x) <= safeTol) sx.push_back(1);
            if (std::abs(frac.y) <= safeTol) sy.push_back(1);
            if (std::abs(frac.z) <= safeTol) sz.push_back(1);

            for (int a : sx)
            for (int b : sy)
            for (int c : sz)
            {
                if (a == 0 && b == 0 && c == 0)
                    continue;

                glm::vec3 shift = cell * glm::vec3((float)a, (float)b, (float)c);
                VoronoiCell sc;
                for (const auto& face : baseCell.faces)
                {
                    VoronoiFace sf;
                    sf.vertices.reserve(face.vertices.size());
                    for (const auto& v : face.vertices)
                        sf.vertices.push_back(v + shift);
                    sc.faces.push_back(std::move(sf));
                }
                replicatedCells.push_back(std::move(sc));
            }
        }

        for (auto& rc : replicatedCells)
            diagram.cells.push_back(std::move(rc));
    }

    return diagram;
}
