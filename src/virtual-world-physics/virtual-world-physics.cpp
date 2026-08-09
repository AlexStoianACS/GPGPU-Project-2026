#include "virtual-world-physics/virtual-world-physics.h"
#include "components/transform.h"
#include "core/managers/resource_path.h"
#include "virtual-world-physics/bounding_volume.h"

#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

void VirtualWorldPhysics::Init() {

  // Generate procedural meshes
  {
    Mesh *boxMesh = physics::GeometryGenerator::CreateBox("proc_box", 1.0f, 1.0f, 1.0f);
    meshes[boxMesh->GetMeshID()] = boxMesh;
  }
  {
    Mesh *planeMesh = physics::GeometryGenerator::CreatePlane("proc_plane", 1.0f, 1.0f);
    meshes[planeMesh->GetMeshID()] = planeMesh;
  }

  // Load shader - In order to raster the scene, we need a shader that can handle instanced rendering and flat shading with Phong lighting.
  // If you don't know what this means, don't worry, you will learn about it in the graphics course. We don't need this for the GPGPU course. :)
  {
    auto *instancedFlatShader = new Shader("InstancedFlatPhong");
    instancedFlatShader->AddShader(
        PATH_JOIN(window->props.selfDir, SOURCE_PATH::VIRTUAL_WORLD_PHYSICS,
                  "shaders", "InstancedFlatVertexShader.glsl"), GL_VERTEX_SHADER);
    instancedFlatShader->AddShader(
        PATH_JOIN(window->props.selfDir, SOURCE_PATH::VIRTUAL_WORLD_PHYSICS,
                  "shaders", "InstancedFlatFragmentShader.glsl"), GL_FRAGMENT_SHADER);
    instancedFlatShader->CreateAndLink();
    shaders[instancedFlatShader->GetName()] = instancedFlatShader;
  }

  // Setup physics - It is a lot better to modularize the physics engine, so we can use it in other projects.
  physicsEngine.Init(glm::vec3(0.0f, -9.81f, 0.0f), true);

  glGenBuffers(1, &instanceVBO_modelMatrix);
  glGenBuffers(1, &instanceVBO_color);

  ResetScenario();

  auto resolution = window->GetResolution();
  textRenderer = new gfxc::TextRenderer(window->props.selfDir, resolution.x, resolution.y);
  textRenderer->Load(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::FONTS, "Hack-Bold.ttf"), 18);

  // Position camera to view the scene from the front (+Z direction)
  auto *camera = GetSceneCamera();
  camera->SetPosition(glm::vec3(0.0f, 7.5f,20.0f)); 
  camera->RotateOX(15.0f);

  glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);

  ToggleGroundPlane();
}

void VirtualWorldPhysics::FrameStart() {
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glm::ivec2 resolution = window->GetResolution();
  glViewport(0, 0, resolution.x, resolution.y);
}

// Update function is the one called every frame, and it is where we update the physics engine and render the scene.
void VirtualWorldPhysics::Update(float deltaTimeSeconds) {
 
  // Update physics
  if (!simulationPaused) {
    physicsEngine.Update(deltaTimeSeconds);
  }

  // Update text timer
  textUpdateTimer += deltaTimeSeconds;

  std::vector<glm::mat4> boxMatrices, wallMatrices;
  std::vector<glm::vec3> boxColors, wallColors;

  for (const auto &obj : physicsEngine.GetObjects()) {
    // Skip invisible objects (marked with negative color components)
    if (obj.color.r < 0.0f || obj.color.g < 0.0f || obj.color.b < 0.0f) {
      continue;
    }

    glm::mat4 modelMatrix = obj.GetModelMatrix();

    if (obj.isStatic) {
      wallMatrices.push_back(modelMatrix);
      wallColors.push_back(obj.color);
    } else {
      boxMatrices.push_back(modelMatrix);
      boxColors.push_back(obj.color);
    }
  }

  // Render calls
  if (!wallMatrices.empty()) {
    RenderInstancedMesh(meshes["proc_box"], shaders["InstancedFlatPhong"], wallMatrices, wallColors, wallMaterial);
  }
  if (!boxMatrices.empty()) {
    RenderInstancedMesh(meshes["proc_box"], shaders["InstancedFlatPhong"], boxMatrices, boxColors, boxMaterial);
  }
}

void VirtualWorldPhysics::FrameEnd() { RenderStats(); }

