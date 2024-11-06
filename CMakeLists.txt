cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
# Specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(obj2las VERSION 1.0 LANGUAGES CXX)

# set src and header directories
include_directories(include)
include_directories(src)




# Add source files
set(SOURCES
    src/obj2las.cpp
    src/las.cpp
    src/texture.cpp
)

# Add header files in include directory
set(HEADERS
    include/las.h
    include/texture.h
    include/tiny_obj_loader.h
    include/stb_image.h
)

# Create the executable
add_executable(obj2las ${SOURCES} ${HEADERS})

# Add include directories
target_include_directories(obj2las PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Add compile options
target_compile_options(obj2las PRIVATE 
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

# Custom target for running the converter
add_custom_target(run
    COMMAND $<TARGET_FILE:obj2las>
    DEPENDS obj2las
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running obj2las"
)
# ~/Downloads/data-pillar/3d_models_final/pila_08_obj.obj
# c

# Test target
add_custom_target(test
    COMMAND $<TARGET_FILE:obj2las> 
            ~/Downloads/data-pillar/capsule.obj
            ${CMAKE_BINARY_DIR}/output_capsule.las
    DEPENDS obj2las
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running test with sample files"
)
add_custom_target(test_complex
    COMMAND $<TARGET_FILE:obj2las> 
            ~/Downloads/data-pillar/3d_models_final/pila_08_obj.obj
            ${CMAKE_BINARY_DIR}/output_pointcloud_pila_modified.las
    DEPENDS obj2las
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running test with sample files"
)
add_custom_target(test_complex_shift
    COMMAND $<TARGET_FILE:obj2las> 
            ~/Downloads/data-pillar-original/pila_08_obj.obj
            ${CMAKE_BINARY_DIR}/output_pointcloud_pila_shifted.las
    DEPENDS obj2las
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running test with sample files"
)
add_custom_target(test_complex_textures
    COMMAND $<TARGET_FILE:obj2las> 
            ~/Downloads/TR0500-SD-OA00-PT0-V02/TR0500-SD-OA00-PT0-V02.obj
            ${CMAKE_BINARY_DIR}/output_pointcloud_bridge.las
    DEPENDS obj2las
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running test with sample files"
)
add_custom_target(clean-all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/clean-all.cmake
    COMMENT "Cleaning all build artifacts"
)
# Install target
install(TARGETS obj2las DESTINATION bin)

# clean build
