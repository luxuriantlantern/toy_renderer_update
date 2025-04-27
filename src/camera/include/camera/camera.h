//
// Created by clx on 25-3-17.
//

#ifndef TOY_RENDERER_CAMERA_H
#define TOY_RENDERER_CAMERA_H

#include<glm/glm.hpp>

enum CameraType {
    PERSPECTIVE,
    ORTHOGRAPHIC
};

class Camera {
public:
    Camera() = default;
    virtual ~Camera() = default;
    Camera(const Camera &other) {*this = other;}
    Camera& operator=(const Camera& other) {
        if (this != &other) {
            mPosition = other.mPosition;
            mFront = other.mFront;
            mUp = other.mUp;
            mYaw = other.mYaw;
            mPitch = other.mPitch;
            mFov = other.mFov;
            mNear = other.mNear;
            mFar = other.mFar;
            mAspectRatio = other.mAspectRatio;
        }
        return *this;
    }
    Camera(glm::vec3 position) {mPosition = position;}
    void resetControl(int w, int h) {
        mPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        mFront = glm::vec3(0.0f, 0.0f, -1.0f);
        mUp = glm::vec3(0.0f, 1.0f, 0.0f);
        mYaw = -90.0f;
        mPitch = 0.0f;
        mFov = 45.0f;
        mNear = 0.1f;
        mFar = 1000.0f;
        update(w, h);
    }

    virtual void update(int w, int h) = 0;
    glm::mat4 getViewMatrix() const {return mViewMatrix;}
    glm::mat4 getProjectionMatrix() const {return mProjectionMatrix;}
    glm::vec3 getPosition() const {return mPosition;}
    float getFov() const {return mFov;}
    float getYaw() const {return mYaw;}
    float getPitch() const {return mPitch;}
    void setFov(float fov) {mFov = fov;}
    void setYaw(float yaw) {mYaw = yaw;}
    void setPitch(float pitch) {mPitch = pitch;}
    virtual CameraType getType() = 0;

    // Movement controls
    void moveForward(float deltaTime, float speed);
    void moveBackward(float deltaTime, float speed);
    void moveLeft(float deltaTime, float speed);
    void moveRight(float deltaTime, float speed);
    void moveUp(float deltaTime, float speed);
    void moveDown(float deltaTime, float speed);

    // Mouse rotation control
    void processRightMouseMovement(float xoffset, float yoffset, float sensitivity, bool constrainPitch = true);
    void processLeftMouseMovement(float xoffset, float yoffset, float speed, float sensitivity);

    // Setters for movement parameters
    void setPosition(glm::vec3 position) { mPosition = position; }
    void setWindowSize(float width, float height) { mwidth = width; mheight = height; }

protected:
    glm::mat4 mViewMatrix{1.0f};
    glm::mat4 mProjectionMatrix{1.0f};

    glm::vec3 mPosition{0.0f, 0.0f, 2.0f};
    glm::vec3 mFront{0.0f, 0.0f, -1.0f};
    glm::vec3 mUp{0.0f, 1.0f, 0.0f};
    glm::vec3 mRight{1.0f, 0.0f, 0.0f};

    float mYaw = -90.0f;
    float mPitch = 0.0f;
    float mFov = 45.0f;
    float mNear = 0.1f;
    float mFar = 100.0f;

    int mwidth, mheight;
    float mAspectRatio;

    void updateCameraVectors();
};


#endif //TOY_RENDERER_CAMERA_H
