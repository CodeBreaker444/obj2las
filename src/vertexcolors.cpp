#include "vertexcolors.h"
#include <cmath>
#include <vector>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <iostream>
#include <numeric>
#define gamma_percent 0.65f
void computeVertexColorsFromTextures(
    const tinyobj::attrib_t& attrib,
    const std::vector<tinyobj::shape_t>& shapes,
    const tinyobj::material_t& material,
    const int material_id,
    const std::map<std::string, Texture>& textures,
    std::vector<Vec3>& vertexColors) {
std::cout << "Normal: computeVertexColorsFromTextures" << std::endl;

    // static std::vector<Vec3> colorSums;
    // static std::vector<int> colorCounts;
    static bool firstMaterial = true;
    float gamma = 1.0f / gamma_percent;
    // std::vector<Vec3> vertexNormals(attrib.vertices.size() / 3, Vec3(0, 0, 0));

    // Helper functions for vector operations
    // auto vec3_subtract = [](const Vec3& a, const Vec3& b) -> Vec3 {
    //     return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
    // };

    // auto vec3_cross = [](const Vec3& a, const Vec3& b) -> Vec3 {
    //     return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    // };

    // auto vec3_normalize = [](Vec3& v) {
    //     float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    //     if (length > 0) {
    //         v.x /= length;
    //         v.y /= length;
    //         v.z /= length;
    //     }
    // };
    if (firstMaterial) {
        // colorSums.resize(attrib.vertices.size() / 3, Vec3());
        // colorCounts.resize(attrib.vertices.size() / 3, 0);
        vertexColors.resize(attrib.vertices.size() / 3);
        firstMaterial = false;
    }

    if (attrib.texcoords.empty()) {
        std::cout << "No texture coordinates found in the OBJ file." << std::endl;
        return;
    }
    std::cout << "Number of material_id: " << material_id << std::endl;

    // if (textureIt == textures.end()) {
    //     std::cout << "Texture not found: " << material.diffuse_texname << std::endl;
    //     Vec3 materialColor(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
    //     for (const auto& shape : shapes) {
    //         for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
    //             if (shape.mesh.material_ids[f] == material_id) {
    //                 // unsigned int fv = shape.mesh.num_face_vertices[f];
    //                 // for (unsigned int vert = 0; vert < fv; vert++) {
    //                 //     tinyobj::index_t idx = shape.mesh.indices[f * fv + vert];
    //                 //     if (idx.vertex_index >= 0) {
    //                 //         colorSums[idx.vertex_index] = colorSums[idx.vertex_index] + materialColor;
    //                 //         colorCounts[idx.vertex_index]++;
    //                 //     }
    //                 // }
    //             }
    //         }
    //     }
    //     return;
    // }
    // First pass: compute vertex normals
    // for (const auto& shape : shapes) {
    //     for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
    //          if (shape.mesh.material_ids[f] != material_id) {
    //             // std::cout << "Material ID mismatch: " << shape.mesh.material_ids[f] << " != " << material_id << std::endl;
    //             continue;
    //         }
    //         unsigned int fv = static_cast<unsigned int>(shape.mesh.num_face_vertices[f]);
 
    //         // Compute face normal
    //         Vec3 v0, v1, v2;
    //         for (unsigned int v = 0; v < fv; v++) {
    //             tinyobj::index_t idx = shape.mesh.indices[f * fv + v];
    //             float vx = attrib.vertices[3 * idx.vertex_index + 0];
    //             float vy = attrib.vertices[3 * idx.vertex_index + 1];
    //             float vz = attrib.vertices[3 * idx.vertex_index + 2];
    //             if (v == 0) v0 = Vec3(vx, vy, vz);
    //             if (v == 1) v1 = Vec3(vx, vy, vz);
    //             if (v == 2) v2 = Vec3(vx, vy, vz);
    //         }
    //         Vec3 faceNormal = vec3_cross(vec3_subtract(v1, v0), vec3_subtract(v2, v0));
    //         vec3_normalize(faceNormal);

    //         // Accumulate face normal to vertex normals
    //         for (unsigned int v = 0; v < fv; v++) {
    //             tinyobj::index_t idx = shape.mesh.indices[f * fv + v];
    //             vertexNormals[idx.vertex_index].x += faceNormal.x;
    //             vertexNormals[idx.vertex_index].y += faceNormal.y;
    //             vertexNormals[idx.vertex_index].z += faceNormal.z;
    //         }
    //     }
    // }

    // // Normalize vertex normals
    // for (auto& normal : vertexNormals) {
    //     vec3_normalize(normal);
    // }
    // const float offsetMagnitude = 0.0001f; // Adjust this value as needed
    // std::vector<Vec3> vertexOffsets(attrib.vertices.size() / 3, Vec3(0, 0, 0));

        // std::vector<Vec3> uniqueColors;
    unsigned int texturedVertices = 0;
    for (const auto& shape : shapes) {
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            auto textureIt = textures.find(material.diffuse_texname);
            // auto textureIt = textures.find(material.diffuse_texname);
            unsigned int fv = static_cast<unsigned int>(shape.mesh.num_face_vertices[f]);

            if (shape.mesh.material_ids[f] != material_id) {
                // std::cout << "Material ID mismatch: " << shape.mesh.material_ids[f] << " != " << material_id << std::endl;
                continue;
            } 

            const auto& texture = textureIt->second;


            for (unsigned int vert = 0; vert < fv; vert++) {
                tinyobj::index_t idx = shape.mesh.indices[f * fv + vert];
               
                if (idx.texcoord_index < 0 || idx.vertex_index < 0) continue;

                float tex_u = attrib.texcoords[2 * idx.texcoord_index + 0];
                float tex_v = attrib.texcoords[2 * idx.texcoord_index + 1];
           // Flip V coordinate
                tex_v = 1.0f - tex_v;
                Vec3 color = sampleTexture(texture, tex_u, tex_v);
                                // Apply gamma correction gamma=2.2 in percentage is 0.4545 ,so to increase 10% gamma=1.1 and 
                
                color.x = std::pow(color.x / 255.0f, 2.2f);
                color.y = std::pow(color.y / 255.0f, 2.2f);
                color.z = std::pow(color.z / 255.0f, 2.2f);

                // colorSums[idx.vertex_index] = colorSums[idx.vertex_index] + color;
                // colorCounts[idx.vertex_index]++;
                 vertexColors[idx.vertex_index] = color;
                if(idx.vertex_index==405){
                    //print all the details
                    std::cout << "idx: " << idx.vertex_index << " tex_u: " << tex_u << " tex_v: " << tex_v << std::endl;
                    std::cout << "color: " << color.x << " " << color.y << " " << color.z << std::endl;
                    std::cout << "material id: " << material_id<< material.diffuse_texname << std::endl;
                    // continue;    
                }

                // Store offset along vertex normal
                // Vec3& normal = vertexNormals[idx.vertex_index];
                // vertexOffsets[idx.vertex_index].x += normal.x * offsetMagnitude;
                // vertexOffsets[idx.vertex_index].y += normal.y * offsetMagnitude;
                // vertexOffsets[idx.vertex_index].z += normal.z * offsetMagnitude;
        
                //  if (idx.vertex_index==v_id){
                //     std::cout << "idx: " << idx.vertex_index << " tex_u: " << tex_u << " tex_v: " << tex_v << std::endl;
                //     // print color
                //     std::cout << "color: " << color.x << " " << color.y << " " << color.z << std::endl;
                //     // print material id
                //     std::cout << "material id: " << material_id << std::endl;
                //     return;
                // }
                texturedVertices++;
          
            }

        }
    

    // if (material_id == static_cast<int>(textures.size()) - 1) {
    //     for (size_t i = 0; i < vertexColors.size(); i++) {
    //         if (colorCounts[i] > 0) {
    //             vertexColors[i] = colorSums[i] * (1.0f / colorCounts[i]);
    //             vertexColors[i].x = pow(vertexColors[i].x / 255.0f, 0.4545f);
    //             vertexColors[i].y = pow(vertexColors[i].y / 255.0f, 0.4545f);
    //             vertexColors[i].z = pow(vertexColors[i].z / 255.0f, 0.4545f);
    //         }
    //     }
        
        // for (const auto& color : vertexColors) {
        //     bool found = false;
        //     for (const auto& unique : uniqueColors) {
        //         if (color.x == unique.x && color.y == unique.y && color.z == unique.z) {
        //             found = true;
        //             break;
        //         }
        //     }
        //     if (!found) {
        //         uniqueColors.push_back(color);
        //     }
        // }
    
        
    }
    

        std::cout << "Computed " << vertexColors.size() << " vertex colors." << texturedVertices << std::endl;
        // applied gamma correction
        std::cout << "Gamma correction applied: " << gamma_percent * 100  << "%" << gamma << std::endl;
        // std::cout << "Unique colors: " << uniqueColors.size() << std::endl;
}