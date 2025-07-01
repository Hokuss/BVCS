#include "utils.hpp"


const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

const uint32_t H[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

constexpr uint32_t MIN_MATCH_LENGTH = 4;
constexpr uint32_t MAX_DISTANCE = 0xFFFF; // 65535
constexpr uint32_t HASH_TABLE_SIZE = 65536; 

uint32_t right_rotate(uint32_t value, uint32_t amount) {
    return (value >> amount) | (value << (32 - amount));
}
 
uint32_t sigma0(uint32_t x) {
    return right_rotate(x, 2) ^ right_rotate(x, 13) ^ right_rotate(x, 22);
}

uint32_t sigma1(uint32_t x) {
    return right_rotate(x, 6) ^ right_rotate(x, 11) ^ right_rotate(x, 25);
}

uint32_t SIGMA0(uint32_t x) {
    return right_rotate(x, 7) ^ right_rotate(x, 18) ^ (x >> 3);
}

uint32_t SIGMA1(uint32_t x) {
    return right_rotate(x, 17) ^ right_rotate(x, 19) ^ (x >> 10);
}


uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

string sha256(const string& input) {
    vector<uint8_t> message(input.begin(), input.end());
    uint64_t original_length = message.size() * 8; // Length in bits

    // Padding
    message.push_back(0x80); // Append 1 bit
    while ((message.size() * 8 + 64) % 512 != 0) {
        message.push_back(0x00); // Append 0 bits
    }

    // Append original length in bits
    for (int i = 7; i >= 0; i--) {
        message.push_back((original_length >> (i * 8)) & 0xFF);
    }

    // Process message in 512-bit chunks
    vector<uint32_t> words(64);
    uint32_t h[8];
    for (int i = 0; i < 8; i++) {
        h[i] = H[i];
    }

    for (size_t i = 0; i < message.size(); i += 64) {
        for (int j = 0; j < 16; j++) {
            words[j] = (message[i + j * 4] << 24) | (message[i + j * 4 + 1] << 16) | (message[i + j * 4 + 2] << 8) | message[i + j * 4 + 3];
        }
        for (int j = 16; j < 64; j++) {
            words[j] = SIGMA1(words[j - 2]) + words[j - 7] + SIGMA0(words[j - 15]) + words[j - 16];
        }

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int j = 0; j < 64; j++) {
            uint32_t temp1 = hh + SIGMA1(e) + ch(e, f, g) + K[j] + words[j];
            uint32_t temp2 = SIGMA0(a) + maj(a, b, c);
            hh = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    // Convert hash to hexadecimal string
    stringstream ss;
    for (int i = 0; i < 8; i++) {
        ss << hex << setw(8) << setfill('0') << h[i];
    }
    return ss.str();
}

uint32_t hashFunc(const uint8_t* data) {
    return (data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)) * 2654435761U % HASH_TABLE_SIZE;
}

struct Match {
    size_t offset;
    size_t length;
};

/**
 * Find the best match for the current position in the input
 */
Match findBestMatch(const uint8_t* input, size_t inputSize, size_t currentPos, 
                   const vector<uint32_t>& matchesTable) {
    Match match = {0, 0};
    for(const uint32_t& matchPos : matchesTable) {
        if (matchPos >= currentPos || matchPos + MAX_DISTANCE < currentPos) {
            continue;
        }
        size_t length = 0;
        while (length < MAX_DISTANCE && currentPos + length < inputSize && matchPos + length < currentPos &&
               input[matchPos + length] == input[currentPos + length]) {
            length++;
        }
        if (length > match.length) {
            match.offset = static_cast<uint32_t>(currentPos - matchPos);
            match.length = length;
        }
    }
    
    return match;
}

vector<uint8_t> lz4Compress(const uint8_t* input, size_t inputSize) {
    if (!input || inputSize == 0)
        return {};
    
    vector<uint8_t> output;
    output.reserve(inputSize); // Worst case: no compression
    // Hash table to store positions of previously seen 4-byte sequences
    vector<vector<uint32_t>> hashTable(HASH_TABLE_SIZE, vector<uint32_t>());
    
    size_t pos = 0;
    size_t literalStart = 0;
    
    while (pos + MIN_MATCH_LENGTH <= inputSize) {
        // Update hash table for current position
        uint32_t hash = hashFunc(input + pos);
        
        // Find best match at current position
        Match match = findBestMatch(input, inputSize, pos, hashTable[hash]);
        
        // Update hash table with current position
        hashTable[hash].push_back(static_cast<uint32_t>(pos));

        // If a good match is found, encode it        
        if (match.length >= MIN_MATCH_LENGTH) {
            uint32_t literalLength = pos - literalStart;
            uint32_t matchOffset = match.offset;
            uint32_t matchLength = match.length;
           
            vector<uint8_t> literals = {input + literalStart, input + pos};
            uint8_t token = (min(literalLength,uint32_t(15)) << 4) | min((matchLength - MIN_MATCH_LENGTH),uint32_t(15));
            output.push_back(token);
            if(literalLength >= 15) {
                literalLength -= min(literalLength,uint32_t(15));
                while (literalLength >= 255) {
                    output.push_back(uint8_t(255));
                    literalLength -= 255;
                }
                if (literalLength >= 0) {
                    output.push_back(uint8_t(literalLength));
                    literalLength = 0;
                } 
            }
            output.insert(output.end(), literals.begin(), literals.end());
            output.push_back((matchOffset >> 8) & 0xFF); // High byte of offset
            output.push_back(matchOffset & 0xFF); // Low byte of offset
            matchLength -= min(matchLength,MIN_MATCH_LENGTH);
            if (matchLength >= 15) {
                matchLength -= min(matchLength,uint32_t(15));
                while (matchLength >= 255) {
                    output.push_back(uint8_t(255));
                    matchLength -= 255;
                }
                if (matchLength >= 0) {
                    output.push_back(uint8_t(matchLength));
                    matchLength = 0;
                }
            } 
            pos += match.length; 
            literalStart = pos;
        } else {
            // No good match, move to next position
            pos++;
        }
    }
    // Handle remaining literals at the end
    uint32_t remainingLiterals = inputSize - literalStart;
    if (remainingLiterals > 0) {
        uint32_t literalLength = remainingLiterals;
        uint8_t token = (min(literalLength,uint32_t(15)) << 4) | 0; // No match
        output.push_back(token);
        if(literalLength >= 15) {
            literalLength -= min(literalLength,uint32_t(15));
            while (literalLength >= 255) {
                output.push_back(uint8_t(255));
                literalLength -= 255;
            }
            if (literalLength >= 0) {
                output.push_back(uint8_t(literalLength));
                literalLength = 0;
            } 
        }
        output.insert(output.end(), input + literalStart, input + inputSize);
    }
    return output;
}

vector<uint8_t> lz4Decompress(const uint8_t* input, size_t inputSize) {
    vector<uint8_t> output;
    output.reserve(inputSize);
    size_t pos = 0;
    while (pos < inputSize) {
        uint8_t token = input[pos++];
        uint32_t literalLength = (token >> 4) & 0x0F;
        uint32_t matchLength = token & 0x0F;
        // Handle literals
        if (literalLength == 15) {
            while (input[pos] == 255) {
                literalLength += 255;
                pos++;
            }
            literalLength += input[pos++];
        }
        output.insert(output.end(), input + pos, input + pos + literalLength);
        pos += literalLength;
        if (pos >= inputSize) {
            break;
        }

        uint32_t matchOffset = (input[pos] << 8) | input[pos + 1];
        pos += 2;
        if (matchLength == 15) {
            while (input[pos] == 255) {
                matchLength += 255;
                pos++;
            }
            matchLength += input[pos++];
        }
        matchLength += MIN_MATCH_LENGTH;
        size_t matchStart = output.size() - matchOffset;
        size_t matchEnd = matchStart + matchLength;
        if(matchLength>matchOffset){
            cout<< "Error: Match length exceeds offset!" << endl;
            return output;
        }
        output.insert(output.end(), output.begin() + matchStart, output.begin() + matchEnd);
    }
    
    return output;
}



void copy(const fs::path& source, const fs::path& destination, copy_options options) {
    bool recursive = (options & copy_options::Recursive) != copy_options::None;
    bool overwrite_existing = (options & copy_options::Overwrite_existing) != copy_options::None;
    bool overwrite_inner = (options & copy_options::Overwrite_inner) != copy_options::None;
    bool skip_inner = (options & copy_options::Skip_inner) != copy_options::None;
    if (!fs::exists(source)) {
        cerr << "Source path does not exist: " << source.string() << endl;
        return;
    }
    if(fs::is_directory(source)) {
        if (!fs::exists(destination)) {
            fs::create_directory(destination);
        } 

        if (!recursive && !overwrite_inner && !skip_inner) return;
        //  cout << "Copying file: " << source.string() << " to " << destination.string() << endl;
        for (const auto& entry : fs::directory_iterator(source)) {
            fs::path destPath = destination / entry.path().filename();
            if (entry.is_directory()) {
                copy(entry.path(), destPath, options);
            } else if (entry.is_regular_file()) {
                copy(entry.path(), destPath, overwrite_inner ? copy_options::Overwrite_existing : copy_options::None);
            }
        }
    } else if (fs::is_regular_file(source)) {
        // cout << "Copying file: " << source.string() << " to " << destination.string() << endl;
        if (fs::exists(destination) && !overwrite_existing && !overwrite_inner) {
            // cout << "Copying file: " << source.string() << " to " << destination.string() << endl;
            return;
        } else if (fs::exists(destination) && (overwrite_existing || overwrite_inner)) {
            // cout << "Copying file: " << source.string() << " to " << destination.string() << endl;
            fs::remove(destination);
        }
        fs::copy_file(source, destination);
    }
}