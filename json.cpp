#include "json.hpp"
// #include <iostream>
// #include <fstream>
// #include <string>
// #include <vector>
// #include <sstream>
// #include <algorithm>

std::string directorydata::escapeJson(const std::string& str) const {
    std::string escaped;
    for (char c : str) {
        switch (c) {
            case '"':  escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break; // Backspace
            case '\f': escaped += "\\f"; break; // Form feed
            case '\n': escaped += "\\n"; break; // Newline
            case '\r': escaped += "\\r"; break; // Carriage return
            case '\t': escaped += "\\t"; break; // Tab
            default:   escaped += c; break;
        }
    }
    return escaped;
}

std::string directorydata::toJson() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"current_directory\": \"" << escapeJson(current_directory) << "\",\n";

    json << "\"directory\": [\n";
    for (size_t i = 0; i < directories.size(); ++i) {
        json << "    \"" << escapeJson(directories[i]) << "\"";
        if (i < directories.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ],\n"; // Comma after directories array

    json << "  \"files\": [\n";
    for (size_t i = 0; i < files.size(); ++i) {
        json << "    \"" << escapeJson(files[i]) << "\"";
        if (i < files.size() - 1) json << ",";
        json << "\n";
    }
    json << "  ]\n"; // No comma after files array
    json << "}";

    return json.str();
}

// Definition of display()
void directorydata::display() const {
    std::cout << "Current Directory: " << current_directory << std::endl;
    std::cout << "Directories: " << std::endl;
    for (const auto& dir : directories) {
        std::cout << "  - " << dir << std::endl;
    }
    std::cout << "Files: " << std::endl;
    for (const auto& file : files) {
        std::cout << "  - " << file << std::endl;
    }
}

std::string DirectoryManager::toJson() const {
    std::ostringstream json;
    json << "{\n";
    json << "\"directories\": [\n";

    for (size_t i = 0; i < directories.size(); ++i) {
        // Call toJson() on each directorydata object
        // Indent the output from directorydata::toJson for better readability
        std::string dirJson = directories[i].toJson();
        // Add indentation to each line of dirJson, then append
        std::istringstream iss(dirJson);
        std::string line;
        while (std::getline(iss, line)) {
            json << "    " << line << "\n"; // Add 4 spaces of indentation
        }

        if (i < directories.size() - 1) {
            json << ",\n"; // Add comma between directory objects
        }
    }

    json << "  ]\n"; // End of directories array
    json << "}";

    return json.str();
}

// Definition of writeToFile()
bool DirectoryManager::writeToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
        return false;
    }

    file << toJson(); // Write the JSON representation of all directories
    file.close();
    return true;
}

// Definition of readFromFile()
bool DirectoryManager::readFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file for reading: " << filename << std::endl;
        return false;
    }

    // Read entire file content into a string
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    return parseJson(content); // Parse the JSON content
}

// Definition of displayAll()
void DirectoryManager::displayAll() const {
    std::cout << "Total directories managed: " << directories.size() << std::endl;
    for (size_t i = 0; i < directories.size(); ++i) {
        std::cout << "\n--- Directory Entry " << (i + 1) << " ---" << std::endl;
        directories[i].display(); // Call display() on each directorydata object
    }
}

bool DirectoryManager::appendToFile(const std::string& filename, const directorydata& dirData) {
    DirectoryManager temp_manager;
    if (temp_manager.readFromFile(filename)) {
        this->directories.clear(); 
        for (const auto& dir : temp_manager.getDirectories()) {
            this->directories.push_back(dir);
        }
    } else {
        this->directories.clear();
    }

    this->directories.push_back(dirData);
    return writeToFile(filename);
}

