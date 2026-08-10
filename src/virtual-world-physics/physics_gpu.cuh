#pragma once

// Define here functions and auxiliaries for GPU physics computations.

namespace physics {

struct GPUObject {
    glm::vec3 center;
    glm::vec3 sizes;
};

struct CandidatePair {
    size_t indexA;
    size_t indexB;
};

__global__
    void BroadPhaseKernel(const GPUObject* objects, size_t objectCount, CandidatePair* pairs, unsigned int* pairCount);

__global__
void NarrowPhaseKernel(const GPUObject* objects, const CandidatePair* pairs, size_t pairCount, CollisionInfo* collisions);

} // namespace physics