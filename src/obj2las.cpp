#include "../include/las.h"
#include "../include/texture.h"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <iomanip>
#define TINYOBJ_USE_DOUBLE
#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_DOUBLE
#include "include/tiny_obj_loader.h"
#include <chrono>
#include <cfloat>
#include <fstream>
#include <algorithm>

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

void computeVertexNormals(
    const tinyobj::attrib_t& attrib,
    const std::vector<tinyobj::shape_t>& shapes,
    std::vector<Vec3>& vertexNormals) {
    
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

    for (const auto& shape : shapes) {
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            unsigned int fv = static_cast<unsigned int>(shape.mesh.num_face_vertices[f]);
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

            for (unsigned int v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[f * fv + v];
                vertexNormals[idx.vertex_index].x += faceNormal.x;
                vertexNormals[idx.vertex_index].y += faceNormal.y;
                vertexNormals[idx.vertex_index].z += faceNormal.z;
            }
        }
    }

    for (auto& normal : vertexNormals) {
        vec3_normalize(normal);
    }
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
std::vector<Vec3> processTextureVertexColors(
    const tinyobj::attrib_t& attrib,
    const std::vector<tinyobj::shape_t>& shapes,
    const std::vector<tinyobj::material_t>& materials,
    const Texture& texture,
    const std::string& textureName,
    std::vector<bool>& processedVertices) {

    std::vector<Vec3> vertexColors(attrib.vertices.size() / 3, Vec3(1, 1, 1));
    int texturedVertices = 0;

    for (const auto& shape : shapes) {
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int materialId = shape.mesh.material_ids[f];
            
            if (materialId < 0 || materialId >= static_cast<int>(materials.size())) {
                continue;
            }

            const auto& material = materials[materialId];
            if (material.diffuse_texname != textureName) {
                continue;
            }

            unsigned int fv = static_cast<unsigned int>(shape.mesh.num_face_vertices[f]);
            for (unsigned int v = 0; v < fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[f * fv + v];
                if (idx.texcoord_index < 0 || idx.vertex_index < 0) {
                    continue;
                }

                float u = attrib.texcoords[2 * idx.texcoord_index + 0];
                float v_cord = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];

                Vec3 color = sampleTexture(texture, u, v_cord);

                color.x = std::pow(color.x / 255.0f, 2.2f);
                color.y = std::pow(color.y / 255.0f, 2.2f);
                color.z = std::pow(color.z / 255.0f, 2.2f);

                vertexColors[idx.vertex_index] = color;
                processedVertices[idx.vertex_index] = true;
                texturedVertices++;
            }
        }
    }

    std::cout << "Processed " << texturedVertices << " vertices for texture: " << textureName << std::endl;
    return vertexColors;
}

std::vector<Vec3> computeVertexColorsFromTextures(
    const tinyobj::attrib_t& attrib,
    const std::vector<tinyobj::shape_t>& shapes,
    const std::vector<tinyobj::material_t>& materials,
    const std::string& objPath) {

    std::vector<Vec3> finalVertexColors(attrib.vertices.size() / 3, Vec3(1, 1, 1));
    std::vector<bool> processedVertices(attrib.vertices.size() / 3, false);
    std::vector<Vec3> vertexNormals(attrib.vertices.size() / 3, Vec3(0, 0, 0));

    if (attrib.texcoords.empty()) {
        std::cout << "No texture coordinates found in the OBJ file." << std::endl;
        return finalVertexColors;
    }

    // Compute vertex normals first
    computeVertexNormals(attrib, shapes, vertexNormals);

    // Process each material/texture one at a time
    for (const auto& material : materials) {
        if (material.diffuse_texname.empty()) {
            continue;
        }

        std::string texturePath = joinPaths(objPath, material.diffuse_texname);
        std::cout << "Loading and processing texture: " << material.diffuse_texname << std::endl;
        
        // Load single texture
        Texture currentTexture = loadTexture(texturePath);
        if (currentTexture.data.empty()) {
            std::cout << "Failed to load texture: " << texturePath << std::endl;
            continue;
        }

        // Process current texture
        std::vector<Vec3> currentTextureColors = processTextureVertexColors(
            attrib, shapes, materials, currentTexture, material.diffuse_texname, processedVertices);

        // Merge colors with final result
        for (size_t i = 0; i < finalVertexColors.size(); i++) {
            if (processedVertices[i]) {
                finalVertexColors[i] = currentTextureColors[i];
            }
        }

        // Clear texture data
        currentTexture.data.clear();
        currentTexture.data.shrink_to_fit();
        
        std::cout << "Finished processing texture: " << material.diffuse_texname << std::endl;
    }

    // Apply vertex normal offsets
    // const float offsetMagnitude = 0.0001f;
        std::vector<Vec3> vertexOffsets(attrib.vertices.size() / 3, Vec3(0, 0, 0));

    for (size_t i = 0; i < finalVertexColors.size(); i++) {
        if (processedVertices[i]) {
            // Vec3& normal = vertexNormals[i];
            // Offset vertices slightly along their normals if needed
            // (This part remains unchanged from your original implementation)

        }
    }

    int totalProcessed = std::count(processedVertices.begin(), processedVertices.end(), true);
    std::cout << "Total textured vertices: " << totalProcessed << " out of " << finalVertexColors.size() << std::endl;

    return finalVertexColors;
}

