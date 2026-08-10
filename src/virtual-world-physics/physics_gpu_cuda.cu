#include "physics_gpu.h"
#include "physics_gpu.cuh"

// Define here kernels and entry points for GPU physics computations.

namespace physics {

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

    cudaFree(d_objects);
    cudaFree(d_pairs);
    cudaFree(d_pairCount);

    // TODO: launch Narrow Phase
    // TODO: copy CollisionInfo back to CPU

    return {};
}

} // namespace physics