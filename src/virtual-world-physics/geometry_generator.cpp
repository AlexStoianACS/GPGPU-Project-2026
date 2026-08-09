/* Implementation of the geometry generator 
 * This file does OPENGL geometry generation for simple shapes, such as boxes and planes.
 * If it looks familiar to you, congrats, it's because you have done the graphics course before,
 * and know how to represent objects in a simple rasterization pipeline.
 * If it doesn't look familiar, don't worry, you will learn about it in the graphics course. We don't
 * need this for the GPGPU course. :)
 */
#include "geometry_generator.h"

namespace physics
{
    Mesh* GeometryGenerator::CreateBox(const std::string& name, float width, float height, float depth)
    {
        auto* mesh = new Mesh(name);

        float w = width * 0.5f;
        float h = height * 0.5f;
        float d = depth * 0.5f;

        std::vector<glm::vec3> vertices = { { -w, -h, d }, { w, -h, d }, { w, h, d }, { -w, h, d }, { -w, -h, -d },
            { w, -h, -d }, { w, h, -d }, { -w, h, -d } };
        std::vector<glm::vec3> normals
                = { { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
                      { -1.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } };
        std::vector<unsigned int> indices = { // Front face (+Z)
            0, 1, 2, 0, 2, 3,
            // Back face (-Z)
            7, 6, 5, 7, 5, 4,
            // Right face (+X)
            1, 5, 6, 1, 6, 2,
            // Left face (-X)
            4, 0, 3, 4, 3, 7,
            // Top face (+Y)
            3, 2, 6, 3, 6, 7,
            // Bottom face (-Y)
            5, 1, 0, 5, 0, 4
        };

        mesh->SetDrawMode(GL_TRIANGLES);
        mesh->InitFromData(vertices, normals, indices);
        return mesh;
    }

    Mesh* GeometryGenerator::CreatePlane(const std::string& name, float width, float height)
    {
        auto* mesh = new Mesh(name);

        float w = width * 0.5f;
        float h = height * 0.5f;

        std::vector<glm::vec3>    positions = { { -w, 0, -h }, { w, 0, -h }, { w, 0, h }, { -w, 0, h } };
        std::vector<glm::vec3>    normals(4, { 0.0f, 1.0f, 0.0f });
        std::vector<unsigned int> indices = { 0, 1, 2, 0, 2, 3 };

        mesh->SetDrawMode(GL_TRIANGLES);
        mesh->InitFromData(positions, normals, indices);
        return mesh;
    }
}  // namespace physics