void VirtualWorldPhysics::CreateBoundaryWalls() {
  float worldSize = 20.0f;
  float wallThickness = 1.0f;
  float wallHeight = 15.0f;

  Mesh *boxMesh = meshes["proc_box"];
  float halfWorldSize = worldSize * 0.5f;
  float halfThickness = wallThickness * 0.5f;

  // Floor
  {
    physics::PhysicsObject floor;
    floor.mesh = boxMesh;
    floor.position = glm::vec3(0.0f, -halfThickness, 0.0f);
    floor.scale = glm::vec3(worldSize, wallThickness, worldSize);
    floor.color = glm::vec3(0.3f, 0.3f, 0.3f);
    floor.isStatic = true;
    floor.boundingVolume = {
        floor.position,
        (glm::vec3(worldSize, wallThickness, worldSize) + 0.2f) * 0.5f};

    floor.UpdateBoundingVolume();
    physicsEngine.AddObject(floor);
  }

  // Ceiling
  {
    physics::PhysicsObject ceiling;
    ceiling.position = glm::vec3(0.0f, wallHeight + halfThickness, 0.0f);
    ceiling.scale = glm::vec3(worldSize, wallThickness, worldSize);
    ceiling.boundingVolume = {
        ceiling.position,
        (glm::vec3(worldSize, wallThickness, worldSize) + 0.2f) * 0.5f};
    ceiling.mesh = boxMesh;
    ceiling.color = glm::vec3(0.3f, 0.3f, 0.3f);
    ceiling.isStatic = true;
    ceiling.UpdateBoundingVolume();
    physicsEngine.AddObject(ceiling);
  }

  // Front wall (+Z) - invisible
  {
    physics::PhysicsObject wall;
    wall.position = glm::vec3(0.0f, wallHeight * 0.5f,
                              halfWorldSize + halfThickness - 0.1f);
    wall.scale = glm::vec3(worldSize, wallHeight, wallThickness);
    wall.boundingVolume = {
        wall.position,
        (glm::vec3(worldSize, wallHeight, wallThickness) + 0.2f) * 0.5f};
    wall.mesh = boxMesh;
    wall.color = glm::vec3(-1.0f, -1.0f, -1.0f); // invisible
    wall.isStatic = true;
    wall.UpdateBoundingVolume();
    physicsEngine.AddObject(wall);
  }

  // Back wall (-Z)
  {
    physics::PhysicsObject wall;
    wall.position =
        glm::vec3(0.0f, wallHeight * 0.5f, -halfWorldSize - halfThickness);
    wall.scale = glm::vec3(worldSize, wallHeight, wallThickness);
    wall.boundingVolume = {
        wall.position,
        (glm::vec3(worldSize, wallHeight, wallThickness) + 0.2f) * 0.5f};
    wall.mesh = boxMesh;
    wall.color = glm::vec3(0.3f, 0.3f, 0.3f);
    wall.isStatic = true;
    wall.UpdateBoundingVolume();
    physicsEngine.AddObject(wall);
  }

  // Right wall (+X)
  {
    physics::PhysicsObject wall;
    wall.position =
        glm::vec3(halfWorldSize + halfThickness, wallHeight * 0.5f, 0.0f);
    wall.scale = glm::vec3(wallThickness, wallHeight, worldSize);
    wall.boundingVolume = {
        wall.position,
        (glm::vec3(wallThickness, wallHeight, worldSize) + 0.2f) * 0.5f};
    wall.mesh = boxMesh;
    wall.color = glm::vec3(0.3f, 0.3f, 0.3f);
    wall.isStatic = true;
    wall.UpdateBoundingVolume();
    physicsEngine.AddObject(wall);
  }

  // Left wall (-X)
  {
    physics::PhysicsObject wall;
    wall.position =
        glm::vec3(-halfWorldSize - halfThickness, wallHeight * 0.5f, 0.0f);
    wall.scale = glm::vec3(wallThickness, wallHeight, worldSize);
    wall.boundingVolume = {
        wall.position,
        (glm::vec3(wallThickness, wallHeight, worldSize) + 0.2f) * 0.5f};
    wall.mesh = boxMesh;
    wall.color = glm::vec3(0.3f, 0.3f, 0.3f);
    wall.isStatic = true;
    wall.UpdateBoundingVolume();
    physicsEngine.AddObject(wall);
  }
}

