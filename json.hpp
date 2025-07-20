#ifndef JSON_HPP
#define JSON_HPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream> 

class directorydata {
public:
    std::string current_directory;
    std::vector<std::string> directories;
    std::vector<std::string> files;

    directorydata(const std::string& dire = "", std::vector<std::string> dirs = {}, std::vector<std::string> file = {})
        : current_directory(dire), directories(dirs), files(file) {}

    void addDirectory(const std::string& dir) {
        directories.push_back(dir);
    }

    void addFile(const std::string& file) {
        files.push_back(file);
    }

    std::vector<std::string> getDirectories() const {
        return directories;
    }

    std::vector<std::string> getFiles() const {
        return files;
    }

    std::string toJson() const;
    void display() const;

private:
    std::string escapeJson(const std::string& str) const;
};

class DirectoryManager {
private:
    std::vector<directorydata> directories; // Stores multiple directorydata objects

public:
    DirectoryManager() = default;

    void adddirectorydata(const directorydata& dirData) {
        directories.push_back(dirData);
    }

    const std::vector<directorydata>& getDirectories() const {
        return directories;
    }

    directorydata* getDirectory(size_t index) {
        if (index < directories.size()) {
            return &directories[index];
        }
        return nullptr;
    }

    directorydata* findDirectory(const std::string& path) {
        for (auto& dir : directories) {
            if (dir.current_directory == path) {
                return &dir;
            }
        }
        return nullptr;
    }

    size_t size() const {
        return directories.size();
    }

    void clear() {
        directories.clear();
    }

    std::string toJson() const;
    bool writeToFile(const std::string& filename) const;
    bool readFromFile(const std::string& filename);
    void displayAll() const;
    bool appendToFile(const std::string& filename, const directorydata& dirData);

private:

    bool parseJson(const std::string& json);
    bool parseDirectoryObject(const std::string& objStr, directorydata& dirData);
    size_t findMatchingBracket(const std::string& str, size_t start);
    size_t findMatchingBrace(const std::string& str, size_t start);
    std::string extractStringValue(const std::string& json, size_t startPos);
    std::vector<std::string> extractArrayValues(const std::string& json, size_t startPos);
    std::string unescapeJson(const std::string& str);
    
};

#endif