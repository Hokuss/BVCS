// #include "json.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

class directorydata {
    public:
        std::string current_directory;
        std::vector<std::string> directories;
        std::vector<std::string> files;

        directorydata (const std::string& dire = "") : current_directory(dire) {}

        void addDirectory(const std::string& dir) {
            directories.push_back(dir);
        }

        void addFile(const std::string& file) {
            files.push_back(file);
        }

        std::string toJson() const {
            std::ostringstream json;
            json << "{\n";
            json << "  \"current_directory\": \"" << escapeJson(current_directory) << "\",\n";
        
            json << "  \"directories\": [\n";
            for (size_t i = 0; i < directories.size(); ++i) {
                json << "    \"" << escapeJson(directories[i]) << "\"";
                if (i < directories.size() - 1) json << ",";
                json << "\n";
            }
            json << "  ],\n";
        
            json << "  \"files\": [\n";
            for (size_t i = 0; i < files.size(); ++i) {
                json << "    \"" << escapeJson(files[i]) << "\"";
                if (i < files.size() - 1) json << ",";
                json << "\n";
            }
            json << "  ]\n";
            json << "}";
        
            return json.str();
        }

        void display() const {
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

    private:

        std::string escapeJson(const std::string& str) const {
            std::string escaped;
            for (char c : str) {
                switch (c) {
                    case '"': escaped += "\\\""; break;
                    case '\\': escaped += "\\\\"; break;
                    case '\b': escaped += "\\b"; break;
                    case '\f': escaped += "\\f"; break;
                    case '\n': escaped += "\\n"; break;
                    case '\r': escaped += "\\r"; break;
                    case '\t': escaped += "\\t"; break;
                    default: escaped += c; break;
                }
            }
            return escaped;
        }
};

class DirectoryManager {
    private:
        std::vector<directorydata> directories;

    public:
        // Add a directory entry
        void adddirectorydata(const directorydata& dirData) {
            directories.push_back(dirData);
        }

        // Get all directory entries
        const std::vector<directorydata>& getDirectories() const {
            return directories;
        }

        // Get directory by index
        directorydata* getDirectory(size_t index) {
            if (index < directories.size()) {
                return &directories[index];
            }
            return nullptr;
        }

        // Find directory by path
        directorydata* findDirectory(const std::string& path) {
            for (auto& dir : directories) {
                if (dir.current_directory == path) {
                    return &dir;
                }
            }
            return nullptr;
        }

        // Get number of directories
        size_t size() const {
            return directories.size();
        }

        // Clear all directories
        void clear() {
            directories.clear();
        }

        // Convert all directories to JSON string
        std::string toJson() const {
            std::ostringstream json;
            json << "{\n";
            json << "  \"directories\": [\n";
            
            for (size_t i = 0; i < directories.size(); ++i) {
                json << directories[i].toJson();
                if (i < directories.size() - 1) json << ",";
                json << "\n";
            }
            
            json << "  ]\n";
            json << "}";
            
            return json.str();
        }

        // Write to JSON file
        bool writeToFile(const std::string& filename) const {
            std::ofstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
                return false;
            }
            
            file << toJson();
            file.close();
            return true;
        }

        // Read from JSON file
        bool readFromFile(const std::string& filename) {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open file for reading: " << filename << std::endl;
                return false;
            }
            
            std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
            file.close();
            
