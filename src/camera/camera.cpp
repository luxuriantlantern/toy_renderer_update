//
// Created by clx on 25-3-17.
//

#include "camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void Camera::moveForward(float deltaTime, float speed) {
    mPosition += mFront * speed * deltaTime;
}

void Camera::moveBackward(float deltaTime, float speed) {
    mPosition -= mFront * speed * deltaTime;
}

void Camera::moveLeft(float deltaTime, float speed) {
    mPosition -= mRight * speed * deltaTime;
}

void Camera::moveRight(float deltaTime, float speed) {
    mPosition += mRight * speed * deltaTime;
}

void Camera::moveUp(float deltaTime, float speed) {
    mPosition += mUp * speed * deltaTime;
}

void Camera::moveDown(float deltaTime, float speed) {
    mPosition -= mUp * speed * deltaTime;
}

void Camera::processLeftMouseMovement(float xoffset, float yoffset, float sensitivity, float speed) {
    xoffset *= sensitivity * 0.1f;
    yoffset *= sensitivity * 0.1f;

    this->moveRight(xoffset, speed);
    this->moveDown(yoffset, speed);
}

void Camera::processRightMouseMovement(float xoffset, float yoffset, float sensitivity, bool constrainPitch) {
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    mYaw += xoffset;
    mPitch += yoffset;

    if(mYaw > 180.0f)mYaw -= 360.0f;
    if(mYaw < -180.0f)mYaw += 360.0f;

    if (constrainPitch) {
        if (mPitch > 89.0f) mPitch = 89.0f;
        if (mPitch < -89.0f) mPitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
    front.y = sin(glm::radians(mPitch));
    front.z = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));

    mFront = glm::normalize(front);
    mRight = glm::normalize(glm::cross(mFront, glm::vec3(0.0f, 1.0f, 0.0f)));
    mUp = glm::normalize(glm::cross(mRight, mFront));
}