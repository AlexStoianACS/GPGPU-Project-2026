/*
  * Bounding volume representation for physics computations.
  * Simple axis-aligned bounding box (AABB) representation, defined by a center point and half-extents along each axis.
  * You may also use Oriented Bounding Boxes (OBB) or other bounding volume types, if you want. :)
 */
#pragma once

#include <glm/glm.hpp>

namespace physics {

typedef struct bounding_volume_t_ {
  glm::vec3 center{};
  glm::vec3 sizes{0.5f};
} bounding_volume_t;

} // namespace physics
