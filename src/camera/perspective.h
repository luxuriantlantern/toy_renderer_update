//
// Created by clx on 25-3-17.
//

#ifndef TOY_RENDERER_PERSPECTIVE_H
#define TOY_RENDERER_PERSPECTIVE_H

#include "camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class PerspectiveCamera : public Camera{
public:

    PerspectiveCamera() = default;
    explicit PerspectiveCamera(const Camera& other) : Camera(other) {}

//    void setFromIntrinsics(float fx, float fy, float cx, float cy, float near = 0.1f, float far = 100.0f) {
//        mAspectRatio = static_cast<float>(mwidth) / static_cast<float>(mheight);
//        mNear = near;
//        mFar = far;
//
//        mProjectionMatrix = glm::mat4(1.0f);
//        mProjectionMatrix[0][0] = 2.0f * fx / mwidth;
//        mProjectionMatrix[1][1] = 2.0f * fy / mheight;
//        mProjectionMatrix[2][0] = 1.0f - 2.0f * cx / mwidth;
//        mProjectionMatrix[2][1] = 2.0f * cy / mheight - 1.0f;
//        mProjectionMatrix[2][2] = -(far + near) / (far - near);
//        mProjectionMatrix[2][3] = -1.0f;
//        mProjectionMatrix[3][2] = -2.0f * far * near / (far - near);
//        mProjectionMatrix[3][3] = 0.0f;
//        mFov = 2.0f * atan2(mheight / 2.0f, fy) * 180.0f / M_PI;
//        mViewMatrix = glm::lookAt(mPosition, mPosition + mFront, mUp);
//    }

    void update(int w, int h) override {
        mwidth = w, mheight = h;
        glm::vec3 front;
        front.x = static_cast<float>(cos(glm::radians(mYaw)) * cos(glm::radians(mPitch)));
        front.y = static_cast<float>(sin(glm::radians(mPitch)));
        front.z = static_cast<float>(sin(glm::radians(mYaw)) * cos(glm::radians(mPitch)));
        mFront = glm::normalize(front);
        mAspectRatio = static_cast<float>(mwidth) / static_cast<float>(mheight);
        mViewMatrix = glm::lookAt(mPosition, mPosition + mFront, mUp);
        mProjectionMatrix = glm::perspective(glm::radians(mFov), mAspectRatio, mNear, mFar);
    }

    CameraType getType() override {return PERSPECTIVE;}

    void setFov(float fov) {mFov = fov;}
    void setAspectRatio(float aspectRatio) {mAspectRatio = aspectRatio;}
    void setNear(float near) {mNear = near;}
    void setFar(float far) {mFar = far;}

    float getFov() const {return mFov;}
    float getAspectRatio() const {return mAspectRatio;}
    float getNear() const {return mNear;}
    float getFar() const {return mFar;}

};


#endif //TOY_RENDERER_PERSPECTIVE_H
