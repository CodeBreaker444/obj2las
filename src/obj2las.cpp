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
#define TINYOBJ_USE_DOUBLE
#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_DOUBLE
#include "include/tiny_obj_loader.h"
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