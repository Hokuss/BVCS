#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <set>
#include <cstdlib>
// using namespace std;
namespace fs = std::filesystem;

enum class copy_options : uint8_t {
    None = 0,
    Recursive = 1 << 1,
    Overwrite_existing = 1 << 2,
    Overwrite_inner = 1 << 3,
    Skip_inner = 1 << 4
};

inline constexpr copy_options operator|(copy_options a, copy_options b) {
    using T = std::underlying_type_t<copy_options>;
    return static_cast<copy_options>(
        static_cast<T>(a) | static_cast<T>(b)
    );
}

inline constexpr copy_options operator&(copy_options a, copy_options b) {
    using T = std::underlying_type_t<copy_options>;
    return static_cast<copy_options>(
        static_cast<T>(a) & static_cast<T>(b)
    );
}

inline constexpr copy_options& operator|=(copy_options& a, copy_options b) {
    return a = a | b;
}

std::string sha256(const std::string& input);
std::string sha256(const std::vector<uint8_t>& input);
std::vector<uint8_t> lz4Compress(const uint8_t* input, size_t inputSize);
std::vector<uint8_t> lz4Decompress(const uint8_t* input, size_t inputSize);
void copy(const fs::path& source, const fs::path& destination, copy_options options = copy_options::None);
std::vector<std::string> splitstring(const std::string& str, char delimiter);
std::vector<uint8_t> readBinaryFile(const std::string& filepath);
bool isTextFile(const std::string& filepath);
std::string readTextFile(const std::string& filepath);
std::string readIgnoreFile();
std::vector<std::string> all_branches();
std::vector<std::string> all_versions(const std::string &branch_name);
std::string exec_and_capture_hidden(const std::string& command);


#endif // UTILS_H