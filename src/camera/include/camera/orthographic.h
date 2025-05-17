//
// Created by clx on 25-3-17.
//

#ifndef TOY_RENDERER_ORTHOGRAPHIC_H
#define TOY_RENDERER_ORTHOGRAPHIC_H

#include "camera/camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class OrthographicCamera : public Camera{
public:
    OrthographicCamera() = default;

    explicit OrthographicCamera(const Camera& other) : Camera(other) {}

    void update(int w, int h) override {
        glm::vec3 front;
        front.x = static_cast<float>(cos(glm::radians(mYaw)) * cos(glm::radians(mPitch)));
        front.y = static_cast<float>(sin(glm::radians(mPitch)));
        front.z = static_cast<float>(sin(glm::radians(mYaw)) * cos(glm::radians(mPitch)));
        mFront = glm::normalize(front);
        mViewMatrix = glm::lookAt(mPosition, mPosition + mFront, mUp);

        mAspectRatio = static_cast<float>(w) / static_cast<float>(h);
        float orthoScale = 20.0f;
        float left = -orthoScale * mAspectRatio;
        float right = orthoScale * mAspectRatio;
        float bottom = -orthoScale;
        float top = orthoScale;

        mProjectionMatrix = glm::ortho(left, right, bottom, top, mNear, mFar);
    }

    CameraType getType() override {return ORTHOGRAPHIC;}

};

#endif //TOY_RENDERER_ORTHOGRAPHIC_H
