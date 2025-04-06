//
// Created by clx on 25-3-19.
//

#include "scene.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "happly.h"

Scene::Scene() {
    mCamera = nullptr;
}

void Scene::addObject(std::shared_ptr<Object> object) {
    mObjects.push_back(object);
}

void Scene::setCamera(std::shared_ptr<Camera> camera) {
    mCamera = camera;
}

const std::unordered_map<std::string, std::function<void(const std::string&, std::shared_ptr<Object>)>> Scene::loadModelFunctions = {
        {".obj", loadOBJModel},
        {".ply", loadPLYModel}
};

void Scene::loadOBJModel(const std::string &path, const std::shared_ptr<Object> &model) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::filesystem::path base_dir = std::filesystem::path(path).parent_path(); // For MTL

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), base_dir.string().c_str())) {
        std::cerr << "Failed to load model: " << warn << err << std::endl;
        throw std::runtime_error("Failed to load model");
    }

    if (!shapes.empty()) {
        model->setName(std::filesystem::path(path).stem().string());
    } else {
        std::cerr << "No shapes found in model" << std::endl;
        throw std::runtime_error("No shapes found in model");
    }

    // load vertices information(pos, normal, texcoord)
    for (const auto& shape : shapes) {
        Shape _shape;
        _shape.name = shape.name;
        for (const auto& index : shape.mesh.indices) {
            glm::vec3 vertex = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
            };
            _shape.vertices.push_back(vertex);

            if (!attrib.normals.empty()) {
                glm::vec3 normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                };
                _shape.normals.push_back(normal);
            }

            if (!attrib.texcoords.empty()) {
                glm::vec2 texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1]
                };
                _shape.texCoords.push_back(texCoord);
            }
        }
        if(attrib.normals.empty())
        {
            _shape.normals.resize(_shape.vertices.size(), glm::vec3(0.0f));
            for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
                tinyobj::index_t idx0 = shape.mesh.indices[i];
                tinyobj::index_t idx1 = shape.mesh.indices[i + 1];
                tinyobj::index_t idx2 = shape.mesh.indices[i + 2];

                size_t v0_idx = i;
                size_t v1_idx = i + 1;
                size_t v2_idx = i + 2;

                glm::vec3 v0 = _shape.vertices[v0_idx];
                glm::vec3 v1 = _shape.vertices[v1_idx];
                glm::vec3 v2 = _shape.vertices[v2_idx];

                glm::vec3 faceNormal = glm::cross(v1 - v0, v2 - v0);

                _shape.normals[v0_idx] += faceNormal;
                _shape.normals[v1_idx] += faceNormal;
                _shape.normals[v2_idx] += faceNormal;
            }

            for (auto& normal : _shape.normals) {
                if (glm::length(normal) > 0.0001f) {
                    normal = glm::normalize(normal);
                }
            }
        }

        if (!materials.empty() && shape.mesh.material_ids[0] >= 0) {
            size_t material_id = shape.mesh.material_ids[0];
            if (material_id < materials.size() && !materials[material_id].diffuse_texname.empty()) {
                std::filesystem::path texture_path = base_dir / materials[material_id].diffuse_texname;
                _shape.texturePath = texture_path.string();
            }
        }

        model->addShape(_shape);
    }
}

void Scene::loadPLYModel(const std::string &path, const std::shared_ptr<Object> &model) {
    return;
}

std::shared_ptr<Object> Scene::addModel(const std::string &filePath) {
    std::string extension = filePath.substr(filePath.find_last_of('.'));
    auto it = loadModelFunctions.find(extension);
    if (it != loadModelFunctions.end()) {
        auto model = std::make_shared<Object>();
        it->second(filePath, model);
        addObject(model);
        return model;
    } else {
        std::cerr << "Unsupported file format: " << extension << std::endl;
        return nullptr;
    }
}