//
// Created by clx on 25-4-7.
//

#ifndef TOY_RENDERER_UPDATE_RENDER_H
#define TOY_RENDERER_UPDATE_RENDER_H

#include "camera/camera.h"
#include "scene/scene.h"
#include "scene/object.h"
#include "../shader/shader.h"

class Render {
public:
    virtual ~Render() = default;

    virtual void render(
            const std::shared_ptr<Scene>& scene,
            const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix
    ) = 0;

    virtual void setup(const std::shared_ptr<Scene>& scene) = 0;
    virtual void addModel(const std::shared_ptr<Object>& model) = 0;
    virtual void removeModel(const std::shared_ptr<Object>& model) = 0;
    virtual void cleanup() = 0;
    virtual void init() = 0;

protected:
    std::unordered_map<SHADER_TYPE, std::shared_ptr<Shader>> mShaders;
    std::pair<SHADER_TYPE, std::shared_ptr<Shader>> mCurrentShader;
};


#endif //TOY_RENDERER_UPDATE_RENDER_H
