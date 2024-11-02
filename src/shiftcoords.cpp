#include <cmath>
#include <vector>
#include "include/shiftcoords.h"
#include "include/tiny_obj_loader.h" // Add this line to include the full definition of tinyobj::attrib_t
#include <iostream>
#include <fstream>
#include <cfloat>



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
        
        // z = z; // Z unchanged
    }
}