void VirtualWorldPhysics::SpawnObjects(int nBoxes, float sizeScale) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> posDistX(-8.0f, 8.0f);
  std::uniform_real_distribution<float> posDistY(2.0f, 12.0f);
  std::uniform_real_distribution<float> posDistZ(-8.0f, 8.0f);
  std::uniform_real_distribution<float> velDist(-3.0f, 3.0f);
  std::uniform_real_distribution<float> sizeDist(0.5f, 1.5f);
  std::uniform_real_distribution<float> colorDist(0.3f, 1.0f);
  std::uniform_real_distribution<float> massDist(0.5f, 2.0f);
  std::uniform_real_distribution<float> frictionDist(0.1f, 0.9f);
  std::uniform_real_distribution<float> restitutionDist(0.3f, 0.9f);

  for (int i = 0; i < nBoxes; ++i) {
    physics::PhysicsObject obj;

    obj.position = glm::vec3(posDistX(gen), posDistY(gen), posDistZ(gen));
    obj.velocity = glm::vec3(velDist(gen), velDist(gen), velDist(gen));
    obj.color = glm::vec3(colorDist(gen), colorDist(gen), colorDist(gen));
    obj.mass = massDist(gen);
    obj.friction = frictionDist(gen);
    obj.restitution = restitutionDist(gen);
    obj.isStatic = false;

    float size = sizeDist(gen) * sizeScale;
    obj.mesh = meshes["proc_box"];
    obj.scale = glm::vec3(size);
    obj.boundingVolume = {obj.position, glm::vec3(size * 0.5f)};

    obj.UpdateBoundingVolume();
    physicsEngine.AddObject(obj);
  }
}

void VirtualWorldPhysics::ResetScenario() {
  physicsEngine.ClearObjects();
  CreateBoundaryWalls();
  SpawnObjects(kObjectCount, kObjectSizeScale);
}

void VirtualWorldPhysics::RenderStats() {
  static const float lineHeight = 25.0f;
  static const glm::vec3 textColor = glm::vec3(0.9f, 0.9f, 0.9f);

  // Update cached text out of sync with the framerate
  if (textUpdateTimer >= textUpdateInterval) {
    cachedTextLines.clear();

    double frameTime = GetLastFrameTime();
    double fps = frameTime > 0.0 ? 1.0 / frameTime : 0.0;
    const auto &stats = physicsEngine.GetStatistics();

    char buffer[64];

    cachedTextLines.push_back("=== Performance Stats ===");

    snprintf(buffer, sizeof(buffer), "FPS: %.1f (%.2f ms)", fps,
             frameTime * 1000.0);
    cachedTextLines.push_back(std::string(buffer));

    snprintf(buffer, sizeof(buffer), "Objects: %d", stats.objectCount);
    cachedTextLines.push_back(std::string(buffer));

    cachedTextLines.push_back("Collisions Detected: " +
                              std::to_string(stats.detectedCollisions));

    snprintf(buffer, sizeof(buffer), "Physics Time: %.4f ms",
             stats.collisionDetectionTime);
    cachedTextLines.push_back(std::string(buffer));

    cachedTextLines.push_back("");
    cachedTextLines.push_back("Controls: R=Reset Space=Pause G=Toggle GPU");

    if (simulationPaused) {
      cachedTextLines.push_back("PAUSED");
    }

    textUpdateTimer -= textUpdateInterval;
  }

  // Render cached text every frame
  float yPos = 10.0f;
  for (size_t i = 0; i < cachedTextLines.size(); ++i) {
    if (cachedTextLines[i].empty()) {
      yPos += lineHeight * 0.5f;
    } else if (i == cachedTextLines.size() - 1 && simulationPaused) {
      textRenderer->RenderText(cachedTextLines[i], 10.0f, yPos, 1.0f,
                               glm::vec3(1.0f, 0.5f, 0.0f));
      yPos += lineHeight;
    } else if (cachedTextLines[i] == "Controls: R=Reset Space=Pause G=Toggle GPU") {
      textRenderer->RenderText(cachedTextLines[i], 10.0f, yPos, 0.8f,
                               glm::vec3(0.7f, 0.7f, 0.7f));
      yPos += lineHeight;
    } else {
      textRenderer->RenderText(cachedTextLines[i], 10.0f, yPos, 1.0f,
                               textColor);
      yPos += lineHeight;
    }
  }
}

