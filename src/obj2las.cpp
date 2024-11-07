#define TINYOBJ_USE_DOUBLE
// #define TINYOBJLOADER_IMPLEMENTATION
// #define TINYOBJLOADER_USE_DOUBLE
#include "include/las.h"
#include "include/texture.h"
#include "include/shiftcoords.h"
#include "include/vertexcolors.h"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <cfloat>
#include <fstream>
#include <algorithm>

#define VERSION "1.0.0a"

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
std::vector<Vec3> computeVertexColorsFromTexturesOriginal(
    const tinyobj::attrib_t& attrib,
    const std::vector<tinyobj::shape_t>& shapes,
    const std::vector<tinyobj::material_t>& materials,
    const std::map<std::string, Texture>& textures) {
std::cout << "Original: computeVertexColorsFromTexturesOriginal" << std::endl;
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

    // First pass: compute vertex normals
    for (const auto& shape : shapes) {
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
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

    // Second pass: compute colors and store offsets
    const float offsetMagnitude = 0.0001f; // Adjust this value as needed
    std::vector<Vec3> vertexOffsets(attrib.vertices.size() / 3, Vec3(0, 0, 0));

    for (const auto& shape : shapes) {
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            unsigned int fv = static_cast<unsigned int>(shape.mesh.num_face_vertices[f]);
            int materialId = shape.mesh.material_ids[f];

            if (materialId < 0 || materialId >= static_cast<int>(materials.size())) {
                std::cout << "Invalid material ID: " << materialId << std::endl;
                continue;
            }

            const auto& material = materials[materialId];
            auto textureIt = textures.find(material.diffuse_texname);

            if (textureIt == textures.end()) {
                std::cout << "Texture not found..: " << material.diffuse_texname << std::endl;
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
                // if(idx.vertex_index==405){
                //     //print all the details
                //     std::cout << "idx: " << idx.vertex_index << " tex_u: " << u << " tex_v: " << v_cord << std::endl;
                //     std::cout << "color: " << color.x << " " << color.y << " " << color.z << std::endl;
                //     std::cout << "material id: " << materialId << std::endl;
                //     // material name
                //     std::cout << "material name: " << material.diffuse_texname << std::endl;
                // }

                // Store offset along vertex normal
                Vec3& normal = vertexNormals[idx.vertex_index];
                vertexOffsets[idx.vertex_index].x += normal.x * offsetMagnitude;
                vertexOffsets[idx.vertex_index].y += normal.y * offsetMagnitude;
                vertexOffsets[idx.vertex_index].z += normal.z * offsetMagnitude;
            }
        }
    }

    std::cout << "Total textured vertices: " << texturedVertices << " out of " << vertexColors.size() << std::endl;

    return vertexColors;
}
void convertObjToLas(const std::string& objFilename, const std::string& lasFilename) {
    try {
        tinyobj::ObjReaderConfig reader_config;
        int material_id;
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
    std::vector<Vec3> vertexColors(attrib.vertices.size() / 3, Vec3(1, 1, 1));
                // std::vector<Vec3> vertexColorsoriginal;

        // Load all textures
        std::map<std::string, Texture> textures;
        // std::map<std::string, Texture> texturesoriginal;
        if(materials.size() == 0) {
            std::cout << "No materials found in the OBJ file." << std::endl;
            return;
        } 
        // print materials size
        std::cout << "Number of materials: " << materials.size() << std::endl;
        material_id = 0;
        for (const auto& material : materials) {
            std::cout << "Material name: " << material.name << std::endl;
            if (!material.diffuse_texname.empty()) {

                std::string texturePath = joinPaths(getParentPath(objFilename), material.diffuse_texname);
                textures[material.diffuse_texname] = loadTexture(texturePath);
                std::cout << "Loaded texture before func: " << material.diffuse_texname << std::endl;
            
                // texturesoriginal[material.diffuse_texname] = textures[material.diffuse_texname];
                computeVertexColorsFromTextures(attrib, shapes, material, material_id, textures, vertexColors);
                // reset textures
                textures.clear();
                
                std::cout << "Loaded texture reset: " << textures.size() << std::endl;
                // std::cout << "Original Loaded texture reset: " << texturesoriginal.size() << std::endl;

                std::cout << "Loaded texture: " << material.diffuse_texname << std::endl;
            

        }                    material_id++;


        }

        std::cout << "Loaded " << textures.size()<< material_id << " textures." << std::endl;

        // Compute vertex colors using the provided textures
        // vertexColorsoriginal = computeVertexColorsFromTexturesOriginal(attrib, shapes, materials, texturesoriginal);
        // // save original colors and vertex colors in txt files
        // exit(0);
        // std::ofstream originalcolors("originalcolors.txt");
        // std::ofstream colors("colors.txt");
        // for (size_t i = 0; i < vertexColors.size(); i++) {
        //     originalcolors << vertexColorsoriginal[i].x << " " << vertexColorsoriginal[i].y << " " << vertexColorsoriginal[i].z << std::endl;
        //     colors << vertexColors[i].x << " " << vertexColors[i].y << " " << vertexColors[i].z << std::endl;
        // }
        // originalcolors.close();
        // colors.close();

        // std::cout << "Computed " << vertexColors.size() << " vertex colors." << std::endl;
        // replace vertexColors with vertexColorsoriginal
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