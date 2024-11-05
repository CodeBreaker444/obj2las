#include "vertexcolors.h"
#include <cmath>
#include <vector>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <iostream>
#include <numeric>
void computeVertexColorsFromTextures(
    const tinyobj::attrib_t& attrib,
    const std::vector<tinyobj::shape_t>& shapes,
    const tinyobj::material_t& material,
    const int material_id,
    const std::map<std::string, Texture>& textures,
    std::vector<Vec3>& vertexColors) {

    // Initialize colors only on first call
    if (vertexColors.empty()) {
        vertexColors.resize(attrib.vertices.size() / 3);
    }

    if (attrib.texcoords.empty()) {
        std::cout << "No texture coordinates found in the OBJ file." << std::endl;
        return;
    }

    // Process texture
    auto textureIt = textures.find(material.diffuse_texname);
    if (textureIt == textures.end()) {
        std::cout << "No texture found for material: " << material.name << std::endl;
        return;
    }

    const auto& texture = textureIt->second;
    
    // Temporary vectors for this material only
    std::vector<Vec3> materialColorSums(attrib.vertices.size() / 3, Vec3());
    std::vector<int> materialColorCounts(attrib.vertices.size() / 3, 0);
    int totalVertices = 0;
    
    // Process vertices for this material
    for (const auto& shape : shapes) {
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            if (shape.mesh.material_ids[f] != material_id) {
                continue;
            }

            unsigned int fv = shape.mesh.num_face_vertices[f];
            for (unsigned int vert = 0; vert < fv; vert++) {
                tinyobj::index_t idx = shape.mesh.indices[f * fv + vert];
                if (idx.texcoord_index < 0) continue;  // Skip if no texture coordinates

                // Use exact same texture coordinate sampling as original
                float u = attrib.texcoords[2 * idx.texcoord_index + 0];
                float v = attrib.texcoords[2 * idx.texcoord_index + 1];

                // Sample texture identical to original code
                Vec3 color = sampleTexture(texture, u, v);
                
                // Accumulate colors exactly as original did
                materialColorSums[idx.vertex_index] = materialColorSums[idx.vertex_index] + color;
                materialColorCounts[idx.vertex_index]++;
                totalVertices++;
            }
        }
    }

    // Apply colors for this material
    int processedVertices = 0;
    for (size_t i = 0; i < vertexColors.size(); i++) {
        if (materialColorCounts[i] > 0) {
            // Average colors exactly as original did
            Vec3 avgColor = materialColorSums[i] * (1.0f / materialColorCounts[i]);
            
            // Apply gamma correction exactly as original
            avgColor.x = pow(avgColor.x / 255.0f, 0.4545f);
            avgColor.y = pow(avgColor.y / 255.0f, 0.4545f);
            avgColor.z = pow(avgColor.z / 255.0f, 0.4545f);

            // Store the color
            if (vertexColors[i].x != 0 || vertexColors[i].y != 0 || vertexColors[i].z != 0) {
                // If vertex already has color, blend
                vertexColors[i].x = (vertexColors[i].x + avgColor.x) * 0.5f;
                vertexColors[i].y = (vertexColors[i].y + avgColor.y) * 0.5f;
                vertexColors[i].z = (vertexColors[i].z + avgColor.z) * 0.5f;
            } else {
                vertexColors[i] = avgColor;
            }
            processedVertices++;
        }
    }

    std::cout << "Material " << material.name << " statistics:" << std::endl
              << "  Total vertices processed: " << totalVertices << std::endl
              << "  Unique vertices processed: " << processedVertices << std::endl;
}