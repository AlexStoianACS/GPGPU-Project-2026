#include "core/engine.h"
#include "virtual-world-physics/virtual-world-physics.h"
#include <filesystem>

#ifdef _WIN32
PREFER_DISCRETE_GPU_NVIDIA;
PREFER_DISCRETE_GPU_AMD;
#endif

int main(int argc, char **argv) {
  srand((unsigned int)time(NULL));

  WindowProperties wp{};
  wp.selfDir = std::filesystem::absolute(argv[0]).parent_path().string();
  wp.resolution = glm::ivec2(1920, 1080);
  wp.vSync = false;
  wp.name = "virtual-world-physics";

  Engine::Init(wp);

  VirtualWorldPhysics world{};

  world.Init();
  world.Run();

  Engine::Exit();

  return 0;
}
