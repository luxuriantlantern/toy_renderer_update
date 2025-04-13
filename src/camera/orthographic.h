//
// Created by clx on 25-3-17.
//

#ifndef TOY_RENDERER_ORTHOGRAPHIC_H
#define TOY_RENDERER_ORTHOGRAPHIC_H

#include "camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class OrthographicCamera : public Camera{
public:
    OrthographicCamera(float near = 0.001f,
                       float far = 20000.0f): mNear(near), mFar(far) {}


    void update(int w, int h) override {
        glm::vec3 front;
        front.x = static_cast<float>(cos(glm::radians(mYaw)) * cos(glm::radians(mPitch)));
        front.y = static_cast<float>(sin(glm::radians(mPitch)));
        front.z = static_cast<float>(sin(glm::radians(mYaw)) * cos(glm::radians(mPitch)));
        mViewMatrix = glm::lookAt(mPosition, mPosition + mFront, mUp) * glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));

        float orthoScale = 30.0f;
        float left = -orthoScale * mAspectRatio;
        float right = orthoScale * mAspectRatio;
        float bottom = -orthoScale;
        float top = orthoScale;

        mProjectionMatrix = glm::ortho(left, right, bottom, top, mNear, mFar);
    }

    CameraType getType() override {return ORTHOGRAPHIC;}

    glm::mat4 getProjectionMatrix() const { return mProjectionMatrix; }

protected:
    glm::mat4 mProjectionMatrix{1.0f};

private:
    float mNear;
    float mFar;
    float mOrthoScale = 10.0f;
};

#endif //TOY_RENDERER_ORTHOGRAPHIC_H
