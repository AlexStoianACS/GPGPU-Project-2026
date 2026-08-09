#pragma once

#include "core/gpu/mesh.h"
#include <glm/glm.hpp>
#include <string>

namespace physics
{
    /**
     * Utility class for generating procedural geometry.
     */
    class GeometryGenerator
    {
    public:
        static Mesh* CreateBox(const std::string& name, float width = 1.0f, float height = 1.0f, float depth = 1.0f);
        static Mesh* CreatePlane(const std::string& name, float width = 1.0f, float height = 1.0f);
    };
}  // namespace physics
