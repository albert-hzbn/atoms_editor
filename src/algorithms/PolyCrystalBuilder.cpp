#include "algorithms/PolyCrystalBuilder.h"

#include "ElementData.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>

// ---------------------------------------------------------------------------
// Euler angles (Bunge convention, degrees) -> rotation matrix
// ---------------------------------------------------------------------------
static glm::mat3 eulerToMatrix(float phi1Deg, float PhiDeg, float phi2Deg)
{
    const float p1 = glm::radians(phi1Deg);
    const float P  = glm::radians(PhiDeg);
    const float p2 = glm::radians(phi2Deg);

    const float c1 = std::cos(p1), s1 = std::sin(p1);
    const float cP = std::cos(P),  sP = std::sin(P);
    const float c2 = std::cos(p2), s2 = std::sin(p2);

    // Bunge convention: Z1-X-Z2
    glm::mat3 R;
    R[0][0] =  c1*c2 - s1*s2*cP;   R[1][0] =  s1*c2 + c1*s2*cP;   R[2][0] =  s2*sP;
    R[0][1] = -c1*s2 - s1*c2*cP;   R[1][1] = -s1*s2 + c1*c2*cP;   R[2][1] =  c2*sP;
    R[0][2] =  s1*sP;              R[1][2] = -c1*sP;              R[2][2] =  cP;
    return R;
}

// Generate a uniformly random rotation quaternion -> matrix
static glm::mat3 randomRotation(std::mt19937& rng)
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    const float u1 = dist(rng);
    const float u2 = dist(rng) * 2.0f * 3.14159265358979f;
    const float u3 = dist(rng) * 2.0f * 3.14159265358979f;

    const float q0 = std::sqrt(1.0f - u1) * std::sin(u2);
    const float q1 = std::sqrt(1.0f - u1) * std::cos(u2);
    const float q2 = std::sqrt(u1)        * std::sin(u3);
    const float q3 = std::sqrt(u1)        * std::cos(u3);

    glm::quat q(q3, q0, q1, q2);
    return glm::mat3_cast(glm::normalize(q));
}

// ---------------------------------------------------------------------------
// Voronoi assignment: for each point, find out which seed is closest.
// ---------------------------------------------------------------------------
static int nearestSeed(const glm::vec3& pos,
                       const std::vector<glm::vec3>& seeds)
{
    int best = 0;
    float bestDist2 = glm::dot(pos - seeds[0], pos - seeds[0]);
    for (int i = 1; i < (int)seeds.size(); ++i)
    {
        const glm::vec3 d = pos - seeds[i];
        const float d2 = glm::dot(d, d);
        if (d2 < bestDist2)
        {
            bestDist2 = d2;
            best = i;
        }
    }
    return best;
}

// ---------------------------------------------------------------------------
// IPF-Z coloring for cubic symmetry (EBSD-style)
// Maps a grain rotation to an RGB color based on the crystal direction
// aligned with the sample Z-axis.
// ---------------------------------------------------------------------------
static std::array<float, 3> computeIPFColor(const glm::mat3& grainRotation)
{
    // Crystal direction corresponding to sample Z = [0,0,1]
    // d = R^T * [0,0,1]  (inverse rotation applied to Z)
    const glm::vec3 zDir(0.0f, 0.0f, 1.0f);
    const glm::vec3 crystalDir = glm::transpose(grainRotation) * zDir;

    // Apply cubic symmetry: take absolute values, sort descending
    float h = std::abs(crystalDir.x);
    float k = std::abs(crystalDir.y);
    float l = std::abs(crystalDir.z);

    if (h < k) std::swap(h, k);
    if (h < l) std::swap(h, l);
    if (k < l) std::swap(k, l);
    // Now h >= k >= l >= 0

    float len = std::sqrt(h * h + k * k + l * l);
    if (len < 1e-10f) return {{0.5f, 0.5f, 0.5f}};
    h /= len; k /= len; l /= len;

    // Map to standard triangle RGB:
    // [001] -> Red, [011] -> Green, [111] -> Blue
    float r = h - k;
    float g = k - l;
    float b = l * 1.7320508f; // sqrt(3) to normalize [111] blue to ~1

    // Normalize so max channel = 1 for vivid colors
    float maxC = std::max({r, g, b, 1e-6f});
    r /= maxC;
    g /= maxC;
    b /= maxC;

    return {{r, g, b}};
}