            return parseJson(content);
        }

        // Display all directories
        void displayAll() const {
            std::cout << "Total directories: " << directories.size() << std::endl;
            for (size_t i = 0; i < directories.size(); ++i) {
                std::cout << "\n--- Directory " << (i + 1) << " ---" << std::endl;
                directories[i].display();
            }
        }

        // Append a single directory to existing file
        bool appendToFile(const std::string& filename, const directorydata& dirData) {
            // Read existing data
            DirectoryManager temp;
            if (temp.readFromFile(filename)) {
                // Add existing directories to current manager
                for (const auto& dir : temp.getDirectories()) {
                    directories.push_back(dir);
                }
            }
            
            // Add new directory
            directories.push_back(dirData);
            
            // Write all data back
            return writeToFile(filename);
        }

    private:
        // Simple JSON parser for our specific structure
        bool parseJson(const std::string& json) {
            try {
                directories.clear();
                
                // Find the directories array
                size_t arrayStart = json.find("\"directories\":");
                if (arrayStart == std::string::npos) return false;
                
                arrayStart = json.find("[", arrayStart);
                if (arrayStart == std::string::npos) return false;
                
                size_t arrayEnd = findMatchingBracket(json, arrayStart);
                if (arrayEnd == std::string::npos) return false;
                
                // Parse each directory object
                size_t pos = arrayStart + 1;
                while (pos < arrayEnd) {
                    size_t objStart = json.find("{", pos);
                    if (objStart == std::string::npos || objStart >= arrayEnd) break;
                    
                    size_t objEnd = findMatchingBrace(json, objStart);
                    if (objEnd == std::string::npos || objEnd > arrayEnd) break;
                    
                    directorydata dirData;
                    if (parseDirectoryObject(json.substr(objStart, objEnd - objStart + 1), dirData)) {
                        directories.push_back(dirData);
                    }
                    
                    pos = objEnd + 1;
                }
                
                return true;
            } catch (const std::exception& e) {
                std::cerr << "Error parsing JSON: " << e.what() << std::endl;
                return false;
            }
        }

        // Parse a single directory object
        bool parseDirectoryObject(const std::string& objStr, directorydata& dirData) {
            // Find current_directory
            size_t pos = objStr.find("\"current_directory\":");
            if (pos != std::string::npos) {
                dirData.current_directory = extractStringValue(objStr, pos);
            }
            
            // Find directories array
            pos = objStr.find("\"directories\":");
            if (pos != std::string::npos) {
                dirData.directories = extractArrayValues(objStr, pos);
            }
            
            // Find files array
            pos = objStr.find("\"files\":");
            if (pos != std::string::npos) {
                dirData.files = extractArrayValues(objStr, pos);
            }
            
            return true;
        }

        // Find matching bracket for arrays
        size_t findMatchingBracket(const std::string& str, size_t start) {
            int count = 1;
            for (size_t i = start + 1; i < str.length() && count > 0; ++i) {
                if (str[i] == '[') count++;
                else if (str[i] == ']') count--;
                if (count == 0) return i;
            }
            return std::string::npos;
        }

        // Find matching brace for objects
        size_t findMatchingBrace(const std::string& str, size_t start) {
            int count = 1;
            bool inString = false;
            bool escaped = false;
            
            for (size_t i = start + 1; i < str.length() && count > 0; ++i) {
                if (escaped) {
                    escaped = false;
                    continue;
                }
                
                if (str[i] == '\\') {
                    escaped = true;
                    continue;
                }
                
                if (str[i] == '"') {
                    inString = !inString;
                    continue;
                }
                
                if (!inString) {
                    if (str[i] == '{') count++;
                    else if (str[i] == '}') count--;
                    if (count == 0) return i;
                }
            }
            return std::string::npos;
        }

        // Extract string value from JSON
        std::string extractStringValue(const std::string& json, size_t startPos) {
            size_t valueStart = json.find(":", startPos);
            if (valueStart == std::string::npos) return "";
            
            valueStart+=3; // Skip opening quote
            size_t valueEnd = json.find("\"", valueStart);
            if (valueEnd == std::string::npos) return "";
            std::cout << "Extracting value from JSON: " << json.substr(startPos+1) << std::endl;
            std::cout<< "Value start: " << valueStart << ", Value end: " << valueEnd << std::endl;
            std::cout << "Extracted value: " << json.substr(valueStart, valueEnd - valueStart) << std::endl;
            return unescapeJson(json.substr(valueStart, valueEnd - valueStart));
        }

        // Extract array values from JSON
        std::vector<std::string> extractArrayValues(const std::string& json, size_t startPos) {
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
                
                pos = quoteEnd + 1;
            }
            
            return values;
        }

        // Unescape JSON string
        std::string unescapeJson(const std::string& str) {
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
                        default: unescaped += str[i]; break;
                    }
                } else {
                    unescaped += str[i];
                }
            }
            return unescaped;
        }
};


int main(){
    DirectoryManager manager;

    manager.readFromFile("directories.json");
    // manager.displayAll();

    directorydata* dir1;
    dir1 = manager.findDirectory("/home/user/downloads");
    if (dir1 == nullptr) {
        std::cout << "Directory not found!" << std::endl;
        return 1;
    }
    for(auto& dir : dir1->directories){
        std::cout << "Directory: " << dir << std::endl;
    }
    for(auto& file : dir1->files){
        std::cout << "File: " << file << std::endl;
    }

    return 0;
}
