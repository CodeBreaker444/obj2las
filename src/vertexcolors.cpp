#include "vertexcolors.h"
#include <cmath>
#include <vector>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <iostream>
#include <numeric>

#define GAMMA_PERCENT 0.68f

void computeVertexColorsFromTextures(
    const tinyobj::attrib_t& attrib,
    const std::vector<tinyobj::shape_t>& shapes,
    const tinyobj::material_t& material,
    const int material_id,
    const std::map<std::string, Texture>& textures,
    std::vector<Vec3>& vertexColors)
{
    // Early returns for invalid conditions
    if (attrib.texcoords.empty()) {
        std::cout << "No texture coordinates found in the OBJ file." << std::endl;
        return;
    }

    auto textureIt = textures.find(material.diffuse_texname);
    if (textureIt == textures.end()) {
        return;
    }

    std::cout << "Number of material_id: " << material_id << std::endl;
    const float gamma = 1.0f / GAMMA_PERCENT;
    const auto& texture = textureIt->second;

    // Pre-calculate gamma correction lookup table
    constexpr size_t TABLE_SIZE = 256;
    static float gammaTable[TABLE_SIZE];
    static bool tableInitialized = false;
    
    if (!tableInitialized) {
        for (size_t i = 0; i < TABLE_SIZE; ++i) {
            gammaTable[i] = std::pow(i / 255.0f, gamma);
        }
        tableInitialized = true;
    }

    for (const auto& shape : shapes) {
        const size_t numFaces = shape.mesh.num_face_vertices.size();
        const auto& materialIds = shape.mesh.material_ids;
        const auto& indices = shape.mesh.indices;
        
        for (size_t f = 0; f < numFaces; ++f) {
            // Skip faces with different material
            if (materialIds[f] != material_id) {
                continue;
            }

            const unsigned int fv = shape.mesh.num_face_vertices[f];
            const size_t index_offset = f * fv;

            for (unsigned int v = 0; v < fv; ++v) {
                const tinyobj::index_t& idx = indices[index_offset + v];
                
                // Skip invalid indices
                if (idx.texcoord_index < 0 || idx.vertex_index < 0) {
                    continue;
                }

                // Calculate texture coordinates
                const size_t tc_idx = 2 * idx.texcoord_index;
                const float tex_u = attrib.texcoords[tc_idx];
                const float tex_v = 1.0f - attrib.texcoords[tc_idx + 1]; // Flip V coordinate

                // Sample texture and apply gamma correction
                Vec3 color = sampleTexture(texture, tex_u, tex_v);
                
                // Use lookup table for gamma correction
                color.x = gammaTable[static_cast<int>(color.x)];
                color.y = gammaTable[static_cast<int>(color.y)];
                color.z = gammaTable[static_cast<int>(color.z)];

                vertexColors[idx.vertex_index] = color;
            }
        }
    }

    std::cout << "Computed " << vertexColors.size() << " vertex colors." << std::endl;
    std::cout << "Gamma correction applied: " << GAMMA_PERCENT * 100 << "%, Gamma Value:" << gamma << std::endl;
}