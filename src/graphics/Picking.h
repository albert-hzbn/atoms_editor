#pragma once

#include <glm/glm.hpp>
#include <vector>

// Unproject a window-space cursor position into a world-space ray direction.
glm::vec3 pickRayDir(double mx, double my, int w, int h,
                     const glm::mat4& projection,
                     const glm::mat4& view);

// Ray–sphere intersection test against a list of atom positions.
// Returns the index of the nearest hit atom, or -1 if none.
// Per-instance radii are taken from 'radii'; 'fallbackRadius' is used when
// the radii vector is too short or the stored value is non-positive.
int pickAtom(const glm::vec3& origin, const glm::vec3& dir,
             const std::vector<glm::vec3>& positions,
             const std::vector<float>& radii,
             float fallbackRadius);