// OPENGL instanced rendering function. No need to understand this for the GPGPU course. :)
void VirtualWorldPhysics::RenderInstancedMesh(
    Mesh *mesh, Shader *shader, const std::vector<glm::mat4> &modelMatrices,
    const std::vector<glm::vec3> &colors, const MaterialProperties &material) {
  if (!mesh || !shader || !shader->GetProgramID() || modelMatrices.empty())
    return;

  auto instanceCount = static_cast<int>(modelMatrices.size());

  glUseProgram(shader->program);

  int light_position = glGetUniformLocation(shader->program, "light_position");
  glUniform3f(light_position, lightPosition.x, lightPosition.y,
              lightPosition.z);

  int light_direction =
      glGetUniformLocation(shader->program, "light_direction");
  glUniform3f(light_direction, lightDirection.x, lightDirection.y,
              lightDirection.z);

  glm::vec3 eyePosition = GetSceneCamera()->m_transform->GetWorldPosition();
  int eye_position = glGetUniformLocation(shader->program, "eye_position");
  glUniform3f(eye_position, eyePosition.x, eyePosition.y, eyePosition.z);

  int material_shininess =
      glGetUniformLocation(shader->program, "material_shininess");
  glUniform1i(material_shininess, material.shininess);

  int material_kd = glGetUniformLocation(shader->program, "material_kd");
  glUniform1f(material_kd, material.kd);

  int material_ks = glGetUniformLocation(shader->program, "material_ks");
  glUniform1f(material_ks, material.ks);

  glm::mat4 viewMatrix = GetSceneCamera()->GetViewMatrix();
  int loc_view_matrix = glGetUniformLocation(shader->program, "View");
  glUniformMatrix4fv(loc_view_matrix, 1, GL_FALSE, glm::value_ptr(viewMatrix));

  glm::mat4 projectionMatrix = GetSceneCamera()->GetProjectionMatrix();
  int loc_projection_matrix =
      glGetUniformLocation(shader->program, "Projection");
  glUniformMatrix4fv(loc_projection_matrix, 1, GL_FALSE,
                     glm::value_ptr(projectionMatrix));

  glBindVertexArray(mesh->GetBuffers()->m_VAO);

  glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_modelMatrix);
  glBufferData(GL_ARRAY_BUFFER, instanceCount * sizeof(glm::mat4),
               modelMatrices.data(), GL_DYNAMIC_DRAW);

  for (int i = 0; i < 4; ++i) {
    glEnableVertexAttribArray(3 + i);
    glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                          (void *)(sizeof(glm::vec4) * i));
    glVertexAttribDivisor(3 + i, 1); // Advance once per instance
  }

  glBindBuffer(GL_ARRAY_BUFFER, instanceVBO_color);
  glBufferData(GL_ARRAY_BUFFER, instanceCount * sizeof(glm::vec3),
               colors.data(), GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(7);
  glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
  glVertexAttribDivisor(7, 1); // Advance once per instance

  glDrawElementsInstanced(mesh->GetDrawMode(),
                          static_cast<int>(mesh->indices.size()),
                          GL_UNSIGNED_INT, nullptr, instanceCount);

  glVertexAttribDivisor(3, 0);
  glVertexAttribDivisor(4, 0);
  glVertexAttribDivisor(5, 0);
  glVertexAttribDivisor(6, 0);
  glVertexAttribDivisor(7, 0);
  glBindVertexArray(0);
}

/*
 *  These are callback functions. To find more about callbacks and
 *  how they behave, see `input_controller.h`.
 */

void VirtualWorldPhysics::OnInputUpdate(float deltaTime, int mods) {
  // No custom camera movement; free-fly camera controls are handled by
  // gfxc::SimpleScene's default input handling.
}

void VirtualWorldPhysics::OnKeyPress(int key, int mods) {
  if (key == GLFW_KEY_SPACE) {
    simulationPaused = !simulationPaused;
    std::cout << "Simulation " << (simulationPaused ? "PAUSED" : "RESUMED")
              << std::endl;
  }

  if (key == GLFW_KEY_R) {
    std::cout << "Resetting simulation..." << std::endl;
    ResetScenario();
  }

  if (key == GLFW_KEY_G) {
    physicsEngine.ToggleGPUMode();
    std::cout << "GPU Mode: " << (physicsEngine.GetGPUMode() ? "true" : "false") << std::endl;
  }
}

void VirtualWorldPhysics::OnKeyRelease(int key, int mods) {}

void VirtualWorldPhysics::OnMouseMove(int mouseX, int mouseY, int deltaX,
                                      int deltaY) {}

void VirtualWorldPhysics::OnMouseBtnPress(int mouseX, int mouseY, int button,
                                          int mods) {}

void VirtualWorldPhysics::OnMouseBtnRelease(int mouseX, int mouseY, int button,
                                            int mods) {}

void VirtualWorldPhysics::OnMouseScroll(int mouseX, int mouseY, int offsetX,
                                        int offsetY) {}

void VirtualWorldPhysics::OnWindowResize(int width, int height) {}