GlobalToLocalTransform computeGlobalToLocalTransform(const tinyobj::attrib_t& attrib) {
    GlobalToLocalTransform transform;
    
    double min_x = DBL_MAX, max_x = -DBL_MAX;
    double min_y = DBL_MAX, max_y = -DBL_MAX;
    double min_z = DBL_MAX, max_z = -DBL_MAX;
    
    for (size_t v = 0; v < attrib.vertices.size(); v += 3) {
        double x = attrib.vertices[v];
        double y = attrib.vertices[v + 1];
        double z = attrib.vertices[v + 2];
        
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

    if (0-transform.global_x_offset != 0.0 || 0-transform.global_y_offset != 0.0) {
        transform.needs_transform = true;
        std::cout << "<--Transform needed-->" << std::endl;
    } else {
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
    
    return transform;
}

void applyGlobalToLocalTransform(double& x, double& y, const GlobalToLocalTransform& transform) {
    static double min_positive_x = DBL_MAX;
    static double min_positive_y = DBL_MAX;
    
    if (transform.needs_transform) {
        x = x + transform.global_x_offset;
        y = y + transform.global_y_offset;
        
        if (x > 0) min_positive_x = std::min(min_positive_x, x);
        if (y > 0) min_positive_y = std::min(min_positive_y, y);
        
        if (x < 0) x = min_positive_x;
        if (y < 0) y = min_positive_y;
    }
}

void convertObjToLas(const std::string& objFilename, const std::string& lasFilename) {
    try {
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
        
        tinyobj::ObjReaderConfig reader_config;
        reader_config.mtl_search_path = getParentPath(objFilename);

        tinyobj::ObjReader reader;
        if (!reader.ParseFromFile(objFilename, reader_config)) {
            if (!reader.Error().empty()) {
                throw std::runtime_error("TinyObjReader: " + reader.Error());
            }
            throw std::runtime_error("Failed to load OBJ file.");
        }

        if (!reader.Warning().empty()) {
            std::cout << "OBJ Reader Warning: " << reader.Warning() << std::endl;
        }

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();

        GlobalToLocalTransform transform = computeGlobalToLocalTransform(attrib);
        std::string transformFile = getFileNameWithoutExtension(lasFilename) + "_transform.txt";
        transform.saveTransformInfo(transformFile);

        LAS13Writer writer;
        if (!writer.open(lasFilename)) {
            throw std::runtime_error("Failed to open LAS file for writing: " + lasFilename);
        }

        // Process textures and compute vertex colors
        std::vector<Vec3> vertexColors = computeVertexColorsFromTextures(
            attrib, shapes, materials, getParentPath(objFilename));

        // Process vertices
        for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
            double x = attrib.vertices[3 * v + 0];
            double y = attrib.vertices[3 * v + 1];
            double z = attrib.vertices[3 * v + 2];

            float threshold = 0.01f;
float r = std::max(vertexColors[v].x, threshold);
            float g = std::max(vertexColors[v].y, threshold);
            float b = std::max(vertexColors[v].z, threshold);

            // Convert to 16-bit color values
            uint16_t r16 = static_cast<uint16_t>(r * 65535);
            uint16_t g16 = static_cast<uint16_t>(g * 65535);
            uint16_t b16 = static_cast<uint16_t>(b * 65535);

            applyGlobalToLocalTransform(x, y, transform);
            writer.addPointColor(x, y, z, r16, g16, b16);
        }

        writer.close();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

        std::cout << "Conversion complete. LAS file saved as: " << lasFilename << std::endl;
        std::cout << "Total vertices processed: " << attrib.vertices.size() / 3 << std::endl;
        std::cout << "Total time taken: " << duration << "s" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error during conversion: " << e.what() << std::endl;
        std::cerr << "OBJ file: " << objFilename << std::endl;
        std::cerr << "LAS file: " << lasFilename << std::endl;
    }
}

int main(int argc, char* argv[]) {
    auto start_full = std::chrono::high_resolution_clock::now();

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.obj> <output.las>" << std::endl;
        return 1;
    }

    std::string objFilename = argv[1];
    std::string lasFilename = argv[2];

    convertObjToLas(objFilename, lasFilename);

    std::cout << "Total execution time: " 
              << std::chrono::duration_cast<std::chrono::seconds>(
                     std::chrono::high_resolution_clock::now() - start_full).count() 
              << "s" << std::endl;

    return 0;
}