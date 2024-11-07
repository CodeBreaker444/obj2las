#include "vertexcolors.h"
#include <cmath>
#include <vector>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <iostream>
#include <numeric>
#define gamma_percent 0.68f
void computeVertexColorsFromTextures(
    const tinyobj::attrib_t &attrib,
    const std::vector<tinyobj::shape_t> &shapes,
    const tinyobj::material_t &material,
    const int material_id,
    const std::map<std::string, Texture> &textures,
    std::vector<Vec3> &vertexColors)
{
    // std::cout << "Normal: computeVertexColorsFromTextures" << std::endl;

    float gamma = 1.0f / gamma_percent;

    if (attrib.texcoords.empty())
    {
        std::cout << "No texture coordinates found in the OBJ file." << std::endl;
        return;
    }
    std::cout << "Number of material_id: " << material_id << std::endl;

    for (const auto &shape : shapes)
    {
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
        {
            auto textureIt = textures.find(material.diffuse_texname);
            unsigned int fv = static_cast<unsigned int>(shape.mesh.num_face_vertices[f]);

            if (shape.mesh.material_ids[f] != material_id)
            {
                // std::cout << "Material ID mismatch: " << shape.mesh.material_ids[f] << " != " << material_id << std::endl;
                continue;
            }
            const auto &texture = textureIt->second;
            for (unsigned int vert = 0; vert < fv; vert++)
            {
                tinyobj::index_t idx = shape.mesh.indices[f * fv + vert];

                if (idx.texcoord_index < 0 || idx.vertex_index < 0)
                    continue;

                float tex_u = attrib.texcoords[2 * idx.texcoord_index + 0];
                float tex_v = attrib.texcoords[2 * idx.texcoord_index + 1];
                // Flip V coordinate
                tex_v = 1.0f - tex_v;
                Vec3 color = sampleTexture(texture, tex_u, tex_v);
                // Apply gamma correction gamma=2.2 in percentage is 0.4545 ,so to increase 10% gamma=1.1 and

                float gamma = 1.0f / gamma_percent;
                color.x = std::pow(color.x / 255.0f, gamma);
                color.y = std::pow(color.y / 255.0f, gamma);
                color.z = std::pow(color.z / 255.0f, gamma);

                // colorSums[idx.vertex_index] = colorSums[idx.vertex_index] + color;
                // colorCounts[idx.vertex_index]++;
                vertexColors[idx.vertex_index] = color;
                // if(idx.vertex_index==405){
                //     //print all the details
                //     std::cout << "idx: " << idx.vertex_index << " tex_u: " << tex_u << " tex_v: " << tex_v << std::endl;
                //     std::cout << "color: " << color.x << " " << color.y << " " << color.z << std::endl;
                //     std::cout << "material id: " << material_id<< material.diffuse_texname << std::endl;
                //     // continue;
                // }


            }
        }
    }

    std::cout << "Computed " << vertexColors.size() << " vertex colors." << std::endl;
    // applied gamma correction
    std::cout << "Gamma correction applied: " << gamma_percent * 100 << "%, Gamma Value:" << gamma << std::endl;
    // std::cout << "Unique colors: " << uniqueColors.size() << std::endl;
}