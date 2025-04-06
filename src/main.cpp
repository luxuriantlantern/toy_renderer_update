#include "scene/scene.h"
#include <memory>

int main()
{
    auto scene = std::make_shared<Scene>();
    scene->addModel("../assets/cube.obj");
    return 0;
}