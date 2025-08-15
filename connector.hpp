#ifndef CONNECTOR_HPP
#define CONNECTOR_HPP

#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <map>
#include <fstream>
#include <filesystem>
#include "imgui.h"

#define ICON_FOLDER "📁"
#define ICON_FILE "📄"

namespace fs = std::filesystem;

extern bool error_occured;
extern std::string error_message;

struct File {
    std::string name;
    int icon;
    void* data;
};

struct Directory {
    std::string name;
    int icon;
    bool open = false;
    std::vector<File*> files;
    std::vector<Directory*> subdirectories;
};

class FileExplorer {
    public:
        Directory root_directory;
        fs::path root_path;
        FileExplorer(fs::path path = fs::current_path());
        ~FileExplorer();
        void refresh(fs::path path = fs::current_path(), Directory* dir = nullptr);
        void addFile(const std::string& name, const std::string& content, Directory* dir = nullptr,fs::path path = fs::current_path());
        void addDirectory(const std::string& name, Directory* dir = nullptr, fs::path path = fs::current_path());
        void removeFile(const std::string& name, Directory* dir = nullptr, fs::path path = fs::current_path());
        void removeDirectory(const std::string& name, Directory* dir = nullptr, fs::path path = fs::current_path());
        void renameFile(const std::string& old_name, const std::string& new_name, Directory* dir = nullptr, fs::path path = fs::current_path());
        void renameDirectory(const std::string& old_name, const std::string& new_name, Directory* dir = nullptr, fs::path path = fs::current_path());
        void saveFile(const std::string& name, const std::string& content, Directory* dir = nullptr, fs::path path = fs::current_path());
        std::string loadFile(const std::string& name, Directory* dir = nullptr,fs::path path = fs::current_path());
        void state_change(Directory *dir){
            dir->open = ~dir->open;
        }
        void show();
        
};

class Connector {
    public:
        Connector() = default;
        ~Connector() = default;

        void start();
        void next();
        void dsmantle();
        void Ignore();
        void change_detector();
        void save(std::string file_name);
        void branches();
        void loadfile(const std::string& file_name, Directory* parent = nullptr);
        std::string data;
        std::string data2;
        std::string file_name1;
        std::string file_name2;
        int current_branch;
        int current_version;
        std::vector<std::string> branch_names;
        std::vector<std::string> version_names;
        static int TextEditCallback(ImGuiInputTextCallbackData* data);
        static FileExplorer f;

    private:

        
};





#endif // CONNECTOR_HPP