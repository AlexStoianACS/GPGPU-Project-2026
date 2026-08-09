#include "core/engine.h"

#include <iostream>

#include "core/managers/texture_manager.h"
#include "utils/gl_utils.h"


WindowObject* Engine::window = nullptr;


WindowObject* Engine::Init(const WindowProperties & props)
{
    /* Initialize the library */
    if (!glfwInit())
        exit(0);

    window = new WindowObject(props);

    glewExperimental = true;
    GLenum err = glewInit();
    
    // Allow GLEW_ERROR_NO_GLX_DISPLAY (4) on Wayland/EGL
    // Issue: https://github.com/thliebig/AppCSXCAD/issues/11
    if (GLEW_OK != err && err != 4)
    {
        // Serious problem
        fprintf(stderr, "GLEW Error %u: %s\n", err, glewGetErrorString(err));
        exit(1);
    }

    TextureManager::Init(window->props.selfDir);

    return window;
}


WindowObject* Engine::GetWindow()
{
    return window;
}


void Engine::Exit()
{
    std::cout << "=====================================================" << std::endl;
    std::cout << "Engine closed. Exit" << std::endl;
    glfwTerminate();
}


double Engine::GetElapsedTime()
{
    return glfwGetTime();
}
