#pragma once

#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
// #include "../src/las.cpp"

#pragma pack(push, 1)
struct LASHeader {
    char fileSignature[4];
    uint16_t fileSourceID;
    uint16_t globalEncoding;
    uint32_t projectIDGUID1;
    uint16_t projectIDGUID2;
    uint16_t projectIDGUID3;
    uint8_t projectIDGUID4[8];
    uint8_t versionMajor;
    uint8_t versionMinor;
    char systemIdentifier[32];
    char generatingSoftware[32];
    uint16_t fileCreationDayOfYear;
    uint16_t fileCreationYear;
    uint16_t headerSize;
    uint32_t offsetToPointData;
    uint32_t numberOfVariableLengthRecords;
    uint8_t pointDataRecordFormat;
    uint16_t pointDataRecordLength;
    uint32_t numberOfPointRecords;
    uint32_t numberOfPointsByReturn[5];
    double xScaleFactor;
    double yScaleFactor;
    double zScaleFactor;
    double xOffset;
    double yOffset;
    double zOffset;
    double maxX;
    double minX;
    double maxY;
    double minY;
    double maxZ;
    double minZ;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct LASPoint {
    int32_t x;
    int32_t y;
    int32_t z;
    uint16_t intensity;
    uint8_t returnNumber : 3;
    uint8_t numberOfReturns : 3;
    uint8_t scanDirectionFlag : 1;
    uint8_t edgeOfFlightLine : 1;
    uint8_t classification;
    int8_t scanAngleRank;
    uint8_t userData;
    uint16_t pointSourceID;
    double gpsTime;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
};
#pragma pack(pop)

class LAS13Writer {
public:
    bool open(const std::string& fname);
    void addPointColor(double x, double y, double z, uint16_t r, uint16_t g, uint16_t b);
    void close();

private:
    std::ofstream file;
    std::string filename;
    LASHeader header;
    bool isFirstPoint;



    std::vector<char> pointBuffer;

    void initializeHeader();
    void updateHeaderBounds(double x, double y, double z);
    void writeHeader();
    void writePoints();
};