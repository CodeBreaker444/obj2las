#pragma once

#include <string>
#include <vector>
#include <map>
// #include "../src/texture.cpp"

struct Vec3 {
    float x, y, z;
    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
};

struct Texture {
    int width, height, channels;
    std::vector<unsigned char> data;
};

std::string getParentPath(const std::string& path);
std::string joinPaths(const std::string& path1, const std::string& path2);
bool fileExists(const std::string& filename);
Texture loadTexture(const std::string& filename);
Vec3 sampleTexture(const Texture& texture, float u, float v);

// New function to load multiple textures
std::map<std::string, Texture> loadTextures(const std::string& mtlFilename);