#include "scene/scene.h"
#include <memory>

int main()
{
    auto scene = std::make_shared<Scene>();
    scene->addModel("../assets/cube2.ply");
    return 0;
}