#pragma once

#include <cstddef>
#include <vector>
#include <glm/glm.hpp>

#include "physics_object.h"

namespace physics {

	typedef struct CollisionInfo_ {
		bool isValid{ false };
		size_t indexA{ 0 };
		size_t indexB{ 0 };
		glm::vec3 normal{ 0.0f };
		float penetration{ 0.0f };
	} CollisionInfo;

	class GPUCollisionDetector {
	public:
		GPUCollisionDetector() = default;
		~GPUCollisionDetector() = default;

		bool Initialize();

		std::vector<CollisionInfo> DetectCollisions(const std::vector<PhysicsObject>& objects); // TODO: implement in physics_gpu_cuda.cu

	private:
		bool m_initialized{ false };
	};

} // namespace physics