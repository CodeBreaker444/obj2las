#ifndef SHIFTCOORDS_H
#define SHIFTCOORDS_H
#include <string>
#include <vector>
#include <fstream>
namespace tinyobj {
    struct attrib_t;
    struct shape_t;
    struct material_t;
}

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

GlobalToLocalTransform computeGlobalToLocalTransform(const tinyobj::attrib_t& attrib);
void applyGlobalToLocalTransform(double& x, double& y, const GlobalToLocalTransform& transform);

#endif // SHIFTCOORDS_H