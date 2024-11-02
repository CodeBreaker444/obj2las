#ifndef VERTEX_COLORS_H
#define VERTEX_COLORS_H
#include "texture.h"
// First, include tiny_obj_loader.h with include guards
#ifndef TINYOBJLOADER_IMPLEMENTATION
// If you haven't defined TINYOBJLOADER_IMPLEMENTATION elsewhere
#include "tiny_obj_loader.h"
#endif

#include <vector>
#include <map>
#include <string>



// Main function declaration with full tinyobj structure access
void computeVertexColorsFromTextures(
    const tinyobj::attrib_t& attrib,
    const std::vector<tinyobj::shape_t>& shapes,
    const tinyobj::material_t& material,
    const int material_id,
    const std::map<std::string, Texture>& textures,
    std::vector<Vec3>& vertexColors);

#endif // VERTEX_COLORS_H