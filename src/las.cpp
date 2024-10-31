#include "include/las.h"
#include <cstring>
#include <ctime>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include "las.h"
#include <limits>

void LAS13Writer::initializeHeader() {
    std::memset(&header, 0, sizeof(LASHeader));
    std::memcpy(header.fileSignature, "LASF", 4);
    header.versionMajor = 1;
    header.versionMinor = 3;
    header.headerSize = 235;
    header.offsetToPointData = 235;
    header.pointDataRecordFormat = 3;
    header.pointDataRecordLength = 34;  // Fixed size for Format 3
    header.xScaleFactor = 0.001;
    header.yScaleFactor = 0.001;
    header.zScaleFactor = 0.001;
    header.xOffset = 0;
    header.yOffset = 0;
    header.zOffset = 0;
    
    // Initialize bounding box to invalid values
    header.minX = header.minY = header.minZ = std::numeric_limits<double>::max();
    header.maxX = header.maxY = header.maxZ = -std::numeric_limits<double>::max();
    
    std::string software = "OBJ to LAS Converter";
    std::memcpy(header.generatingSoftware, software.c_str(), std::min(software.length(), size_t(32)));

    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    header.fileCreationDayOfYear = now->tm_yday + 1;
    header.fileCreationYear = now->tm_year + 1900;

    isFirstPoint = true;
}

void LAS13Writer::updateHeaderBounds(double x, double y, double z) {
    header.minX = std::min(header.minX, x);
    header.maxX = std::max(header.maxX, x);
    header.minY = std::min(header.minY, y);
    header.maxY = std::max(header.maxY, y);
    header.minZ = std::min(header.minZ, z);
    header.maxZ = std::max(header.maxZ, z);
    
    if (isFirstPoint) {
        header.xOffset = x;
        header.yOffset = y;
        header.zOffset = z;
        isFirstPoint = false;
    }
}

void LAS13Writer::writeHeader() {
    file.seekp(0);
    file.write(reinterpret_cast<char*>(&header), sizeof(LASHeader));
    if (file.fail()) {
        throw std::runtime_error("Failed to write LAS header");
    }
}

void LAS13Writer::writePoints() {
    file.seekp(header.offsetToPointData);
    file.write(pointBuffer.data(), pointBuffer.size());
    if (file.fail()) {
        throw std::runtime_error("Failed to write point data");
    }
}

bool LAS13Writer::open(const std::string& fname) {
    filename = fname;
    file.open(filename, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    initializeHeader();
    return true;
}


void LAS13Writer::addPointColor(double x, double y, double z, uint16_t r, uint16_t g, uint16_t b) {
    updateHeaderBounds(x, y, z);
    
    int32_t ix = static_cast<int32_t>((x - header.xOffset) / header.xScaleFactor);
    int32_t iy = static_cast<int32_t>((y - header.yOffset) / header.yScaleFactor);
    int32_t iz = static_cast<int32_t>((z - header.zOffset) / header.zScaleFactor);

    char point[34] = {0};  // Initialize to zero
    std::memcpy(point, &ix, 4);
    std::memcpy(point + 4, &iy, 4);
    std::memcpy(point + 8, &iz, 4);
    // Skip intensity (2 bytes)
    point[14] = 0x01;  // Return Number (1) and Number of Returns (1)
    // Skip classification, scan angle rank, user data, point source ID, and GPS time
    std::memcpy(point + 28, &r, 2);
    std::memcpy(point + 30, &g, 2);
    std::memcpy(point + 32, &b, 2);

    pointBuffer.insert(pointBuffer.end(), point, point + 34);
    header.numberOfPointRecords++;
}

void LAS13Writer::close() {
    try {
        writeHeader();
        writePoints();
        file.flush();
        if (file.fail()) {
            throw std::runtime_error("Failed to flush file buffer");
        }
        file.close();
        if (file.fail()) {
            throw std::runtime_error("Failed to close file");
        }

        // Verify file size
        std::ifstream verifyFile(filename, std::ios::binary | std::ios::ate);
        if (!verifyFile.is_open()) {
            throw std::runtime_error("Failed to open file for verification");
        }
        std::streamsize fileSize = verifyFile.tellg();
        std::streamsize expectedSize = header.offsetToPointData + (header.numberOfPointRecords * header.pointDataRecordLength);
        if (fileSize != expectedSize) {
            throw std::runtime_error("File size mismatch. Expected: " + std::to_string(expectedSize) + ", Actual: " + std::to_string(fileSize));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in close(): " << e.what() << std::endl;
        throw;
    }

    // Debug prints
    std::cout << "\nLAS File Header Information:" << std::endl;
    std::cout << "File Signature: " << std::string(header.fileSignature, 4) << std::endl;
    std::cout << "Version: " << (int)header.versionMajor << "." << (int)header.versionMinor << std::endl;
    std::cout << "Header Size: " << header.headerSize << std::endl;
    std::cout << "Point Data Record Format: " << (int)header.pointDataRecordFormat << std::endl;
    std::cout << "Point Data Record Length: " << header.pointDataRecordLength << std::endl;
    std::cout << "Number of Point Records: " << header.numberOfPointRecords << std::endl;
    std::cout << "Scale Factors (X, Y, Z): " << header.xScaleFactor << ", " 
              << header.yScaleFactor << ", " << header.zScaleFactor << std::endl;
    std::cout << "Offset (X, Y, Z): " << header.xOffset << ", " 
              << header.yOffset << ", " << header.zOffset << std::endl;
    std::cout << "Min Bounds (X, Y, Z): " << header.minX << ", " 
              << header.minY << ", " << header.minZ << std::endl;
    std::cout << "Max Bounds (X, Y, Z): " << header.maxX << ", " 
              << header.maxY << ", " << header.maxZ << std::endl;

    std::cout << "\nPoint Data Size: " << pointBuffer.size() << " bytes" << std::endl;
    std::cout << "Expected Point Data Size: " << header.numberOfPointRecords * header.pointDataRecordLength << " bytes" << std::endl;

    std::cout << "\nFirst 100 bytes of point data:" << std::endl;
    for (size_t i = 0; i < std::min(size_t(100), pointBuffer.size()); ++i) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex << (int)(unsigned char)pointBuffer[i] << " ";
        if ((i + 1) % 34 == 0) std::cout << std::endl;
    }
    std::cout << std::dec << std::endl;
}