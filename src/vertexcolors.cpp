#include "vertexcolors.h"
#include <cmath>
#include <vector>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <iostream>

void computeVertexColorsFromTextures(
    const tinyobj::attrib_t& attrib,
    const std::vector<tinyobj::shape_t>& shapes,
    const tinyobj::material_t& material,
    const int material_id,
    const std::map<std::string, Texture>& textures,
    std::vector<Vec3>& vertexColors) {

    // Initialize vertexColors if empty
    if (vertexColors.empty()) {
        vertexColors.resize(attrib.vertices.size() / 3, Vec3(1, 1, 1));
    }
    std::vector<Vec3> vertexNormals(attrib.vertices.size() / 3, Vec3(0, 0, 0));

    if (attrib.texcoords.empty()) {
        std::cout << "No texture coordinates found in the OBJ file." << std::endl;
        return;
    }

    std::cout << "Number of shapes: " << shapes.size() << std::endl;
    std::cout << "Number of textures: " << textures.size() << std::endl;
    std::cout << "Processing material with texture: " << material.diffuse_texname << std::endl;

    int texturedVertices = 0;

    // Helper functions for vector operations
    auto vec3_subtract = [](const Vec3& a, const Vec3& b) -> Vec3 {
        return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
    };

    auto vec3_cross = [](const Vec3& a, const Vec3& b) -> Vec3 {
        return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    };

    auto vec3_normalize = [](Vec3& v) {
        float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (length > 0) {
            v.x /= length;
            v.y /= length;
            v.z /= length;
        }
    };

    // First check if we have the texture
    auto textureIt = textures.find(material.diffuse_texname);
    if (textureIt == textures.end()) {
        std::cout << "Texture not found: " << material.diffuse_texname << std::endl;
        // Apply material color to all vertices that use this material
        Vec3 materialColor(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
        for (const auto& shape : shapes) {
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                if (shape.mesh.material_ids[f] == material_id) {  // Only process faces using this material
                    unsigned int fv = shape.mesh.num_face_vertices[f];
                    for (unsigned int v = 0; v < fv; v++) {
                        tinyobj::index_t idx = shape.mesh.indices[f * fv + v];
                        vertexColors[idx.vertex_index] = materialColor;
                    }
                }
            }
        }
        return;
    }

    const auto& texture = textureIt->second;

    // First pass: compute vertex normals only for faces using this material
    for (const auto& shape : shapes) {
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            if (shape.mesh.material_ids[f] != material_id) {
                std::cout << "Skipping face with material ID " << shape.mesh.material_ids[f] << std::endl;
                continue;  // Skip faces not using this material
            }

            unsigned int fv = static_cast<unsigned int>(shape.mesh.num_face_vertices[f]);

            // Compute face normal
            Vec3 v0, v1, v2;
            for (unsigned int v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[f * fv + v];
                float vx = attrib.vertices[3 * idx.vertex_index + 0];
                float vy = attrib.vertices[3 * idx.vertex_index + 1];
                float vz = attrib.vertices[3 * idx.vertex_index + 2];
                if (v == 0) v0 = Vec3(vx, vy, vz);
                if (v == 1) v1 = Vec3(vx, vy, vz);
                if (v == 2) v2 = Vec3(vx, vy, vz);
            }
            Vec3 faceNormal = vec3_cross(vec3_subtract(v1, v0), vec3_subtract(v2, v0));
            vec3_normalize(faceNormal);

            // Accumulate face normal to vertex normals
            for (unsigned int v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[f * fv + v];
                vertexNormals[idx.vertex_index].x += faceNormal.x;
                vertexNormals[idx.vertex_index].y += faceNormal.y;
                vertexNormals[idx.vertex_index].z += faceNormal.z;
            }
        }
    }

    // Normalize vertex normals
    for (auto& normal : vertexNormals) {
        vec3_normalize(normal);
    }

    // Second pass: compute colors and store offsets only for faces using this material
    const float offsetMagnitude = 0.0001f;
    std::vector<Vec3> vertexOffsets(attrib.vertices.size() / 3, Vec3(0, 0, 0));

    for (const auto& shape : shapes) {
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            if (shape.mesh.material_ids[f] != material_id) {
                continue;  // Skip faces not using this material
            }

            unsigned int fv = static_cast<unsigned int>(shape.mesh.num_face_vertices[f]);

            for (unsigned int v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[f * fv + v];
                if (idx.texcoord_index < 0 || idx.vertex_index < 0) {
                    std::cout << "Invalid index encountered: vertex_index=" << idx.vertex_index
                             << ", texcoord_index=" << idx.texcoord_index << std::endl;
                    continue;
                }

                float u = attrib.texcoords[2 * idx.texcoord_index + 0];
                float v_cord = attrib.texcoords[2 * idx.texcoord_index + 1];

                // Flip V coordinate
                v_cord = 1.0f - v_cord;

                Vec3 color = sampleTexture(texture, u, v_cord);

                // Apply gamma correction
                color.x = std::pow(color.x / 255.0f, 2.2f);
                color.y = std::pow(color.y / 255.0f, 2.2f);
                color.z = std::pow(color.z / 255.0f, 2.2f);

                vertexColors[idx.vertex_index] = color;
                texturedVertices++;

                // Store offset along vertex normal
                Vec3& normal = vertexNormals[idx.vertex_index];
                vertexOffsets[idx.vertex_index].x += normal.x * offsetMagnitude;
                vertexOffsets[idx.vertex_index].y += normal.y * offsetMagnitude;
                vertexOffsets[idx.vertex_index].z += normal.z * offsetMagnitude;
            }
        }
    }

    std::cout << "Total textured vertices for material " << material.name 
              << ": " << texturedVertices << " out of " << vertexColors.size() << std::endl;
}