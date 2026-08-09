#include "physics_object.h"
#include <glm/gtc/matrix_transform.hpp>

namespace physics {
glm::mat4 PhysicsObject::GetModelMatrix() const {
  glm::mat4 modelMatrix = glm::mat4(1.0f);
  modelMatrix = glm::translate(modelMatrix, position);
  modelMatrix = glm::scale(modelMatrix, scale);
  return modelMatrix;
}

void PhysicsObject::UpdateBoundingVolume() {
  // TODO
}

void PhysicsObject::ApplyForce(const glm::vec3 &force) {
  // TODO
}

void PhysicsObject::ApplyImpulse(const glm::vec3 &impulse) {
  // TODO
}

void PhysicsObject::Integrate(float deltaTime) {
  // TODO
}
} // namespace physics
