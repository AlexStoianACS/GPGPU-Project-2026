#pragma once

#include "components/simple_scene.h"
#include "components/text_renderer.h"
#include "physics_engine.h"
#include "geometry_generator.h"

struct MaterialProperties
{
    unsigned int shininess;
    float        kd;
    float        ks;
};

class VirtualWorldPhysics : public gfxc::SimpleScene
{
public:
    VirtualWorldPhysics()           = default;
    ~VirtualWorldPhysics() override = default;

    void Init() override;

private:
    void FrameStart() override;
    void Update(float deltaTimeSeconds) override;
    void FrameEnd() override;

    void RenderInstancedMesh(Mesh* mesh, Shader* shader, const std::vector<glm::mat4>& modelMatrices,
            const std::vector<glm::vec3>& colors, const MaterialProperties& material);

    void OnInputUpdate(float deltaTime, int mods) override;
    void OnKeyPress(int key, int mods) override;
    void OnKeyRelease(int key, int mods) override;
    void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
    void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
    void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
    void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
    void OnWindowResize(int width, int height) override;

    void CreateBoundaryWalls();
    void SpawnObjects(int nBoxes, float sizeScale = 1.0f);
    void ResetScenario();
    void RenderStats();

private:
    physics::PhysicsEngine physicsEngine;
    gfxc::TextRenderer*    textRenderer{};

    GLuint instanceVBO_modelMatrix{};
    GLuint instanceVBO_color{};

    glm::vec3 lightPosition{ 0.0f, 10.0f, 5.0f };
    glm::vec3 lightDirection{ 0.0f, -1.0f, 0.0f };

    MaterialProperties boxMaterial{ 30, 0.7f, 0.5f };
    MaterialProperties wallMaterial{ 20, 0.5f, 0.3f };

    static constexpr int   kObjectCount{ 5000 };
    static constexpr float kObjectSizeScale{ .5f };
    bool                    simulationPaused{ false };

    float                    textUpdateTimer{ 0.0f };
    const float              textUpdateInterval{ 0.1f };
    std::vector<std::string> cachedTextLines;
};
