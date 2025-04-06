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
    Camera &operator=(const Camera &other) {*this = other; return *this;}
    Camera(glm::vec3 position) {mPosition = position;}

    virtual void update(int w, int h) = 0;
    glm::mat4 getViewMatrix() const {return mViewMatrix;}
    glm::mat4 getProjectionMatrix() const {return mProjectionMatrix;}
    glm::vec3 getPosition() const {return mPosition;}
    virtual CameraType getType() = 0;
    virtual void setFromIntrinsics(float fx, float fy, float cx, float cy, float near = 0.1f, float far = 100.0f) = 0;

    // Movement controls
    void moveForward(float deltaTime);
    void moveBackward(float deltaTime);
    void moveLeft(float deltaTime);
    void moveRight(float deltaTime);
    void moveUp(float deltaTime);
    void moveDown(float deltaTime);

    // Mouse rotation control
    void processRightMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void processLeftMouseMovement(float xoffset, float yoffset);

    // Setters for movement parameters
    void setMovementSpeed(float speed) { mMovementSpeed = speed; }
    void setMouseSensitivity(float sensitivity) { mMouseSensitivity = sensitivity; }
    void setPosition(glm::vec3 position) { mPosition = position; }
    void setWindowSize(float width, float height) { mwidth = width; mheight = height; }

protected:
    glm::mat4 mViewMatrix{1.0f};
    glm::mat4 mProjectionMatrix{1.0f};

    glm::vec3 mPosition{0.0f};
    glm::vec3 mFront{0.0f, 0.0f, -1.0f};
    glm::vec3 mUp{0.0f, 1.0f, 0.0f};
    glm::vec3 mRight{1.0f, 0.0f, 0.0f};

    float mYaw = -90.0f;
    float mPitch = 0.0f;
    float mMovementSpeed = 2.5f;
    float mMouseSensitivity = 0.1f;

    int mwidth, mheight;

    void updateCameraVectors();
};


#endif //TOY_RENDERER_CAMERA_H
