#include <iostream>
#include "physics_gpu.h"
#include "physics_gpu.cuh"

// Define here kernels and entry points for GPU physics computations.

namespace physics {


bool GPUCollisionDetector::Initialize() {

    int deviceCount = 0;

    cudaError_t error = cudaGetDeviceCount(&deviceCount);

    if (error != cudaSuccess || deviceCount == 0) {
        return false;
    }

    m_initialized = true;

    return true;
}

__global__
void BroadPhaseKernel(const GPUObject* objects, size_t objectCount, CandidatePair* pairs, unsigned int* pairCount) {

    const size_t threadId = blockIdx.x * blockDim.x + threadIdx.x;

    size_t pairIndex = 0;

    for (size_t i = 0; i < objectCount; i++) {

        const size_t pairsForI = objectCount - i - 1;

        if (threadId < pairIndex + pairsForI) {

            const size_t j = i + 1 + (threadId - pairIndex);

            const GPUObject& objectA = objects[i];
            const GPUObject& objectB = objects[j];

            const float radiusA = glm::length(objectA.sizes);
            const float radiusB = glm::length(objectB.sizes);

            const glm::vec3 difference = objectA.center - objectB.center;

            const float distance = glm::length(difference);

            if (distance <= radiusA + radiusB) {

                const unsigned int index = atomicAdd(pairCount, 1);

                pairs[index].indexA = i;
                pairs[index].indexB = j;
            }

            return;
        }

        pairIndex += pairsForI;
    }
}

__global__ void NarrowPhaseKernel(const GPUObject* objects, const CandidatePair* pairs, size_t pairCount, CollisionInfo* collisions) {

    const size_t threadId = blockIdx.x * blockDim.x + threadIdx.x;

    if (threadId >= pairCount) {
        return;
    }

    const CandidatePair pair = pairs[threadId];

    const GPUObject& objectA = objects[pair.indexA];
    const GPUObject& objectB = objects[pair.indexB];

    const glm::vec3 minA = objectA.center - objectA.sizes;
    const glm::vec3 maxA = objectA.center + objectA.sizes;

    const glm::vec3 minB = objectB.center - objectB.sizes;
    const glm::vec3 maxB = objectB.center + objectB.sizes;

    const float overlapX = fminf(maxA.x, maxB.x) - fmaxf(minA.x, minB.x);
    const float overlapY = fminf(maxA.y, maxB.y) - fmaxf(minA.y, minB.y);
    const float overlapZ = fminf(maxA.z, maxB.z) - fmaxf(minA.z, minB.z);

    CollisionInfo collision;

    collision.indexA = pair.indexA;
    collision.indexB = pair.indexB;

    if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f) {

        collisions[threadId] = collision;
        return;
    }

    if (overlapX <= overlapY && overlapX <= overlapZ) {
        collision.penetration = overlapX;

        collision.normal = glm::vec3(objectB.center.x >= objectA.center.x ? 1.0f : -1.0f, 0.0f, 0.0f);

    } else if (overlapY <= overlapZ) {

        collision.penetration = overlapY;

        collision.normal = glm::vec3(0.0f, objectB.center.y >= objectA.center.y ? 1.0f : -1.0f, 0.0f);
    } else {

        collision.penetration = overlapZ;

        collision.normal = glm::vec3(0.0f, 0.0f, objectB.center.z >= objectA.center.z ? 1.0f : -1.0f);
    }

    collision.isValid = true;

    collisions[threadId] = collision;
}

std::vector<CollisionInfo>
GPUCollisionDetector::DetectCollisions(const std::vector<PhysicsObject>& objects) {

    if (objects.empty()) {
        return {};
    }

    std::vector<GPUObject> gpuObjects;
    gpuObjects.reserve(objects.size());

    for (const auto& object : objects) {

        GPUObject gpuObject;

        gpuObject.center = object.boundingVolume.center;
        gpuObject.sizes = object.boundingVolume.sizes;

        gpuObjects.push_back(gpuObject);
    }

    // copy gpuObjects to GPU
    
    GPUObject* d_objects = nullptr;

    cudaMalloc(&d_objects, gpuObjects.size() * sizeof(GPUObject));
    cudaMemcpy(d_objects, gpuObjects.data(), gpuObjects.size() * sizeof(GPUObject), cudaMemcpyHostToDevice);

    // launch Broad Phase
    
    const size_t maxPairs = objects.size() * (objects.size() - 1) / 2;

    CandidatePair* d_pairs = nullptr;
    cudaMalloc(&d_pairs, maxPairs * sizeof(CandidatePair));

    unsigned int* d_pairCount = nullptr;
    cudaMalloc(&d_pairCount, sizeof(unsigned int));
    cudaMemset(d_pairCount, 0, sizeof(unsigned int));

    const int threadsPerBlock = 256;
    const size_t blocks = (maxPairs + threadsPerBlock - 1) / threadsPerBlock;

    BroadPhaseKernel<<<blocks, threadsPerBlock>>>(d_objects, objects.size(), d_pairs, d_pairCount);

    cudaDeviceSynchronize();


    unsigned int pairCount = 0;

    cudaMemcpy(&pairCount, d_pairCount, sizeof(unsigned int), cudaMemcpyDeviceToHost);

    std::vector<CandidatePair> candidatePairs(pairCount);

    if (pairCount > 0) {
        cudaMemcpy(candidatePairs.data(), d_pairs, pairCount * sizeof(CandidatePair), cudaMemcpyDeviceToHost);
    }

    // launch Narrow Phase
    
    CollisionInfo* d_collisions = nullptr;

    cudaMalloc(&d_collisions, pairCount * sizeof(CollisionInfo));

    if (pairCount > 0) {
        const size_t narrowBlocks = (pairCount + threadsPerBlock - 1) / threadsPerBlock;

        NarrowPhaseKernel<<<narrowBlocks, threadsPerBlock>>>(d_objects, d_pairs, pairCount, d_collisions);

        cudaDeviceSynchronize();
    }


    // copy CollisionInfo back to CPU

    std::vector<CollisionInfo> collisions(pairCount);

    if (pairCount > 0) {
        cudaMemcpy(collisions.data(), d_collisions, pairCount * sizeof(CollisionInfo), cudaMemcpyDeviceToHost);
    }

    std::vector<CollisionInfo> validCollisions;

    for (const auto& collision : collisions) {

        if (collision.isValid) {
            validCollisions.push_back(collision);
        }
    }

    cudaFree(d_objects);
    cudaFree(d_pairs);
    cudaFree(d_pairCount);
    cudaFree(d_collisions);

    return validCollisions;
}

} // namespace physics