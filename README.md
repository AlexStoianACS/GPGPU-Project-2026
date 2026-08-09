### GPGPU Workshop - 3DUPB
*Mihnea Mitrache, mihnea.mitrache@upb.ro*

In this repository you will find the base code for the GPGPU Workshop Project held at 3DUPB, Faculty of Automatic Control and Computer Science, University Politehnica of Bucharest.

#### Structure
This repository follows the structure of the [3D UPB-GFX framework](https://github.com/UPB-Graphics/gfx-framework)
In order to visually inspect the results, we need a graphics context. The framework provides a simple OpenGL context, and a set of classes to manage the window, the camera, and the input events.

I added CUDA support to the framework, so you can use it to run your GPGPU code and visualize the results. Watch the CMAKELists.txt file to see where CUDA is enabled, and set your CUDA architecture according to your GPU.
```cmake
# TODO: Set your CUDA architecture here. Check https://developer.nvidia.com/cuda-gpus for a list of supported architectures.
# If you list more than one architecture, CMake will build multiple versions of the CUDA code, which will increase build time and binary size.
if (NOT CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 120)
endif()

# Set the name of the project
set(target_name GFXFramework)
project(${target_name} C CXX CUDA)

```

#### Task
You will simulate physics for a bunch of boxes in a 3D space. See the last session slides for a thorough description of the task. The main idea is to use CUDA to compute the physics simulation on the GPU, and then visualize the results using OpenGL.
You must implement first on CPU and then on GPU. The app should also be able to switch between CPU and GPU implementations at runtime.
You can watch a demo video of the final result [here](https://www.youtube.com/watch?v=EUYCXjW3RTA). The second part is after pressing the 'G' key, which toggles the simulation between CPU and GPU implementations. You can observe how laggy the CPU implementation is, and how smooth the GPU implementation is.

#### Setting up the project
1. Clone the repository
2. Set up the project using CMake. You can use the following commands:
```bash
mkdir build
cmake -B build .
```
3. Modify the files and implement the physics simulation on CPU and GPU. You can find the files in the `src` folder. I modularized the code into several files, so you can focus on the parts that are relevant to your task.
4. Build the project using CMake:
```bash
cmake --build build --config Release
```
The above uses the command line for building the project, but you can also use the graphical interface. If not remembering, look at [UPB GFX configuration guide](https://ocw.cs.pub.ro/courses/pgapi/setup-framework) for more details.

**Good luck with your implementation! If you have any questions, feel free to ask on Teams :)**
