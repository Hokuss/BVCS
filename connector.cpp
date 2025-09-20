#include "connector.hpp"
#include "core.hpp"
#include "branch.hpp"
#include "json.hpp"
#include "utils.hpp"

bool error_occured = false;
std::string error_message;

FileExplorer Connector::f;

void Connector::start() {
    begin();
}

void Connector::change_detector(){
    change_upp();
}

void Connector::next(){
    versioning();
}

void Connector::dsmantle() {
    dismantle();
}

// int Connector::TextEditCallback(ImGuiInputTextCallbackData* data) {
//     if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
//         std::string* str = (std::string*)data->UserData;
//         str->resize(data->BufSize);
//         data->Buf = (char*)str->data();
//     }
//     return 0;
// }

// void Connector::save(std::string file_name, fs::path file_path) {
//     if (!data.empty()) {
//         std::ofstream file(file_path / file_name, std::ios::out | std::ios::trunc);
//         if (file.is_open()) {
//             file << data;
//             file.close();
//         } else {
//             std::cerr << "Unable to open file for writing: " << file_name << std::endl;
//         }
//     } else {
//         std::cerr << "No data to save." << std::endl;
//     }
// }

void Connector::branches() {
    branch_names.clear();
    branch_names = all_branches();
    config_parser();
    this->current_version = version;
    this->current_branch = std::find(branch_names.begin(),branch_names.end(), branch_name) - branch_names.begin();
    version_names.clear();
    // version_names.push_back("1");
    // version_names.push_back("2");
    this->version_names = all_versions(branch_names[current_branch]);
}

void Connector::loadfile(const std::string& file_name, Directory* parent) {
    if (parent == nullptr) {
        parent = &f.root_directory;
    }
    // data = f.loadFile(file_name, parent, parent->path);
    file_name1 = file_name;
    file_path1 = parent->path;
}

