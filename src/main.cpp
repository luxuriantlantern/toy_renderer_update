#include "scene/scene.h"
#include "viewer/viewer.h"
#include "camera/perspective.h"
#include <memory>

int main()
{
    const int WIDTH = 1920, HEIGHT = 1080;
    auto scene = std::make_shared<Scene>();
    scene->addModel("../assets/cube2.ply");
    auto camera = std::make_shared<PerspectiveCamera>();
    auto viewer = std::make_shared<Viewer>(WIDTH, HEIGHT, camera, scene);
    viewer->mainloop();
    return 0;
}