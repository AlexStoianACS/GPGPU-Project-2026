#pragma once

#include "bounding_volume.h"
#include <glm/glm.hpp>

namespace physics {

inline bool Intersects(const bounding_volume_t &boxA,
                       const bounding_volume_t &boxB) {

    // AABB intersection test

    glm::vec3 minA = boxA.center - boxA.sizes;
    glm::vec3 maxA = boxA.center + boxA.sizes;

    glm::vec3 minB = boxB.center - boxB.sizes;
    glm::vec3 maxB = boxB.center + boxB.sizes;

    return (minA.x <= maxB.x && maxA.x >= minB.x) &&
           (minA.y <= maxB.y && maxA.y >= minB.y) &&
           (minA.z <= maxB.z && maxA.z >= minB.z);
}

} // namespace physics