// ---------------------------------------------------------------------------
// Builder
// ---------------------------------------------------------------------------
PolyBuildResult buildPolycrystal(Structure& structure,
                                 const Structure& reference,
                                 const PolyParams& params,
                                 const std::vector<glm::vec3>& elementColors)
{
    PolyBuildResult result;
    result.inputAtoms = (int)reference.atoms.size();
    result.numGrains  = params.numGrains;

    // --- Validation ---------------------------------------------------------
    if (reference.atoms.empty())
    {
        result.message = "Reference structure has no atoms.";
        return result;
    }
    if (!reference.hasUnitCell)
    {
        result.message = "Reference structure needs a unit cell for polycrystal tiling.";
        return result;
    }
    if (params.numGrains < 1)
    {
        result.message = "Number of grains must be at least 1.";
        return result;
    }
    if (params.sizeX <= 0.0f || params.sizeY <= 0.0f || params.sizeZ <= 0.0f)
    {
        result.message = "Box dimensions must be positive.";
        return result;
    }

    // --- Lattice vectors ----------------------------------------------------
    const auto& cv = reference.cellVectors;
    const glm::vec3 a((float)cv[0][0], (float)cv[0][1], (float)cv[0][2]);
    const glm::vec3 b((float)cv[1][0], (float)cv[1][1], (float)cv[1][2]);
    const glm::vec3 c((float)cv[2][0], (float)cv[2][1], (float)cv[2][2]);

    const float la = glm::length(a), lb = glm::length(b), lc = glm::length(c);
    if (la < 1e-8f || lb < 1e-8f || lc < 1e-8f)
    {
        result.message = "Reference structure has degenerate lattice vectors.";
        return result;
    }

    // --- Generate Voronoi seeds (grain centres) -----------------------------
    std::mt19937 rng((unsigned)params.seed);
    std::uniform_real_distribution<float> distX(0.0f, params.sizeX);
    std::uniform_real_distribution<float> distY(0.0f, params.sizeY);
    std::uniform_real_distribution<float> distZ(0.0f, params.sizeZ);

    std::vector<glm::vec3> seeds(params.numGrains);
    for (int i = 0; i < params.numGrains; ++i)
        seeds[i] = glm::vec3(distX(rng), distY(rng), distZ(rng));

    // --- Build rotation matrix per grain ------------------------------------
    std::vector<glm::mat3> grainRotations(params.numGrains);

    if (params.orientationMode == GrainOrientationMode::AllRandom)
    {
        for (int i = 0; i < params.numGrains; ++i)
            grainRotations[i] = randomRotation(rng);
    }
    else if (params.orientationMode == GrainOrientationMode::AllSpecified)
    {
        for (int i = 0; i < params.numGrains; ++i)
        {
            if (i < (int)params.specifiedOrientations.size())
            {
                const auto& o = params.specifiedOrientations[i];
                grainRotations[i] = eulerToMatrix(o.phi1, o.Phi, o.phi2);
            }
            else
            {
                grainRotations[i] = glm::mat3(1.0f); // identity fallback
            }
        }
    }
    else // PartialSpecified
    {
        // Start with random for all
        for (int i = 0; i < params.numGrains; ++i)
            grainRotations[i] = randomRotation(rng);

        // Override specified entries
        for (const auto& o : params.specifiedOrientations)
        {
            if (o.grainIndex >= 0 && o.grainIndex < params.numGrains)
                grainRotations[o.grainIndex] = eulerToMatrix(o.phi1, o.Phi, o.phi2);
        }
    }

    // --- Determine tiling extents -------------------------------------------
    // The diagonal of the box is the maximum radius we need to cover.
    const float boxDiag = std::sqrt(params.sizeX * params.sizeX +
                                    params.sizeY * params.sizeY +
                                    params.sizeZ * params.sizeZ);

    // After rotation, lattice vectors change, so we tile generously.
    const float minLatticeLen = std::min({la, lb, lc});
    const int maxRep = std::min(100, (int)std::ceil(boxDiag / minLatticeLen) + 2);

    // Estimate total atom count to prevent memory blowup
    const long long totalEstimate = (long long)(2 * maxRep + 1) *
                                    (long long)(2 * maxRep + 1) *
                                    (long long)(2 * maxRep + 1) *
                                    (long long)reference.atoms.size();
    if (totalEstimate > 50000000LL)
    {
        std::ostringstream msg;
        msg << "Tiling would generate ~" << totalEstimate
            << " candidate atoms (limit 50M). Reduce box size or use a larger unit cell.";
        result.message = msg.str();
        return result;
    }

    // --- Fill the box -------------------------------------------------------
    std::vector<AtomSite> generatedAtoms;
    generatedAtoms.reserve(std::min((long long)5000000, totalEstimate));

    // Precompute rotated lattice vectors for each grain
    std::vector<glm::vec3> rotA(params.numGrains), rotB(params.numGrains), rotC(params.numGrains);
    std::vector<std::array<float, 3>> grainIPFColors(params.numGrains);
    for (int g = 0; g < params.numGrains; ++g)
    {
        rotA[g] = grainRotations[g] * a;
        rotB[g] = grainRotations[g] * b;
        rotC[g] = grainRotations[g] * c;
        grainIPFColors[g] = computeIPFColor(grainRotations[g]);
    }

    // For each grain, tile the unit cell (rotated) and keep atoms inside the
    // box that belong to this grain's Voronoi region.
    std::vector<int> atomGrainIndex; // parallel to generatedAtoms
    for (int g = 0; g < params.numGrains; ++g)
    {
        const glm::vec3& seed = seeds[g];
        const glm::mat3& rot  = grainRotations[g];

        // Compute per-grain replication range from rotated lattice lengths
        const float rla = glm::length(rotA[g]);
        const float rlb = glm::length(rotB[g]);
        const float rlc = glm::length(rotC[g]);
        const int nA = std::min(100, (int)std::ceil(boxDiag / rla) + 2);
        const int nB = std::min(100, (int)std::ceil(boxDiag / rlb) + 2);
        const int nC = std::min(100, (int)std::ceil(boxDiag / rlc) + 2);

        for (int ia = -nA; ia <= nA; ++ia)
        for (int ib = -nB; ib <= nB; ++ib)
        for (int ic = -nC; ic <= nC; ++ic)
        {
            const glm::vec3 latticeOffset = (float)ia * rotA[g]
                                           + (float)ib * rotB[g]
                                           + (float)ic * rotC[g];

            for (const AtomSite& atom : reference.atoms)
            {
                // Position relative to reference cell origin
                const glm::vec3 refPos((float)atom.x, (float)atom.y, (float)atom.z);

                // Rotate and translate to grain-local frame, then offset to
                // place the grain's origin at its seed position.
                const glm::vec3 pos = seed + rot * refPos + latticeOffset;

                // Test: inside the simulation box?
                if (pos.x < 0.0f || pos.x >= params.sizeX ||
                    pos.y < 0.0f || pos.y >= params.sizeY ||
                    pos.z < 0.0f || pos.z >= params.sizeZ)
                    continue;

                // Test: belongs to this grain (nearest Voronoi seed)?
                if (nearestSeed(pos, seeds) != g)
                    continue;

                AtomSite out = atom;
                out.x = (double)pos.x;
                out.y = (double)pos.y;
                out.z = (double)pos.z;

                int z = out.atomicNumber;
                if (z >= 0 && z < (int)elementColors.size())
                {
                    out.r = elementColors[z].r;
                    out.g = elementColors[z].g;
                    out.b = elementColors[z].b;
                }
                else
                {
                    getDefaultElementColor(z, out.r, out.g, out.b);
                }
                generatedAtoms.push_back(out);
                atomGrainIndex.push_back(g);
            }
        }
    }

    if (generatedAtoms.empty())
    {
        result.message = "No atoms generated. Try increasing the box size or "
                         "check the reference structure.";
        return result;
    }

    // --- Write output -------------------------------------------------------
    result.outputAtoms = (int)generatedAtoms.size();
    structure.atoms.swap(generatedAtoms);
    structure.hasUnitCell  = true;
    structure.cellOffset   = {{ 0.0, 0.0, 0.0 }};
    structure.cellVectors  = {{
        {{ (double)params.sizeX, 0.0, 0.0 }},
        {{ 0.0, (double)params.sizeY, 0.0 }},
        {{ 0.0, 0.0, (double)params.sizeZ }}
    }};

    // Populate per-atom grain orientation colors (IPF-Z) for EBSD-style display
    structure.grainColors.resize(structure.atoms.size());
    for (size_t i = 0; i < structure.atoms.size(); ++i)
        structure.grainColors[i] = grainIPFColors[atomGrainIndex[i]];

    result.success = true;
    std::ostringstream msg;
    msg << "Polycrystal built: " << result.outputAtoms << " atoms, "
        << params.numGrains << " grains.";
    result.message = msg.str();
    return result;
}
