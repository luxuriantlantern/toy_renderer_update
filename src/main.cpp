#include "scene/scene.h"
#include "viewer/viewer.h"
#include "camera/perspective.h"
#include <memory>
#include "render/render_OpenGL.h"

int main()
{
    const int WIDTH = 1920, HEIGHT = 1080;
    const std::string title = "toy renderer";
    auto scene = std::make_shared<Scene>();
    scene->addModel("../assets/cube.obj");
    auto camera = std::make_shared<PerspectiveCamera>();
    auto render = std::make_shared<Render_OpenGL>();
    auto viewer = std::make_shared<Viewer>(WIDTH, HEIGHT, render, camera, scene, title);
    render->init();
    render->setup(scene);
    viewer->mainloop();
    return 0;
}