#include "../include/las.h"
#include "../include/texture.h"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <iomanip>
#define TINYOBJ_USE_DOUBLE
#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_DOUBLE
// #define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "include/tiny_obj_loader.h"
#include <chrono>
#include <cfloat>
#include <fstream>
#define v_id 405

#define VERSION "1.0.0a"

struct GlobalToLocalTransform {
    double global_x_offset = 0.0;
    double global_y_offset = 0.0;
    double global_z_offset = 0.0;
    double scale = 1.0;
    bool needs_transform = false;
    void saveTransformInfo(const std::string& filename) const {
        std::ofstream file(filename);
        if (file.is_open()) {
            file << std::setprecision(12) 
                 << "global_x_offset " << global_x_offset << "\n"
                 << "global_y_offset " << global_y_offset << "\n"
                 << "global_z_offset " << global_z_offset << "\n"
                 << "scale " << scale;
        }
    }
};

GlobalToLocalTransform computeGlobalToLocalTransform(const tinyobj::attrib_t& attrib) {
    GlobalToLocalTransform transform;
    
    double min_x = DBL_MAX, max_x = -DBL_MAX;
    double min_y = DBL_MAX, max_y = -DBL_MAX;
    double min_z = DBL_MAX, max_z = -DBL_MAX;
    
    for (size_t v = 0; v < attrib.vertices.size(); v += 3) {
        double x = attrib.vertices[v];
        double y = attrib.vertices[v + 1];
        double z = attrib.vertices[v + 2];
        // std::cout << "computeglobal: x: " << x << " y: " << y << " z: " << z << std::endl;
        
        min_x = std::min(min_x, x);
        max_x = std::max(max_x, x);
        min_y = std::min(min_y, y);
        max_y = std::max(max_y, y);
        min_z = std::min(min_z, z);
        max_z = std::max(max_z, z);
    }

    transform.global_x_offset = -std::round(min_x / 1000.0) * 1000.0;
    transform.global_y_offset = -std::round(min_y / 1000.0) * 1000.0;
    transform.global_z_offset = 0.0;
    // if offsets are gt 0, no need to transform
    if (0-transform.global_x_offset != 0.0 || 0-transform.global_y_offset != 0.0) {
        transform.needs_transform = true;
        std::cout << "<--Transform needed-->"  << std::endl;
    }else{
        transform.needs_transform = false;
        std::cout << "<--Transform not needed-->" << std::endl;
        return transform;

    }



    std::cout << "\nCoordinate Analysis:" << std::endl;
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Original bounds:" << std::endl;
    std::cout << "X: " << min_x << " to " << max_x << std::endl;
    std::cout << "Y: " << min_y << " to " << max_y << std::endl;
    std::cout << "Z: " << min_z << " to " << max_z << std::endl;
    std::cout << "\nComputed shifts:" << std::endl;
    std::cout << "X shift: " << transform.global_x_offset << std::endl;
    std::cout << "Y shift: " << transform.global_y_offset << std::endl;
    std::cout << "Z shift: " << transform.global_z_offset << std::endl;

    double shifted_min_x = min_x + transform.global_x_offset;
    double shifted_max_x = max_x + transform.global_x_offset;
    double shifted_min_y = min_y + transform.global_y_offset;
    double shifted_max_y = max_y + transform.global_y_offset;
    std::cout << "\nExpected bounds after transformation:" << std::endl;
    std::cout << "X: " << shifted_min_x << " to " << shifted_max_x << std::endl;
    std::cout << "Y: " << shifted_min_y << " to " << shifted_max_y << std::endl;
    std::cout << "Z: " << min_z << " to " << max_z << " (unchanged)" << std::endl;
    
    return transform;
}
void applyGlobalToLocalTransform(double& x, double& y, const GlobalToLocalTransform& transform) {
    static double min_positive_x = DBL_MAX;
    static double min_positive_y = DBL_MAX;
    
    if (transform.needs_transform) {
        // Apply offsets
        x = x + transform.global_x_offset;
        y = y + transform.global_y_offset;
        
        // Keep track of minimum positive values
        if (x > 0) min_positive_x = std::min(min_positive_x, x);
        if (y > 0) min_positive_y = std::min(min_positive_y, y);
        
        // Replace negatives with minimum positive values
        if (x < 0) x = min_positive_x;
        if (y < 0) y = min_positive_y;
        
    }
}


