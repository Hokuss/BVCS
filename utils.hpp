#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <iostream>
using namespace std;

string sha256(const string& input);
vector<uint8_t> lz4Compress(const uint8_t* input, size_t inputSize);
vector<uint8_t> lz4Decompress(const uint8_t* input, size_t inputSize);

#endif // UTILS_H