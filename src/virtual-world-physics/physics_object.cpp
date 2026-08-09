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

	boundingVolume.center = position;
}

void PhysicsObject::ApplyForce(const glm::vec3 &force) {

	if (isStatic) {
		return;
	}

    // F = m * a
	acceleration += force / mass;
}

void PhysicsObject::ApplyImpulse(const glm::vec3 &impulse) {

	if (isStatic) {
		return;
	}

    // J = m * v

	velocity += impulse / mass;
}

void PhysicsObject::Integrate(float deltaTime) {

	if (isStatic) {
		return;
	}

	// v = v0 + a * t
	velocity += acceleration * deltaTime;

	// p = v * t
	position += velocity * deltaTime;

	// reset for next step
	acceleration = glm::vec3(0.0f);

	// update position of bounding volume, as well
	UpdateBoundingVolume();
}
} // namespace physics
