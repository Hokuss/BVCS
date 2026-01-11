#ifndef CONNECTOR_HPP
#define CONNECTOR_HPP

#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <map>
#include <fstream>
#include <unordered_set>
#include <filesystem>
#include <mutex>
#include <chrono>
#include "imgui.h"

namespace fs = std::filesystem;

extern bool error_occured;
extern std::string error_message;

struct item {
    virtual ~item() = default;
    std::string name;
    ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    fs::path path;
};

struct File : public item {
    void *data = nullptr;
};

struct Directory : public item {
    std::vector<File*> files;
    std::vector<Directory*> subdirectories;
    Directory* parent = nullptr;
    bool open = false; // To track if the directory is open in the UI
};

class FileExplorer {
    public:
        std::chrono::time_point<std::chrono::steady_clock> last = std::chrono::steady_clock::now();

        bool clipboard = false;
        bool clipboard_active = false;
        std::vector<File*> clipboard_files;
        std::vector<Directory*> clipboard_directories;
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
        // void saveFile(const std::string& name, const std::string& content, Directory* dir = nullptr, fs::path path = fs::current_path());
        // std::string loadFile(const std::string& name, Directory* dir = nullptr,fs::path path = fs::current_path());
        void state_change(Directory *dir){
            dir->open = ~dir->open;
        }
        void show();
        void cutSelected(std::unordered_set<void*>& selected_items);
        void copySelected(std::unordered_set<void*>& selected_items);
        void paste(void* paste_dir = nullptr);
        void removeSelected(std::unordered_set<void*>& selected_items);
};

class Connector {
    public:
        Connector() = default;
        ~Connector() = default;

        std::vector<std::string> result_lines;
        std::mutex mutex_;

        void start();
        void next();
        void dsmantle();
        void change_detector();
        void run_commands(const std::vector<std::string>& commands);
        void run_in_terminal(const std::vector<std::string>& commands);
        // void save(std::string file_name, fs::path file_path = fs::current_path());
        void branches();
        void loadfile(const std::string& file_name, Directory* parent = nullptr);
        std::string data;
        std::string data2;
        std::string file_name1;
        std::string file_name2;
        fs::path file_path1;
        fs::path file_path2;
        int current_branch;
        int current_version;
        std::vector<std::string> branch_names;
        std::vector<std::string> version_names;
        // static int TextEditCallback(ImGuiInputTextCallbackData* data);
        static FileExplorer f;

    private:

        
};





#endif // CONNECTOR_HPP