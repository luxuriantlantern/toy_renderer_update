//
// Created by clx on 25-3-17.
//

#include "camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void Camera::moveForward(float deltaTime) {
    mPosition += mFront * mMovementSpeed * deltaTime;
}

void Camera::moveBackward(float deltaTime) {
    mPosition -= mFront * mMovementSpeed * deltaTime;
}

void Camera::moveLeft(float deltaTime) {
    mPosition -= mRight * mMovementSpeed * deltaTime;
}

void Camera::moveRight(float deltaTime) {
    mPosition += mRight * mMovementSpeed * deltaTime;
}

void Camera::moveUp(float deltaTime) {
    mPosition += mUp * mMovementSpeed * deltaTime;
}

void Camera::moveDown(float deltaTime) {
    mPosition -= mUp * mMovementSpeed * deltaTime;
}

void Camera::processLeftMouseMovement(float xoffset, float yoffset) {
    xoffset *= mMouseSensitivity * 0.1f;
    yoffset *= mMouseSensitivity * 0.1f;

    this->moveRight(xoffset);
    this->moveDown(yoffset);
}

void Camera::processRightMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= mMouseSensitivity;
    yoffset *= mMouseSensitivity;

    mYaw += xoffset;
    mPitch += yoffset;

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