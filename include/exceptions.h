#ifndef OBJ2LAS_EXCEPTIONS_H
#define OBJ2LAS_EXCEPTIONS_H

#include <stdexcept>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <iostream>

namespace obj2las {

/**
 * @brief Base exception class for OBJ to LAS conversion errors
 */
class Obj2LasException : public std::runtime_error {
public:
    explicit Obj2LasException(const std::string& message) 
        : std::runtime_error(message) {}
};

/**
 * @brief Exception class for file input/output related errors
 */
class FileIOException : public Obj2LasException {
public:
    explicit FileIOException(const std::string& message) 
        : Obj2LasException("File I/O Error: " + message) {}
};

/**
 * @brief Exception class for OBJ file parsing errors
 */
class ObjParsingException : public Obj2LasException {
public:
    explicit ObjParsingException(const std::string& message) 
        : Obj2LasException("OBJ Parsing Error: " + message) {}
};

/**
 * @brief Exception class for texture loading and processing errors
 */
class TextureLoadException : public Obj2LasException {
public:
    explicit TextureLoadException(const std::string& message) 
        : Obj2LasException("Texture Loading Error: " + message) {}
};

/**
 * @brief Exception class for coordinate transformation errors
 */
class TransformationException : public Obj2LasException {
public:
    explicit TransformationException(const std::string& message) 
        : Obj2LasException("Transformation Error: " + message) {}
};

/**
 * @brief Utility class for logging errors with timestamp and context information
 */
class ErrorLogger {
public:
    /**
     * @brief Log an error with context information
     * @param errorType Type of the error
     * @param message Error message
     * @param objFile Optional OBJ filename
     * @param lasFile Optional LAS filename
     */
    static void logError(const std::string& errorType, 
                        const std::string& message, 
                        const std::string& objFile = "", 
                        const std::string& lasFile = "") {
        std::cerr << "\nError Details:" << std::endl;
        std::cerr << "Type: " << errorType << std::endl;
        std::cerr << "Message: " << message << std::endl;
        if (!objFile.empty()) std::cerr << "OBJ File: " << objFile << std::endl;
        if (!lasFile.empty()) std::cerr << "LAS File: " << lasFile << std::endl;
        std::cerr << "Timestamp: " << getCurrentTimestamp() << std::endl;
    }

    /**
     * @brief Log a warning message
     * @param message Warning message
     * @param context Optional context information
     */
    static void logWarning(const std::string& message, const std::string& context = "") {
        std::cout << "Warning: " << message;
        if (!context.empty()) {
            std::cout << " (" << context << ")";
        }
        std::cout << std::endl;
    }

    /**
     * @brief Log a progress update
     * @param current Current progress value
     * @param total Total expected value
     * @param operation Description of the operation
     */
    static void logProgress(size_t current, size_t total, const std::string& operation) {
        double percentage = (current * 100.0) / total;
        std::cout << operation << ": " << current << " of " << total 
                 << " (" << std::fixed << std::setprecision(2) << percentage 
                 << "%)" << std::endl;
    }

private:
    /**
     * @brief Get current timestamp as string
     * @return Formatted timestamp string
     */
    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

} // namespace obj2las

#endif // OBJ2LAS_EXCEPTIONS_H