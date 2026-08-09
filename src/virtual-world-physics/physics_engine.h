#pragma once

#include "physics_object.h"
#include "virtual-world-physics/bounding_volume.h"
#include "virtual-world-physics/physics_gpu.h"
#include <unordered_map>
#include <vector>

namespace physics {

/**
 * Data-oriented physics engine.
 * All physics objects are stored in a single vector for cache locality.
 */
class PhysicsEngine {
public:
  struct Statistics {
    int detectedCollisions{};
    float collisionDetectionTime{};
    int objectCount{};
  };

  PhysicsEngine() = default;
  ~PhysicsEngine();

  void Init(const glm::vec3 &gravity, bool useGpu = false);
  glm::vec3 GetGravity() const { return m_gravity; }
  bool GetGPUMode() const { return m_useGPU; }
  void ToggleGPUMode() { m_useGPU = !m_useGPU; }

  size_t AddObject(const PhysicsObject &object) {
    m_objects.push_back(object);
    return m_objects.size() - 1;
  }

  std::vector<PhysicsObject> &GetObjects() { return m_objects; }
  const std::vector<PhysicsObject> &GetObjects() const { return m_objects; }

  const Statistics &GetStatistics() const { return m_stats; }

  void ClearObjects();
  void Update(float deltaTime);

private:
  void ApplyGravity(float deltaTime);

  // Broad phase: spatial partitioning to eliminate non-colliding pairs
  void BroadPhase();
  std::vector<std::pair<size_t, size_t>> GetPotentialCollisionPairs();

  // Narrow phase: precise collision detection
  void NarrowPhase();

  CollisionInfo DetectCollision(size_t indexA, size_t indexB);
  CollisionInfo ComputeBoxBoxCollision(size_t indexA, size_t indexB,
                                       const bounding_volume_t &boxA,
                                       const bounding_volume_t &boxB);

  void ResolveCollision(size_t indexA, size_t indexB,
                        const CollisionInfo &collision);

private:
  std::vector<PhysicsObject> m_objects;
  glm::vec3 m_gravity{0.0f, -9.81f, 0.0f};
  Statistics m_stats{};
  bool m_useGPU;
  GPUCollisionDetector *m_gpuDetector = nullptr;

  float m_accumulator{0.0f};
  // Repeat 8 times per second to ensure stable simulation, even with variable frame rates
  const float m_fixedDeltaTime{1.0f / 125.0f}; 
  const int m_maxSubSteps{5};                 
};
} // namespace physics
