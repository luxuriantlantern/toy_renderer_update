//
// Created by clx on 25-3-19.
//

#include "scene/scene.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "happly.h"
#include <algorithm>
#include <nlohmann/json.hpp>
namespace fs = std::filesystem;

Scene::Scene() {
    mCamera = nullptr;
}

void Scene::addObject(std::shared_ptr<Object> object) {
    mObjects.push_back(object);
}

void Scene::setCamera(std::shared_ptr<Camera> camera) {
    mCamera = camera;
}

const std::unordered_map<std::string, std::function<void(const std::filesystem::path&, std::shared_ptr<Object>)>> Scene::loadModelFunctions = {
        {".obj", loadOBJModel},
        {".ply", loadPLYModel},
};

void Scene::loadOBJModel(const fs::path &path, const std::shared_ptr<Object> &model) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    std::filesystem::path base_dir = std::filesystem::path(path).parent_path(); // For MTL

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str(), base_dir.string().c_str())) {
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

void Scene::loadPLYModel(const std::filesystem::path&path, const std::shared_ptr<Object> &model) {
    std::string Path = path.string();
    happly::PLYData plyIn(Path);
    std::vector<std::array<double, 3>> vPos = plyIn.getVertexPositions();
    std::vector<std::vector<size_t>> fInd;
    if (plyIn.hasElement("face") && plyIn.getElement("face").hasProperty("vertex_indices")) {
        fInd = plyIn.getFaceIndices<size_t>();
    }

    std::string name;
    if (!vPos.empty()) {
        name = std::filesystem::path(path).stem().string();
        model->setName(name);
    } else {
        std::cerr << "No vertices found in model" << std::endl;
        throw std::runtime_error("No vertices found in model");
    }

    Shape _shape;    // One ply file only has one shape
    _shape.name = name;
    auto calcVertNormal = [](const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        return normal;
    };
    if (fInd.empty()) {
        // Only vertices, no faces, create small triangles to show in renderer
        for (const auto& vertex : vPos) {
            glm::vec3 v = {
                    static_cast<float>(vertex[0]),
                    static_cast<float>(vertex[1]),
                    static_cast<float>(vertex[2])
            };

            float epsilon = 0.0001f;
            glm::vec3 v0 = v + glm::vec3(epsilon, 0.0f, 0.0f);
            glm::vec3 v1 = v + glm::vec3(0.0f, epsilon, 0.0f);
            glm::vec3 v2 = v + glm::vec3(0.0f, 0.0f, epsilon);
            _shape.vertices.push_back(v0);
            _shape.vertices.push_back(v1);
            _shape.vertices.push_back(v2);

            glm::vec3 normal = calcVertNormal(v0, v1, v2);
            _shape.normals.push_back(normal);
            _shape.normals.push_back(normal);
            _shape.normals.push_back(normal);
        }
    } else {
        // Has faces, mesh-like
        std::unordered_map<size_t, glm::vec3> vertNormals;
        std::unordered_map<size_t, std::vector<size_t>> vertexIndices;

        for (const auto& face: fInd) {
            if (face.size() < 3) continue;
            for (size_t i = 1; i < face.size() - 1; ++i) {
                size_t idx0 = face[0];
                size_t idx1 = face[i];
                size_t idx2 = face[i + 1];
                glm::vec3 v0 = {
                        static_cast<float>(vPos[idx0][0]),
                        static_cast<float>(vPos[idx0][1]),
                        static_cast<float>(vPos[idx0][2])
                };
                glm::vec3 v1 = {
                        static_cast<float>(vPos[idx1][0]),
                        static_cast<float>(vPos[idx1][1]),
                        static_cast<float>(vPos[idx1][2])
                };
                glm::vec3 v2 = {
                        static_cast<float>(vPos[idx2][0]),
                        static_cast<float>(vPos[idx2][1]),
                        static_cast<float>(vPos[idx2][2])
                };

                _shape.vertices.push_back(v0);
                _shape.vertices.push_back(v1);
                _shape.vertices.push_back(v2);
                _shape.normals.emplace_back(0.0f);
                _shape.normals.emplace_back(0.0f);
                _shape.normals.emplace_back(0.0f);

                glm::vec3 normal = calcVertNormal(v0, v1, v2);
                if (vertNormals.find(idx0) == vertNormals.end()) {
                    vertNormals[idx0] = glm::vec3(0.0f);
                    vertexIndices[idx0] = {};
                }
                vertNormals[idx0] += normal;
                vertexIndices[idx0].push_back(_shape.vertices.size() - 3);

                if (vertNormals.find(idx1) == vertNormals.end()) {
                    vertNormals[idx1] = glm::vec3(0.0f);
                    vertexIndices[idx1] = {};
                }
                vertNormals[idx1] += normal;
                vertexIndices[idx1].push_back(_shape.vertices.size() - 2);

                if (vertNormals.find(idx2) == vertNormals.end()) {
                    vertNormals[idx2] = glm::vec3(0.0f);
                    vertexIndices[idx2] = {};
                }
                vertNormals[idx2] += normal;
                vertexIndices[idx2].push_back(_shape.vertices.size() - 1);
            }
        }

        for (auto& pair : vertNormals) {
            pair.second = glm::normalize(pair.second);
            for (size_t idx : vertexIndices[pair.first]) {
                _shape.normals[idx] = pair.second;
            }
        }
    }
    model->addShape(_shape);
}

void Scene::loadJSON(const std::filesystem::path &path)
{
    std::ifstream file(path);
    if(!file.is_open())
    {
        std::cerr << "Failed to open file: " << path << std::endl;
        return;
    }
    nlohmann::json config = nlohmann::json::parse(file);
    if(config.contains("objects"))
    {
        auto object = config["objects"];
        for(auto &obj : object)
        {
            if(obj.contains("file"))
            {
                const std::filesystem::path &filePath = obj["file"];
                auto mobject = std::make_shared<Object>();
                std::string extension = filePath.extension().string();
                const auto it = loadModelFunctions.find(extension);
                if (it != loadModelFunctions.end()) {
                    it->second(filePath, mobject);
                } else {
                    std::cerr << "Unsupported file format: " << extension << std::endl;
                }
                mobject->setModelMatrix(glm::vec3{0.0f, 0.0f, 0.0f});
                if(obj.contains("position"))
                {
                    glm::vec3 pos = glm::vec3{obj["position"][0], obj["position"][1], obj["position"][2]};
                    mobject->setModelMatrix(pos);
                }
                if(obj.contains("scale"))
                {
                    glm::vec3 scale = glm::vec3 {obj["scale"][0], obj["scale"][1], obj["scale"][2]};
                    mobject->scale(scale);
                }
                if(obj.contains("rotation"))
                {
                    if(obj["rotation"].contains("type"))
                    {
                        if(obj["rotation"]["type"] == "euler_xyz")
                        {
                            glm::vec3 rot = glm::vec3{obj["rotation"]["data"][0], obj["rotation"]["data"][1], obj["rotation"]["data"][2]};
                            mobject->rotateEulerXYZ(rot.x, rot.y, rot.z);
                        }
                        if(obj["rotation"]["type"] == "quaternion")
                        {
                            glm::quat quaternion = glm::quat(obj["rotation"]["data"][0], obj["rotation"]["data"][1],
                                                             obj["rotation"]["data"][2], obj["rotation"]["data"][3]);
                            mobject->rotateQuaternion(quaternion);
                        }
                    }
                }
                addObject(mobject);
            }
        }
    }
}

void Scene::addModel(const std::filesystem::path &filePath) {
    std::string extension = filePath.extension().string();
    if(extension == ".json")
    {
        loadJSON(filePath);
        return;
    }
    const auto it = loadModelFunctions.find(extension);
    if (it != loadModelFunctions.end()) {
        auto model = std::make_shared<Object>();
        it->second(filePath, model);
        addObject(model);
    } else {
        std::cerr << "Unsupported file format: " << extension << std::endl;
    }
}

void Scene::removeModel(const std::shared_ptr<Object>& model) {
    auto it = std::find(mObjects.begin(), mObjects.end(), model);
    if (it != mObjects.end()) {
        mObjects.erase(it);
    }
}