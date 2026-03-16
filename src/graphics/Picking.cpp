#include "Picking.h"

#include <glm/gtc/matrix_transform.hpp>

glm::vec3 pickRayDir(double mx, double my, int w, int h,
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

int pickAtom(const glm::vec3& origin, const glm::vec3& dir,
             const std::vector<glm::vec3>& positions,
             const std::vector<float>& radii,
             float fallbackRadius)
{
    int   best  = -1;
    float bestT = 1e30f;

    for (int i = 0; i < (int)positions.size(); ++i)
    {
        glm::vec3 oc = positions[i] - origin;
        float t = glm::dot(oc, dir);
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
