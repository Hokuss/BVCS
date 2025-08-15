#include "connector.hpp"
#include "utils.hpp"

FileExplorer::FileExplorer(fs::path path) : root_path(path) {
    root_directory.name = path.filename().string();
    root_directory.icon = 0;
    refresh();
}

void FileExplorer::refresh(fs::path path, Directory* dir) {
    if (dir == nullptr) {
        dir = &root_directory;
    } else {
        dir->files.clear();
        dir->subdirectories.clear();
    }

    for (const auto& entry : fs::directory_iterator(path)) {
        if (fs::is_directory(entry)) {
            Directory* subdir = new Directory();
            subdir->name = entry.path().filename().string();
            subdir->icon = 0;
            dir->subdirectories.push_back(subdir);
            refresh(entry.path(), subdir);
        } else if (fs::is_regular_file(entry)) {
            File* file = new File();
            file->name = entry.path().filename().string();
            file->icon = 0;
            dir->files.push_back(file);
        }
    }
}

void FileExplorer::addFile(const std::string& name, const std::string& content, Directory* dir, fs::path path) {
    if (dir == nullptr) {
        dir = &root_directory;
    }
    File* file = new File();
    file->name = name;
    file->data = new std::string(content);
    file->icon = 0;
    std::ofstream ofs(path / name, std::ios::out | std::ios::trunc);
    if (ofs.is_open()) {
        ofs << content;
        ofs.close();
    }
    else {
        error_occured = true;
        error_message = "Failed to create file: " + path.string() + '/' + name;
        return;
    }
    dir->files.push_back(file);
}

void FileExplorer::addDirectory(const std::string& name, Directory* dir, fs::path path) {
    if (dir == nullptr) {
        dir = &root_directory;
    }
    Directory* subdir = new Directory();
    subdir->name = name;
    subdir->icon = 0;
    fs::create_directory(path / name);
    if (!fs::exists(path / name)) {
        error_occured = true;
        error_message = "Failed to create directory: " + path.string() + '/' + name;
        return;
    }
    dir->subdirectories.push_back(subdir);
}

void FileExplorer::removeFile(const std::string& name, Directory* dir, fs::path path) {
    if (dir == nullptr) {
        dir = &root_directory;
    }
    fs::path file_path = path / name;
    if (fs::exists(file_path)) {
        fs::remove(file_path);
    } else {
        error_occured = true;
        error_message = "File not found: " + file_path.string();
        return;
    }
    for (auto it = dir->files.begin(); it != dir->files.end(); ++it) {
        if ((*it)->name == name) {
            delete *it; // Free memory
            dir->files.erase(it);
            return;
        }
    }
    error_occured = true;
    error_message = "File not found in directory: " + file_path.string();
    return;
}

void FileExplorer::removeDirectory(const std::string& name, Directory* dir, fs::path path) {
    if (dir == nullptr) {
        dir = &root_directory;
    }
    fs::path dir_path = path / name;
    if (fs::exists(dir_path)) {
        fs::remove_all(dir_path);
    } else {
        error_occured = true;
        error_message = "Directory not found: " + dir_path.string();
        return;
    }
    for (auto it = dir->subdirectories.begin(); it != dir->subdirectories.end(); ++it) {
        if ((*it)->name == name) {
            delete *it; // Free memory
            dir->subdirectories.erase(it);
            return;
        }
    }
    error_occured = true;
    error_message = "Directory not found in directory: " + dir_path.string();
    return;
}

void FileExplorer::renameFile(const std::string& old_name, const std::string& new_name, Directory* dir, fs::path path) {
    if (dir == nullptr) {
        dir = &root_directory;
    }
    fs::path old_file_path = path / old_name;
    fs::path new_file_path = path / new_name;
    if (fs::exists(old_file_path)) {
        fs::rename(old_file_path, new_file_path);
        for (auto& file : dir->files) {
            if (file->name == old_name) {
                file->name = new_name;
                return;
            }
        }
    } else {
        error_occured = true;
        error_message = "File not found: " + old_file_path.string();
    }
}

void FileExplorer::renameDirectory(const std::string& old_name, const std::string& new_name, Directory* dir, fs::path path) {
    if (dir == nullptr) {
        dir = &root_directory;
    }
    fs::path old_dir_path = path / old_name;
    fs::path new_dir_path = path / new_name;
    if (fs::exists(old_dir_path)) {
        fs::rename(old_dir_path, new_dir_path);
        for (auto& subdir : dir->subdirectories) {
            if (subdir->name == old_name) {
                subdir->name = new_name;
                return;
            }
        }
    } else {
        error_occured = true;
        error_message = "Directory not found: " + old_dir_path.string();
    }
}

void FileExplorer::saveFile(const std::string& name, const std::string& content, Directory* dir, fs::path path) {
    if (dir == nullptr) {
        dir = &root_directory;
    }
    fs::path file_path = path / name;
    std::ofstream ofs(file_path, std::ios::out | std::ios::trunc | std::ios::binary);
    if (ofs.is_open()) {
        ofs.write(content.data(), content.size());
        ofs.close();
    } else {
        error_occured = true;
        error_message = "Failed to save file: " + file_path.string();
        return;
    }
}

std::string FileExplorer::loadFile(const std::string& name, Directory* dir, fs::path path) {
    if (dir == nullptr) {
        dir = &root_directory;
    }
    fs::path file_path = path / name;
    if (!fs::exists(file_path)) {
        error_occured = true;
        error_message = "File not found: " + file_path.string();
        return "";
    }
    std::string data = readTextFile(file_path.string());
    if (data.empty()) {
        error_occured = true;
        error_message = "Failed to read file: " + file_path.string();
        return "";
    }
    return data;
}

void FileExplorer::show() {

    error_message.clear();
    for (const auto& subdir : root_directory.subdirectories) {
        error_message += "Directory: " + subdir->name + "\n";
    }
    // error_message = root_directory.name + " Explorer\n";
    // for (const auto& file : root_directory.files) {
    //     error_message += "File: " + file->name + "\n";
    // }
    

    error_occured = true;
    
}


FileExplorer::~FileExplorer() {
    // Cleanup if necessary
}