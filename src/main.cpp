#include "scene/scene.h"
#include "viewer/viewer.h"
#include "camera/perspective.h"
#include <memory>
#include "render/render_OpenGL.h"
#include "viewer/ui/modelUI.h"
#include "viewer/ui/shaderUI.h"
#include "viewer/ui/cameraUI.h"

int main()
{
    const int WIDTH = 1920, HEIGHT = 1080;
    const std::string title = "toy renderer";
    auto scene = std::make_shared<Scene>();
    scene->addModel("./assets/SJTU_east_gate_MC/East_Gate_Voxel.obj");
//    scene->addModel("./assets/cube2.ply");
    auto camera = std::make_shared<PerspectiveCamera>();
    camera->update(WIDTH, HEIGHT);
    auto render = std::make_shared<Render_OpenGL>();
    auto viewer = std::make_shared<Viewer>(WIDTH, HEIGHT, render, camera, scene, title);
    const auto ui_model = std::make_shared<ModelUI>(viewer);
    const auto ui_shader = std::make_shared<ShaderUI>(viewer);
    const auto ui_camera = std::make_shared<CameraUI>(viewer);
    viewer->addUI(ui_model);
    viewer->addUI(ui_shader);
    viewer->addUI(ui_camera);
    render->init();
    render->setup(scene);
    viewer->mainloop();
    return 0;
}