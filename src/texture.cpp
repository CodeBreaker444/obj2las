#include "include/texture.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define brightnessIncrease 1.03f  // 5% increase


std::map<std::string, Texture> textureCache;
template<typename T>
T lerp(T a, T b, float t) {
    return a + t * (b - a);
}
std::string getParentPath(const std::string& path) {
    size_t found = path.find_last_of("/\\");
    return (found != std::string::npos) ? path.substr(0, found) : "";
}

std::string joinPaths(const std::string& path1, const std::string& path2) {
    if (path1.empty()) return path2;
    if (path2.empty()) return path1;
    char lastChar = path1[path1.length() - 1];
    if (lastChar == '/' || lastChar == '\\') {
        return path1 + path2;
    }
    return path1 + "/" + path2;
}

bool fileExists(const std::string& filename) {
    std::ifstream file(filename.c_str());
    return file.good();
}

Texture loadTexture(const std::string& filename) {
    Texture texture;
    if (textureCache.find(filename) != textureCache.end()) {
        return textureCache[filename];
    }

    if (!fileExists(filename)) {
        std::cerr << "Texture file not found: " << filename << std::endl;
        return texture;
    }
    std::cout << "Loading texture: " << filename << std::endl;

    unsigned char* data = stbi_load(filename.c_str(), &texture.width, &texture.height, &texture.channels, 3);
    // std::cout << "Texture channels: " << texture.channels << data << std::endl;
    if (data) {
        texture.data.assign(data, data + texture.width * texture.height * 3);
        stbi_image_free(data);
        textureCache[filename] = texture;
        std::cout << "Texture loaded successfully: " << filename << std::endl;
    } else {
        std::cerr << "Failed to load texture: " << filename << std::endl;
    }
    return texture;
}

Vec3 sampleTexture(const Texture& texture, float u, float v) {
    // Ensure u and v are in [0, 1] range
    u = std::fmod(u, 1.0f);
    v = std::fmod(v, 1.0f);
    if (u < 0) u += 1.0f;
    if (v < 0) v += 1.0f;
    
    // Convert to pixel coordinates
    int x = static_cast<int>(u * (texture.width - 1));
    int y = static_cast<int>(v * (texture.height - 1));
    // PRINT X, Y, WIDTH, HEIGHT

    // Ensure x and y are within bounds
    x = std::max(0, std::min(x, texture.width - 1));
    y = std::max(0, std::min(y, texture.height - 1));

    // PRINT X, Y, WIDTH, HEIGHT

    // Calculate the index in the image data array
    int index = (y * texture.width + x) * texture.channels;
    // PRINT INDEX
    

    // Sample the color
    Vec3 color;
    if (texture.channels >= 3) {

        color.x = static_cast<float>(texture.data[index + 0]);
        color.y = static_cast<float>(texture.data[index + 1]);
        color.z = static_cast<float>(texture.data[index + 2]);
        
    } else if (texture.channels == 1) {
        float value = static_cast<float>(texture.data[index]);
        color.x = color.y = color.z = value;
    } else {
        std::cout << "Unsupported number of channels: " << texture.channels << std::endl;
        return Vec3(0, 0, 0);
    }
    // check if color is within the range of 0-255
    if (color.x < 0 || color.x > 255) {
        std::cout << "Color x out of range: " << color.x << std::endl;
    }
    if (color.y < 0 || color.y > 255) {
        std::cout << "Color y out of range: " << color.y << std::endl;
    }
    if (color.z < 0 || color.z > 255) {
        std::cout << "Color z out of range: " << color.z << std::endl;
    }
   
    if (color.x < 200 && color.y < 200 && color.z < 200) {
        color.x *= brightnessIncrease;
        color.y *= brightnessIncrease;
        color.z *= brightnessIncrease;
    }

    return color;
}

std::map<std::string, Texture> loadTextures(const std::string& mtlFilename) {
    std::map<std::string, Texture> textures;
    std::ifstream mtlFile(mtlFilename);
    if (!mtlFile.is_open()) {
        std::cerr << "Failed to open MTL file: " << mtlFilename << std::endl;
        return textures;
    }

    std::string line, currentMaterial;
    std::string mtlDir = getParentPath(mtlFilename);

    while (std::getline(mtlFile, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "newmtl") {
            iss >> currentMaterial;
        } else if (token == "map_Kd" && !currentMaterial.empty()) {
            std::string textureFilename;
            iss >> textureFilename;
            std::string fullPath = joinPaths(mtlDir, textureFilename);
            
            if (fileExists(fullPath)) {
                textures[currentMaterial] = loadTexture(fullPath);
                std::cout << "Loaded texture: " << fullPath << std::endl;
            } else {
                std::cerr << "Texture file not found: " << fullPath << std::endl;
            }
        }
    }

    return textures;
}