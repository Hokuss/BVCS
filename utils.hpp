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
using namespace std;
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

string sha256(const string& input);
string sha256(const vector<uint8_t>& input);
vector<uint8_t> lz4Compress(const uint8_t* input, size_t inputSize);
vector<uint8_t> lz4Decompress(const uint8_t* input, size_t inputSize);
void copy(const fs::path& source, const fs::path& destination, copy_options options = copy_options::None);
vector<string> splitstring(const string& str, char delimiter);
vector<uint8_t> readBinaryFile(const string& filepath);

#endif // UTILS_H