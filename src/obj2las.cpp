#define TINYOBJ_USE_DOUBLE
#include "include/las.h"
#include "include/texture.h"
#include "include/shiftcoords.h"
#include "include/vertexcolors.h"
#include "include/exceptions.h"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <cfloat>
#include <fstream>
#include <algorithm>

#define VERSION "1.0.0a"
using namespace obj2las;

// Helper functions remain the same
std::string getFileExtension(const std::string& filename) {
    size_t dotPos = filename.find_last_of(".");
    return (dotPos == std::string::npos) ? "" : filename.substr(dotPos);
}

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
        // Check if input file exists
        std::ifstream objFile(objFilename);
        if (!objFile.good()) {
            throw FileIOException("Input file does not exist or is not accessible: " + objFilename);
        }

        tinyobj::ObjReaderConfig reader_config;
        int material_id;
        reader_config.mtl_search_path = getParentPath(objFilename);
        
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
                throw ObjParsingException("TinyObjReader: " + reader.Error());
            }
            throw ObjParsingException("Failed to load OBJ file.");
        }

        if (!reader.Warning().empty()) {
            ErrorLogger::logWarning(reader.Warning(), "ObjReader");
        }

        std::cout << "Time taken to load the obj file: " 
                  << std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::high_resolution_clock::now() - start).count() 
                  << "s" << std::endl;

        auto& attrib = reader.GetAttrib();
        if (attrib.vertices.empty()) {
            throw ObjParsingException("No vertices found in OBJ file");
        }

        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();

        // Compute transformation
        GlobalToLocalTransform transform;
        try {
            transform = computeGlobalToLocalTransform(attrib);
        } catch (const std::exception& e) {
            throw TransformationException(e.what());
        }

               std::string transformFile = getFileNameWithoutExtension(lasFilename) + "_transform.txt";
        if (transform.needs_transform)
        {
            std::cout << "Need translation and file saved to: " << transformFile << std::endl;
            transform.saveTransformInfo(transformFile);  // Remove the if check since it's void
        }

        LAS13Writer writer;
        if (!writer.open(lasFilename)) {
            throw FileIOException("Failed to open LAS file for writing: " + lasFilename);
        }

        std::vector<Vec3> vertexColors(attrib.vertices.size() / 3, Vec3(1, 1, 1));
        std::map<std::string, Texture> textures;

        // Fixed materials size check
        if (materials.empty()) {
            std::cout << "No materials found in the OBJ file." << std::endl;
        } else if (materials.size() > 200) {  // Changed from materials.size() > 0
            throw ObjParsingException("Too many materials in OBJ file (limit: 200)");
        }

        std::cout << "Number of materials: " << materials.size() << std::endl;
        material_id = 0;

        for (const auto& material : materials) {
            try {
                std::cout << "Processing material: " << material.name << std::endl;
                if (!material.diffuse_texname.empty()) {
                    std::string texturePath = joinPaths(getParentPath(objFilename), material.diffuse_texname);
                    
                    // Check if texture file exists
                    std::ifstream textureFile(texturePath);
                    if (!textureFile.good()) {
                        throw TextureLoadException("Texture file not found: " + texturePath);
                    }
                    
                    textures[material.diffuse_texname] = loadTexture(texturePath);
                    computeVertexColorsFromTextures(attrib, shapes, material, material_id, textures, vertexColors);
                    textures.clear();
                }
                material_id++;
            } catch (const std::exception& e) {
                ErrorLogger::logWarning("Failed to process material '" + material.name + "': " + e.what());
                continue;
            }
        }

        size_t totalVertices = attrib.vertices.size() / 3;
        for (size_t v = 0; v < totalVertices; v++) {
            if (v % 1000000 == 0) {
                ErrorLogger::logProgress(v, totalVertices, "Processing vertices");
            }

            double x = attrib.vertices[3 * v + 0];
            double y = attrib.vertices[3 * v + 1];
            double z = attrib.vertices[3 * v + 2];

            float threshold = 0.01f;
            float r = std::max(vertexColors[v].x, threshold);
            float g = std::max(vertexColors[v].y, threshold);
            float b = std::max(vertexColors[v].z, threshold);

            

            uint16_t r16 = static_cast<uint16_t>(r * 65535);
            uint16_t g16 = static_cast<uint16_t>(g * 65535);
            uint16_t b16 = static_cast<uint16_t>(b * 65535);

            applyGlobalToLocalTransform(x, y, transform);
            writer.addPointColor(x, y, z, r16, g16, b16);
        }

        writer.close();
        std::cout << "Conversion complete. LAS file saved as: " << lasFilename << std::endl;
        std::cout << "Total vertices processed: " << totalVertices << std::endl;

    } catch (const Obj2LasException& e) {
        ErrorLogger::logError(typeid(e).name(), e.what(), objFilename, lasFilename);
        throw;
    } catch (const std::exception& e) {
        ErrorLogger::logError("StandardException", e.what(), objFilename, lasFilename);
        throw Obj2LasException(e.what());
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " <input.obj> <output.las>" << std::endl;
            return EXIT_FAILURE;
        }

        std::string objFilename = argv[1];
        std::string lasFilename = argv[2];

        auto start_full = std::chrono::high_resolution_clock::now();

        convertObjToLas(objFilename, lasFilename);

        std::cout << "Total execution time: " 
                  << std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::high_resolution_clock::now() - start_full).count() 
                  << "s" << std::endl;

        return EXIT_SUCCESS;

    } catch (const Obj2LasException& e) {
        std::cerr << "Error during conversion: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return EXIT_FAILURE;
    }
}