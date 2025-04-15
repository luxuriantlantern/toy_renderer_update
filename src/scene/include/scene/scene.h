//
// Created by clx on 25-3-19.
//

#ifndef TOY_RENDERER_SCENE_H
#define TOY_RENDERER_SCENE_H

#include "camera/camera.h"
#include "object.h"
#include <memory>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <filesystem>

class Scene {
public:
    Scene();
    ~Scene() = default;

    void addObject(std::shared_ptr<Object> object);
    void setCamera(std::shared_ptr<Camera> camera);
    std::shared_ptr<Camera> getCamera() const { return mCamera; }
    std::shared_ptr<Object> addModel(const std::filesystem::path& filePath);
    std::vector<std::shared_ptr<Object>> getModels() const { return mObjects; }
    void removeModel(const std::shared_ptr<Object>& model);

private:
    std::vector<std::shared_ptr<Object>> mObjects;
    std::shared_ptr<Camera> mCamera;

    static const std::unordered_map<std::string, std::function<void(const std::filesystem::path&, std::shared_ptr<Object>)>> loadModelFunctions;
    static void loadOBJModel(const std::filesystem::path& path, const std::shared_ptr<Object>& model);
    static void loadPLYModel(const std::filesystem::path& path, const std::shared_ptr<Object>& model);
};

#endif //TOY_RENDERER_SCENE_H