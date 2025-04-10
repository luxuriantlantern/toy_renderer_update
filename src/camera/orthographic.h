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
    OrthographicCamera(float left = -30.0f,
                       float right = 30.0f,
                       float bottom = -30.0f,
                       float top = 30.0f,
                       float near = 0.1f,
                       float far = 2000.0f):
            mleft(left), mright(right), mbottom(bottom), mtop(top), mnear(near), mfar(far) {}

    void setFromIntrinsics(float fx, float fy, float cx, float cy, float near = 0.1f, float far = 100.0f) {}

    void update(int w, int h) override {
        mwidth = w, mheight = h;
        mAspectRatio = static_cast<float>(w) / static_cast<float>(h);
        mtop = mOrthoScale;
        mbottom = -mOrthoScale;
        mleft = -mOrthoScale * mAspectRatio;
        mright = mOrthoScale * mAspectRatio;
        mViewMatrix = glm::lookAt(mPosition, mPosition + mFront, mUp);
        mViewMatrix = glm::scale(mViewMatrix, glm::vec3(0.03f, 0.03f, 0.03f));
        mProjectionMatrix = glm::ortho(mleft, mright, mbottom, mtop, mnear, mfar);
    }

    CameraType getType() override {return ORTHOGRAPHIC;}

    void setOrthoSize(float left, float right, float bottom, float top) {
        mleft = left; mright = right;
        mbottom = bottom; mtop = top;
    }
    void setNearFar(float near, float far) { mnear = near; mfar = far; }

    glm::mat4 getProjectionMatrix() const { return mProjectionMatrix; }

protected:
    glm::mat4 mProjectionMatrix{1.0f};

private:
    float mleft;
    float mright;
    float mbottom;
    float mtop;
    float mnear;
    float mfar;
    float mOrthoScale = 100.0f;
};

#endif //TOY_RENDERER_ORTHOGRAPHIC_H
