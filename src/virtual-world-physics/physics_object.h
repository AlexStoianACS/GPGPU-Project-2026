#pragma once

#include "bounding_volume.h"
#include "core/gpu/mesh.h"
#include <glm/glm.hpp>

namespace physics {
/**
 * Encapsulates the properties and state of a physical object in the simulation.
 */
struct PhysicsObject {
  // Transform data
  glm::vec3 position;
  glm::vec3 velocity;
  glm::vec3 acceleration;

  // Physical properties
  float mass;
  float friction;
  float restitution;
  // If true, object doesn't move. Think where you may need this.
  bool isStatic;

  // Visual properties
  glm::vec3 color;
  glm::vec3 scale;

  // Bounding volume
  bounding_volume_t boundingVolume;

  // Mesh reference
  Mesh *mesh;

public:
  PhysicsObject()
      : position(0.0f), velocity(0.0f), acceleration(0.0f), mass(1.0f),
        friction(0.05f), restitution(0.8f), isStatic(false), color(1.0f),
        scale(1.0f), boundingVolume(bounding_volume_t()), mesh(nullptr) {}

  PhysicsObject(Mesh *m, bounding_volume_t bv)
      : position(0.0f), velocity(0.0f), acceleration(0.0f), mass(1.0f),
        restitution(0.8f), isStatic(false), color(1.0f), scale(1.0f),
        boundingVolume(bv), mesh(m) {}

  glm::mat4 GetModelMatrix() const;
  void UpdateBoundingVolume();
  void ApplyForce(const glm::vec3 &force);
  void ApplyImpulse(const glm::vec3 &impulse);
  void Integrate(float deltaTime);
};
} // namespace physics
