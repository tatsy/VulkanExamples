#ifdef _MSC_VER
#pragma once
#endif

#ifndef _TRIMESH_H_
#define _TRIMESH_H_

#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

struct Vertex {
    glm::vec3 position;
    glm::vec2 texcoord;
    glm::vec3 normal;

    bool operator==(const Vertex &other) const {
        return position == other.position &&
               normal == other.normal &&
               texcoord == other.texcoord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texcoord) << 1);
        }
    };
}

class TriMesh {
public:
    TriMesh() {
    }

    TriMesh(const std::string &filename)
        : TriMesh{} {
        load(filename);
    }
    
    virtual ~TriMesh() {
    }

    void load(const std::string &filename) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;

        bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str());
        if (!err.empty()) {
            std::cerr << "[WARNING] " << err << std::endl;
        }

        if (!success) {
            throw std::runtime_error("[ERROR] failed to load mesh file!");
        }

        indices.clear();

        // Unique vertex attributes.
        std::vector<Vertex> vertices;
        std::unordered_map<Vertex, uint32_t> uniqueVertices;

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex = {};

                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                if (index.texcoord_index >= 0) {
                    vertex.texcoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                } else {
                    vertex.texcoord = { 0.0f, 0.0f };
                }

                if (index.normal_index >= 0) {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    };
                } else {
                    vertex.normal = { 0.0f, 0.0f, 0.0f };
                }

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = vertices.size();
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }

        // Set vertex attributes.
        positions.clear();
        texcoords.clear();
        normals.clear();

        positions.reserve(vertices.size() * 3);
        texcoords.reserve(vertices.size() * 2);
        normals.reserve(vertices.size() * 3);
        for (const auto &v: vertices) {
            positions.push_back(v.position.x);
            positions.push_back(v.position.y);
            positions.push_back(v.position.z);

            texcoords.push_back(v.texcoord.x);
            texcoords.push_back(v.texcoord.y);

            normals.push_back(v.normal.x);
            normals.push_back(v.normal.y);
            normals.push_back(v.normal.z);
        }
    }

    std::vector<float> positions;
    std::vector<float> texcoords;
    std::vector<float> normals;
    std::vector<uint32_t> indices;
    uint8_t *diffuse_texture;
    std::vector<std::string> diffuse_texnames;
};

#endif  // _TRIMESH_H_
