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
    PerspectiveCamera(float fov = 45.0f,
                      float aspectRatio = 1.0f,
                      float near = 0.1f,
                      float far = 100.0f,
                      glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f)):
                      Camera(position),
                      mfov(fov),
                      maspectRatio(aspectRatio),
                      mnear(near),
                      mfar(far) {}

    void setFromIntrinsics(float fx, float fy, float cx, float cy, float near = 0.1f, float far = 100.0f) {
        maspectRatio = static_cast<float>(mwidth) / static_cast<float>(mheight);
        mnear = near;
        mfar = far;

        mProjectionMatrix = glm::mat4(1.0f);
        mProjectionMatrix[0][0] = 2.0f * fx / mwidth;
        mProjectionMatrix[1][1] = 2.0f * fy / mheight;
        mProjectionMatrix[2][0] = 1.0f - 2.0f * cx / mwidth;
        mProjectionMatrix[2][1] = 2.0f * cy / mheight - 1.0f;
        mProjectionMatrix[2][2] = -(far + near) / (far - near);
        mProjectionMatrix[2][3] = -1.0f;
        mProjectionMatrix[3][2] = -2.0f * far * near / (far - near);
        mProjectionMatrix[3][3] = 0.0f;
        mfov = 2.0f * atan2(mheight / 2.0f, fy) * 180.0f / M_PI;
        mViewMatrix = glm::lookAt(mPosition, mPosition + mFront, mUp);
    }

    void update(int w, int h) override {
        mwidth = w, mheight = h;
        maspectRatio = static_cast<float>(mwidth) / static_cast<float>(mheight);
        mViewMatrix = glm::lookAt(mPosition, mPosition + mFront, mUp);
        mProjectionMatrix = glm::perspective(mfov, maspectRatio, mnear, mfar);
    }

    CameraType getType() override {return PERSPECTIVE;}

    void setFov(float fov) {mfov = fov;}
    void setAspectRatio(float aspectRatio) {maspectRatio = aspectRatio;}
    void setNear(float near) {mnear = near;}
    void setFar(float far) {mfar = far;}

    float getFov() const {return mfov;}
    float getAspectRatio() const {return maspectRatio;}
    float getNear() const {return mnear;}
    float getFar() const {return mfar;}

private:
    float mfov;
    float maspectRatio;
    float mnear;
    float mfar;
};


#endif //TOY_RENDERER_PERSPECTIVE_H