bool DirectoryManager::parseJson(const std::string& json) {
    try {
        directories.clear(); // Clear existing data before parsing new

        // std::cout << "Parsing JSON: " << json << std::endl;

        size_t arrayStartPos = json.find("\"directories\":");
        if (arrayStartPos == std::string::npos) {
            std::cerr << "Error: 'directories' array not found in JSON." << std::endl;
            return false;
        }

        arrayStartPos = json.find("[", arrayStartPos);
        if (arrayStartPos == std::string::npos) {
            std::cerr << "Error: Opening bracket for 'directories' array not found." << std::endl;
            return false;
        }

        size_t arrayEndPos = findMatchingBracket(json, arrayStartPos);
        if (arrayEndPos == std::string::npos) {
            std::cerr << "Error: Matching closing bracket for 'directories' array not found." << std::endl;
            return false;
        }

        // Extract content of the directories array
        std::string arrayContent = json.substr(arrayStartPos + 1, arrayEndPos - arrayStartPos - 1);

        size_t pos = 0;
        while (pos < arrayContent.length()) {

            size_t objStart = arrayContent.find("{", pos);
            if (objStart == std::string::npos) break;

            size_t objEnd = findMatchingBrace(arrayContent, objStart);
            if (objEnd == std::string::npos) {
                std::cerr << "Error: Matching closing brace for object not found." << std::endl;
                return false;
            }

            std::string objStr = arrayContent.substr(objStart, objEnd - objStart + 1);

            directorydata dirData;
            if (parseDirectoryObject(objStr, dirData)) {
                directories.push_back(dirData);
            } else {
                std::cerr << "Warning: Failed to parse a directory object." << std::endl;
                // Continue to try parsing next objects, or return false if strict
            }

            pos = objEnd + 1; // Move past the current object
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return false;
    }
}

// Definition of parseDirectoryObject()
bool DirectoryManager::parseDirectoryObject(const std::string& objStr, directorydata& dirData) {
    // Current Directory
    size_t pos = objStr.find("\"current_directory\":");
    if (pos != std::string::npos) {
        dirData.current_directory = extractStringValue(objStr, pos);
    } else {
        std::cerr << "Warning: 'current_directory' not found in object." << std::endl;
    }

    // Directories array
    pos = objStr.find("\"directory\":");
    if (pos != std::string::npos) {
        dirData.directories = extractArrayValues(objStr, pos);
    } else {
        std::cerr << "Warning: 'directory' array not found in object." << std::endl;
    }

    // Files array
    pos = objStr.find("\"files\":");
    if (pos != std::string::npos) {
        dirData.files = extractArrayValues(objStr, pos);
    } else {
        std::cerr << "Warning: 'files' array not found in object." << std::endl;
    }

    return true;
}

// Definition of findMatchingBracket()
size_t DirectoryManager::findMatchingBracket(const std::string& str, size_t start) {
    int count = 1; // Count for '['
    for (size_t i = start + 1; i < str.length(); ++i) {
        if (str[i] == '[') {
            count++;
        } else if (str[i] == ']') {
            count--;
        }
        if (count == 0) return i;
    }
    return std::string::npos; // Not found
}

// Definition of findMatchingBrace()
size_t DirectoryManager::findMatchingBrace(const std::string& str, size_t start) {
    int count = 1; // Count for '{'
    bool inString = false;
    bool escaped = false;

    for (size_t i = start + 1; i < str.length(); ++i) {
        if (escaped) { // If previous char was '\', ignore current char's special meaning
            escaped = false;
            continue;
        }

        if (str[i] == '\\') { // Found an escape character
            escaped = true;
            continue;
        }

        if (str[i] == '"') { // Toggle inString state
            inString = !inString;
            continue;
        }

        if (!inString) { // Only count braces if not inside a string
            if (str[i] == '{') {
                count++;
            } else if (str[i] == '}') {
                count--;
            }
            if (count == 0) return i;
        }
    }
    return std::string::npos; // Not found
}

// Definition of extractStringValue()
std::string DirectoryManager::extractStringValue(const std::string& json, size_t startPos) {
    size_t colonPos = json.find(":", startPos);
    if (colonPos == std::string::npos) return "";

    size_t quoteStart = json.find("\"", colonPos + 1); // Find first quote after colon
    if (quoteStart == std::string::npos) return "";

    size_t quoteEnd = json.find("\"", quoteStart + 1); // Find closing quote
    if (quoteEnd == std::string::npos) return "";

    std::string value = json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
    return unescapeJson(value);
}

// Definition of extractArrayValues()
std::vector<std::string> DirectoryManager::extractArrayValues(const std::string& json, size_t startPos) {
    std::vector<std::string> values;

    size_t arrayStart = json.find("[", startPos);
    if (arrayStart == std::string::npos) return values;

    size_t arrayEnd = findMatchingBracket(json, arrayStart);
    if (arrayEnd == std::string::npos) return values;

    std::string arrayContent = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);

    size_t pos = 0;
    while (pos < arrayContent.length()) {
        size_t quoteStart = arrayContent.find("\"", pos);
        if (quoteStart == std::string::npos) break;

        size_t quoteEnd = arrayContent.find("\"", quoteStart + 1);
        if (quoteEnd == std::string::npos) break;

        std::string value = arrayContent.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        values.push_back(unescapeJson(value));

        pos = quoteEnd + 1; // Move past the current string value
    }

    return values;
}

// Definition of unescapeJson()
std::string DirectoryManager::unescapeJson(const std::string& str) {
    std::string unescaped;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '"': unescaped += '"'; i++; break;
                case '\\': unescaped += '\\'; i++; break;
                case 'b': unescaped += '\b'; i++; break;
                case 'f': unescaped += '\f'; i++; break;
                case 'n': unescaped += '\n'; i++; break;
                case 'r': unescaped += '\r'; i++; break;
                case 't': unescaped += '\t'; i++; break;
                // Add other escape sequences if needed (e.g., /uXXXX for unicode)
                default: unescaped += str[i]; break; // Keep the backslash if not a known escape
            }
        } else {
            unescaped += str[i];
        }
    }
    return unescaped;
}


// int main(){
//     DirectoryManager manager;

//     manager.readFromFile("directories.json");
//     // manager.displayAll();

//     directorydata* dir1;
//     dir1 = manager.findDirectory("/home/user/downloads");
//     if (dir1 == nullptr) {
//         std::cout << "Directory not found!" << std::endl;
//         return 1;
//     }
//     for(auto& dir : dir1->directories){
//         std::cout << "Directory: " << dir << std::endl;
//     }
//     for(auto& file : dir1->files){
//         std::cout << "File: " << file << std::endl;
//     }

//     return 0;
// }
