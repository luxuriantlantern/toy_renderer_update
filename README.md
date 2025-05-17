# toy_renderer_update

An updated version of the previous [toy_render](https://github.com/luxuriantlantern/toy_renderer/), which has many bugs and cannot run directly.

For this version, I still have many bugs to fix right now(25.5.17). And I add many absolute filepaths in the project, so you may need to open it with an IDE like CLion or Visual Studio Code to run it(At toy_renderer_update, the project's root). I will fix this in the future.

### Some functions I have finished:

* The basic functions of a renderer, such as loading a model and rendering it.
* Support .obj and .ply files, for .json files, it should has following format:
```json
{
  "objects": [
    {
      "file": "./assets/cube.obj",
      "position": [-2, -2, -2],
      "scale": [1, 1, 1],
      "rotation": {
        "type": "euler_xyz",
        "data": [30, 30, 30]
      }
    },
    {
      "file": "./assets/cube2.ply",
      "position": [0, 0, 0],
      "scale": [0.5, 0.5, 0.5],
      "rotation": {
        "type": "quaternion",
        "data": [0, 0, 0, 1]
      }
    }
  ]
}
```
* Support camera control, using wasd and space(for up)and left shift(for down). You can also choose perspective or orthographic projection.
* Support basic shaders, like Blinn-Phong, Material and Wireframe.
* Support OpenGL and Vulkan backends. Unluckily, I have not finished the Vulkan backend yet, it will break down when you terminate it. You can switch OpenGL to Vulkan backend but from Vulkan to OpenGL, it will break down. I will fix this in the future.
* Support window resizing(only for OpenGL).


### How to run it

I only test it on Windows 11 and Linux Ubuntu 22.04 with CLion.

0. Make sure you have installed vulkan sdk.

1. Clone the repository

```bash
git clone https://github.com/luxuriantlantern/toy_renderer_update.git --recursive
```
or
```bash
git clone git@github.com:luxuriantlantern/toy_renderer_update.git --recursive
```

2. Open the project with CLion or Visual Studio Code.

3. Build the project with CMake. Then run TR_EXE_MAIN with working directory path/to/toy_renderer_update.

### Note

As the renderer do not support backend switching, the initial backend is Vulkan. You can change line 105 in root/viewer/include/viewer/viewer.h to 
```cpp
SHADER_BACKEND_TYPE mShaderBackendType = SHADER_BACKEND_TYPE::OPENGL;
```

to start with OpenGL backend.

### TODO

* Fix many bugs and add some new basic functions for a renderer.

~~All of these things above have cost me more than 200h~~