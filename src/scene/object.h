//
// Created by clx on 25-3-19.
//

#ifndef OBJECT_H
#define OBJECT_H

#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <stb_image.h>
#include <iostream>

struct Shape {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::string texturePath;// std::filesystem::path
    std::string name;
    bool visible = true;
};

class Object {
public:
    Object() = default;
    ~Object() = default;

    glm::mat4 getModelMatrix() const { return model; }
    size_t getShapeCount() const { return shapes.size(); }
    void setModelMatrix(const glm::mat4 &modelMatrix) { model = modelMatrix; }
    void setModelMatrix(const glm::vec3 pos){ model = glm::translate(model, pos); }
    void scale(const glm::vec3 &scale) { model = glm::scale(model, scale); }
    void rotate(float angleInDegrees, const glm::vec3& axis) {
        model = glm::rotate(model, glm::radians(angleInDegrees), glm::normalize(axis));
    }
    void rotateEulerXYZ(float angleX, float angleY, float angleZ) {
        float radX = glm::radians(angleX);
        float radY = glm::radians(angleY);
        float radZ = glm::radians(angleZ);

        glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), radX, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), radY, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), radZ, glm::vec3(0.0f, 0.0f, 1.0f));

        model = model * rotZ * rotY * rotX;
    }

    void rotateQuaternion(const glm::quat& quaternion) {
        glm::mat4 rotationMatrix = glm::mat4_cast(quaternion);
        model = model * rotationMatrix;
    }
    void rotateQuaternion(float angleInDegrees, const glm::vec3& axis) {
        glm::quat quaternion = glm::angleAxis(glm::radians(angleInDegrees), glm::normalize(axis));
        rotateQuaternion(quaternion);
    }
    void setName(const std::string& objectName) { name = objectName; }
    void addShape(const Shape& shape) { shapes.push_back(shape); }
    std::vector<glm::vec3> getVertices(size_t shapeIndex) const { return shapes[shapeIndex].vertices; }
    std::vector<glm::vec3> getNormals(size_t shapeIndex) const { return shapes[shapeIndex].normals; }
    std::vector<glm::vec2> getTexCoords(size_t shapeIndex) const { return shapes[shapeIndex].texCoords; }
    std::string getTexturePath(size_t shapeIndex) const { return shapes[shapeIndex].texturePath; }

private:
    std::vector<Shape> shapes;
    glm::mat4 model{1.0f};
    std::string name;
};

#endif //OBJECT_H