// Helper function to get file extension
std::string getFileExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of(".");
    return (dotPos == std::string::npos) ? "" : filename.substr(dotPos);
}

// Helper function to get file name without extension
std::string getFileNameWithoutExtension(const std::string& filename) {
    size_t lastSlash = filename.find_last_of("/\\");
    size_t lastDot = filename.find_last_of(".");
    if (lastSlash == std::string::npos) lastSlash = 0;
    else lastSlash++;
    if (lastDot == std::string::npos || lastDot < lastSlash) lastDot = filename.length();
    return filename.substr(lastSlash, lastDot - lastSlash);
    
}
void computeVertexColorsFromTextures(
    const tinyobj::attrib_t& attrib,
    const std::vector<tinyobj::shape_t>& shapes,
    const tinyobj::material_t& material,
    const int material_id,
    const std::map<std::string, Texture>& textures,
    std::vector<Vec3>& vertexColors) {

    // static std::vector<Vec3> colorSums;
    // static std::vector<int> colorCounts;
    static bool firstMaterial = true;
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

    auto textureIt = textures.find(material.diffuse_texname);
    if (textureIt == textures.end()) {
        std::cout << "Texture not found: " << material.diffuse_texname << std::endl;
        Vec3 materialColor(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
        for (const auto& shape : shapes) {
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                if (shape.mesh.material_ids[f] == material_id) {
                    // unsigned int fv = shape.mesh.num_face_vertices[f];
                    // for (unsigned int vert = 0; vert < fv; vert++) {
                    //     tinyobj::index_t idx = shape.mesh.indices[f * fv + vert];
                    //     if (idx.vertex_index >= 0) {
                    //         colorSums[idx.vertex_index] = colorSums[idx.vertex_index] + materialColor;
                    //         colorCounts[idx.vertex_index]++;
                    //     }
                    // }
                }
            }
        }
        return;
    }
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

    int texturedVertices = 0;
        // std::vector<Vec3> uniqueColors;

    for (const auto& shape : shapes) {
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            if (shape.mesh.material_ids[f] != material_id) {
                // std::cout << "Material ID mismatch: " << shape.mesh.material_ids[f] << " != " << material_id << std::endl;
                continue;
            }
    const auto& texture = textureIt->second;

            unsigned int fv = shape.mesh.num_face_vertices[f];
            for (unsigned int vert = 0; vert < fv; vert++) {
                tinyobj::index_t idx = shape.mesh.indices[f * fv + vert];
               
                if (idx.texcoord_index < 0 || idx.vertex_index < 0) continue;

                float tex_u = attrib.texcoords[2 * idx.texcoord_index + 0];
                float tex_v = attrib.texcoords[2 * idx.texcoord_index + 1];
           // Flip V coordinate
                tex_v = 1.0f - tex_v;
                Vec3 color = sampleTexture(texture, tex_u, tex_v);
                                // Apply gamma correction
                color.x = std::pow(color.x / 255.0f, 2.2f);
                color.y = std::pow(color.y / 255.0f, 2.2f);
                color.z = std::pow(color.z / 255.0f, 2.2f);

                // colorSums[idx.vertex_index] = colorSums[idx.vertex_index] + color;
                // colorCounts[idx.vertex_index]++;
                 vertexColors[idx.vertex_index] = color;

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
    

        std::cout << "Computed " << vertexColors.size() << " vertex colors." << std::endl;
        // std::cout << "Unique colors: " << uniqueColors.size() << std::endl;
}
std::vector<Vec3> computeVertexColorsFromTexturesOriginal(
    const tinyobj::attrib_t& attrib,
    const std::vector<tinyobj::shape_t>& shapes,
    const std::vector<tinyobj::material_t>& materials,
    const std::map<std::string, Texture>& textures) {

    std::vector<Vec3> vertexColors(attrib.vertices.size() / 3, Vec3(1, 1, 1));
    std::vector<Vec3> vertexNormals(attrib.vertices.size() / 3, Vec3(0, 0, 0));

    if (attrib.texcoords.empty()) {
        std::cout << "No texture coordinates found in the OBJ file." << std::endl;
        return vertexColors;
    }

    std::cout << "Number of shapes: " << shapes.size() << std::endl;
    std::cout << "Number of materials: " << materials.size() << std::endl;
    std::cout << "Number of textures: " << textures.size() << std::endl;

    int texturedVertices = 0;

    // // Helper functions for vector operations
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

    // // First pass: compute vertex normals
    // for (const auto& shape : shapes) {
    //     for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
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

    // Second pass: compute colors and store offsets
    // const float offsetMagnitude = 0.0001f; // Adjust this value as needed
    // std::vector<Vec3> vertexOffsets(attrib.vertices.size() / 3, Vec3(0, 0, 0));

    for (const auto& shape : shapes) {
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            unsigned int fv = static_cast<unsigned int>(shape.mesh.num_face_vertices[f]);
            int materialId = shape.mesh.material_ids[f];

            if (materialId < 0 || materialId >= static_cast<int>(materials.size())) {
                // std::cout << "Invalid material ID: " << materialId << std::endl;
                continue;
            }

            const auto& material = materials[materialId];
            auto textureIt = textures.find(material.diffuse_texname);

            if (textureIt == textures.end()) {
                std::cout << "Texture not found: " << material.diffuse_texname << std::endl;
                Vec3 materialColor(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
                for (unsigned int v = 0; v < fv; v++) {
                    tinyobj::index_t idx = shape.mesh.indices[f * fv + v];
                    vertexColors[idx.vertex_index] = materialColor;
                }
                continue;
            }

            const auto& texture = textureIt->second;

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
                // Vec3& normal = vertexNormals[idx.vertex_index];
                // vertexOffsets[idx.vertex_index].x += normal.x * offsetMagnitude;
                // vertexOffsets[idx.vertex_index].y += normal.y * offsetMagnitude;
                // vertexOffsets[idx.vertex_index].z += normal.z * offsetMagnitude;
                //   if (idx.vertex_index==v_id){
                //     std::cout << "idx: " << idx.vertex_index << " u: " << u << " v: " << v_cord << std::endl;
                //     // print color
                //     std::cout << "color: " << color.x << " " << color.y << " " << color.z << std::endl;
                //     // print material
                //     std::cout << "material: " << materialId << material.diffuse_texname<< std::endl;
                //     // print texture

                //     return vertexColors;
                // }
            
            }
        }

    }

    std::cout << "Total textured vertices: " << texturedVertices << " out of " << vertexColors.size() << std::endl;

    return vertexColors;
}
void convertObjToLas(const std::string& objFilename, const std::string& lasFilename) {
    try {
        tinyobj::ObjReaderConfig reader_config;
        reader_config.mtl_search_path = getParentPath(objFilename); // Path to material files
        // add time to measure the time
        auto start = std::chrono::high_resolution_clock::now();
        std::cout << R"(
       _     _ ____  _
  ___ | |__ (_)___ \| | __ _ ___
 / _ \| '_ \| | __) | |/ _` / __|
| (_) | |_) | |/ __/| | (_| \__ \
 \___/|_.__// |_____|_|\__,_|___/
          |__/            v1.0.0a
        << OBJ to LAS Converter >>
        << by: @codebreaker444 >>
        << github.com/codebreaker44/obj2las >>
        )" << std::endl;
        std::cout << "Version: " << VERSION << std::endl;
        std::cout << "Loading OBJ file: " << objFilename << std::endl;

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(objFilename, reader_config)) {
            if (!reader.Error().empty()) {
                throw std::runtime_error("TinyObjReader: " + reader.Error());
            }
            throw std::runtime_error("Failed to load OBJ file.");
        }

        if (!reader.Warning().empty()) {
            std::cout << "Obj Reader: " << reader.Warning() << std::endl;
        }
        // time in seconds
        std::cout << "Time taken to load the obj file: " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count() << "s" << std::endl;

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();
        // Compute global to local transformation
        GlobalToLocalTransform transform = computeGlobalToLocalTransform(attrib);

        // Save transformation parameters for future reference
        std::string transformFile = getFileNameWithoutExtension(lasFilename) + "_transform.txt";
        if (transform.needs_transform)
        {
            std::cout << "Need translation and file saved to: " << transformFile << std::endl;
        }
        transform.saveTransformInfo(transformFile);
        LAS13Writer writer;
        if (!writer.open(lasFilename)) {
            throw std::runtime_error("Failed to open LAS file for writing: " + lasFilename);
        }
        std::vector<Vec3> vertexColors;
                std::vector<Vec3> vertexColorsoriginal;

        // Load all textures
        std::map<std::string, Texture> textures;
        // std::map<std::string, Texture> texturesoriginal;
        int material_id = 1;
        for (const auto& material : materials) {
            if (!material.diffuse_texname.empty()) {
                std::string texturePath = joinPaths(getParentPath(objFilename), material.diffuse_texname);
                textures[material.diffuse_texname] = loadTexture(texturePath);
                // texturesoriginal[material.diffuse_texname] = textures[material.diffuse_texname];
                computeVertexColorsFromTextures(attrib, shapes, material, material_id, textures, vertexColors);
                material_id++;
                // reset textures
                textures.clear();
                
                std::cout << "Loaded texture reset: " << textures.size() << std::endl;
                // std::cout << "Original Loaded texture reset: " << texturesoriginal.size() << std::endl;

                std::cout << "Loaded texture: " << material.diffuse_texname << std::endl;
            }
        }

        std::cout << "Loaded " << textures.size() << " textures." << std::endl;

        // // Compute vertex colors using the provided textures
        // vertexColorsoriginal = computeVertexColorsFromTexturesOriginal(attrib, shapes, materials, texturesoriginal);
        // // // save original colors and vertex colors in txt files
        // // std::ofstream originalcolors("originalcolors.txt");
        // // std::ofstream colors("colors.txt");
        // // for (size_t i = 0; i < vertexColors.size(); i++) {
        // //     originalcolors << vertexColorsoriginal[i].x << " " << vertexColorsoriginal[i].y << " " << vertexColorsoriginal[i].z << std::endl;
        // //     colors << vertexColors[i].x << " " << vertexColors[i].y << " " << vertexColors[i].z << std::endl;
        // // }
        // // originalcolors.close();
        // // colors.close();

        // std::cout << "Computed " << vertexColors.size() << " vertex colors." << std::endl;
        // // replace vertexColors with vertexColorsoriginal
        // vertexColors = vertexColorsoriginal;
for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    // std::cout << "Processing vertex " << v + 0 << " of " << attrib.vertices.size() / 3 << std::endl;
    // std::cout << "Processing vertex " << v + 1 << " of " << attrib.vertices.size() / 3 << std::endl;
    // std::cout << "Processing vertex " << v + 2 << " of " << attrib.vertices.size() / 3 << std::endl;
    // // print 
    double x = attrib.vertices[3 * v + 0];
    double y = attrib.vertices[3 * v + 1];
    double z = attrib.vertices[3 * v + 2];


    // Apply a threshold to very dark colors
    float threshold = 0.01f; // Adjust this value as needed
    float r = std::max(vertexColors[v].x, threshold);
    float g = std::max(vertexColors[v].y, threshold);
    float b = std::max(vertexColors[v].z, threshold);

    // Convert to 16-bit color values
    uint16_t r16 = static_cast<uint16_t>(r * 65535);
    uint16_t g16 = static_cast<uint16_t>(g * 65535);
    uint16_t b16 = static_cast<uint16_t>(b * 65535);
    // print x,y,z
    // std::cout << "x: " << x << " y: " << y << " z: " << z << std::endl;  


    applyGlobalToLocalTransform(x, y, transform);
    // std::cout << "x: " << x << " y: " << y << " z: " << z << std::endl;  
    // return;
    writer.addPointColor(x, y, z, r16, g16, b16);
}
        writer.close();

        std::cout << "Conversion complete. LAS file saved as: " << lasFilename << std::endl;
        std::cout << "Total vertices processed: " << attrib.vertices.size() / 3 << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error during conversion: " << e.what() << std::endl;
        std::cerr << "OBJ file: " << objFilename << std::endl;
        std::cerr << "LAS file: " << lasFilename << std::endl;
    }
}


int main(int argc, char* argv[]) {
    // start timer and clock memory usage
    auto start_full = std::chrono::high_resolution_clock::now();

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.obj> <output.las>" << std::endl;
        return 1;
    }

    std::string objFilename = argv[1];
    std::string lasFilename = argv[2];

    convertObjToLas(objFilename, lasFilename);
    // print timer
    std::cout << "Total time taken: " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_full).count() << "s" << std::endl;

    return 0;
}