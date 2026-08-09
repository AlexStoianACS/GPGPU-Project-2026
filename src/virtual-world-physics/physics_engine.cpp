#include "physics_engine.h"
#include "virtual-world-physics/bounding_volume.h"
#include "virtual-world-physics/collision.h"
#include "virtual-world-physics/physics_gpu.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <stdexcept>

namespace physics {
void PhysicsEngine::ClearObjects() { m_objects.clear(); }

void PhysicsEngine::Update(float deltaTime) {
  // Reset statistics
  m_stats.detectedCollisions = 0;
  m_stats.objectCount = static_cast<int>(m_objects.size());

  auto startTime = std::chrono::high_resolution_clock::now();

  // Fixed timestep with accumulator
  m_accumulator += deltaTime;
  int subSteps = 0;
  while (m_accumulator >= m_fixedDeltaTime && subSteps < m_maxSubSteps) {
    ApplyGravity(m_fixedDeltaTime);
    for (auto &object : m_objects) {
      object.Integrate(m_fixedDeltaTime);
    }
    if (m_useGPU && m_gpuDetector) {
      try {
        auto collisions = m_gpuDetector->DetectCollisions(m_objects);

        m_stats.detectedCollisions = collisions.size();

        for (const auto &collision : collisions) {
          ResolveCollision(collision.indexA, collision.indexB, collision);
        }
      } catch (const std::exception &ex) {
        std::cerr << "CUDA collision backend failed, falling back to CPU: "
                  << ex.what() << std::endl;
        m_useGPU = false;
        BroadPhase();
        NarrowPhase();
      }
    } else {
      BroadPhase();
      NarrowPhase();
    }

    m_accumulator -= m_fixedDeltaTime;
    subSteps++;
  }
  if (m_accumulator > m_fixedDeltaTime) {
    m_accumulator = 0.0f;
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  m_stats.collisionDetectionTime =
      std::chrono::duration<float, std::milli>(endTime - startTime).count();
}

void PhysicsEngine::ApplyGravity(float deltaTime) {

    for (auto& object : m_objects) {
        object.ApplyForce(object.mass * m_gravity);
    }
}

void PhysicsEngine::BroadPhase() {

    m_potentialCollisionPairs = GetPotentialCollisionPairs();
}


std::vector<std::pair<size_t, size_t>>
PhysicsEngine::GetPotentialCollisionPairs() {

    std::vector<std::pair<size_t, size_t>> pairs;

    for (size_t i = 0; i < m_objects.size(); i++) {
        for (size_t j = i + 1; j < m_objects.size(); j++) {

            const auto& objectA = m_objects[i];
            const auto& objectB = m_objects[j];

            // skip room sides collisions (floor, celling, walls)
            if (objectA.isStatic && objectB.isStatic) {
                continue;
            }

            // sphere collision: ||centerA - centerB|| <= radiusA + radiusB

            const float radiusA = glm::length(objectA.boundingVolume.sizes);
            const float radiusB = glm::length(objectB.boundingVolume.sizes);

            const glm::vec3 difference = objectA.boundingVolume.center - objectB.boundingVolume.center;

            const float distance = glm::length(difference);
            const float radiusSum = radiusA + radiusB;

            if (distance <= radiusSum) {
                pairs.emplace_back(i, j);
            }
        }
    }

    return pairs;
}

void PhysicsEngine::NarrowPhase() {

    for (const auto& pair : m_potentialCollisionPairs) {

        const size_t indexA = pair.first;
        const size_t indexB = pair.second;

        CollisionInfo collision = DetectCollision(indexA, indexB);

        if (!collision.isValid) {
            continue;
        }

        m_stats.detectedCollisions++;

        ResolveCollision(indexA, indexB, collision);
    }
}

CollisionInfo PhysicsEngine::DetectCollision(size_t indexA, size_t indexB) {

    const auto& objectA = m_objects[indexA];
    const auto& objectB = m_objects[indexB];

    const auto& boxA = objectA.boundingVolume;
    const auto& boxB = objectB.boundingVolume;

    if (!Intersects(boxA, boxB)) {
        // default constructor assumes no collision
        return CollisionInfo{};
    }

    return ComputeBoxBoxCollision(indexA, indexB, boxA, boxB);
}

CollisionInfo
PhysicsEngine::ComputeBoxBoxCollision(size_t indexA, size_t indexB,
                                      const bounding_volume_t &boxA,
                                      const bounding_volume_t &boxB) {

    CollisionInfo collision;

    collision.indexA = indexA;
    collision.indexB = indexB;

    glm::vec3 minA = boxA.center - boxA.sizes;
    glm::vec3 maxA = boxA.center + boxA.sizes;

    glm::vec3 minB = boxB.center - boxB.sizes;
    glm::vec3 maxB = boxB.center + boxB.sizes;

    float overlapX = std::min(maxA.x, maxB.x) - std::max(minA.x, minB.x);
    float overlapY = std::min(maxA.y, maxB.y) - std::max(minA.y, minB.y);
    float overlapZ = std::min(maxA.z, maxB.z) - std::max(minA.z, minB.z);

    // find the axis with the smallest overlap
    if (overlapX <= overlapY && overlapX <= overlapZ) {

        collision.penetration = overlapX;
        collision.normal = glm::vec3(boxB.center.x >= boxA.center.x ? 1.0f : -1.0f, 0.0f, 0.0f);

    } else if (overlapY <= overlapZ) {

        collision.penetration = overlapY;
        collision.normal = glm::vec3(0.0f, boxB.center.y >= boxA.center.y ? 1.0f : -1.0f, 0.0f);

    } else {

        collision.penetration = overlapZ;
        collision.normal = glm::vec3(0.0f, 0.0f, boxB.center.z >= boxA.center.z ? 1.0f : -1.0f);
    }

    collision.isValid = true;

    return collision;
}

void PhysicsEngine::ResolveCollision(size_t indexA, size_t indexB,
                                     const CollisionInfo &collision) {

    if (!collision.isValid) {
        return;
    }

    PhysicsObject& objectA = m_objects[indexA];
    PhysicsObject& objectB = m_objects[indexB];

    const glm::vec3 normal = collision.normal;

    // static objects have infinite mass
    const float inverseMassA = objectA.isStatic ? 0.0f : 1.0f / objectA.mass;
    const float inverseMassB = objectB.isStatic ? 0.0f : 1.0f / objectB.mass;
    const float totalInverseMass = inverseMassA + inverseMassB;

    // both objects are static
    if (totalInverseMass == 0.0f) {
        return;
    }

    // remove overlap

    const glm::vec3 correction = normal * (collision.penetration / totalInverseMass);

    if (!objectA.isStatic) {
        objectA.position -= correction * inverseMassA;
    }

    if (!objectB.isStatic) {
        objectB.position += correction * inverseMassB;
    }

    objectA.UpdateBoundingVolume();
    objectB.UpdateBoundingVolume();

    // calculate relative velocity

    const glm::vec3 relativeVelocity = objectB.velocity - objectA.velocity;

    const float velocityAlongNormal = glm::dot(relativeVelocity, normal);

    // objects are already moving apart
    if (velocityAlongNormal >= 0.0f) {
        return;
    }

    // calculate collision impulse

    const float restitution = std::min(objectA.restitution, objectB.restitution);

    const float impulseMagnitude = -(1.0f + restitution) * velocityAlongNormal / totalInverseMass;

    const glm::vec3 impulse = impulseMagnitude * normal;


    // apply impulse

    if (!objectA.isStatic) {
        objectA.velocity -= impulse * inverseMassA;
    }

    if (!objectB.isStatic) {
        objectB.velocity += impulse * inverseMassB;
    }
}

void PhysicsEngine::Init(const glm::vec3 &gravity, bool useGpu) {
  m_gravity = gravity;
  m_useGPU = false;

  delete m_gpuDetector;
  m_gpuDetector = nullptr;

  if (useGpu) {
    m_gpuDetector = new GPUCollisionDetector();
    if (m_gpuDetector->Initialize()) {
      m_useGPU = true;
    } else {
      delete m_gpuDetector;
      m_gpuDetector = nullptr;
      std::cerr << "CUDA collision backend unavailable; using CPU backend."
                << std::endl;
    }
  }
}

PhysicsEngine::~PhysicsEngine() { delete m_gpuDetector; }
} // namespace physics
