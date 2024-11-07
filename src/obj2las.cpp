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

        // Load all textures
        std::map<std::string, Texture> textures;
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
                // std::cout << "Loaded texture before func: " << material.diffuse_texname << std::endl;
            
                computeVertexColorsFromTextures(attrib, shapes, material, material_id, textures, vertexColors);
                // reset textures
                textures.clear();
                // std::cout << "Loaded texture reset: " << textures.size() << std::endl;
                // std::cout << "Original Loaded texture reset: " << texturesoriginal.size() << std::endl;

        }
        material_id++;
        }

        std::cout << "Loaded " << textures.size()<< material_id << " textures." << std::endl;

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