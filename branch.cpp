#include "branch.hpp"

int32_t version = -1;
string branch = "main";

void config_parser(string config_file = ".bvcs/cnfg.txt") {
    ifstream inputfile(config_file, ios::binary);
    if (!inputFile.is_open()) {
        cerr << "Fatal Error! Configuration Missing. If this is the first time the project is using the BVCS. Please use 'Start' command." << endl;
        return; // Return an error code
    }
    inputfile.seekg(18);
    inputfile >> version;
    inputFile.seekg(35);
    getline(inputFile, branch);
    if (branch.empty()) {
        cerr << "Fatal Error! Branch name missing in configuration." << endl;
        return; // Return an error code
    }
    inputFile.close();
}

void config_writer(const string& branch_name, int version) {
    ofstream outputfile(".bvcs/cnfg.txt", ios::trunc);
    if (!outputfile) {
        cerr << "Error opening file for writing!" << endl;
        return; // Return an error code
    }
    outputfile << "Current Version: " << version << endl;
    outputfile << "Current Branch: " << branch_name << endl;
    outputfile.close();
}

long long counter(const fs::path& dir_path) {
    long long count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
        if (fs::is_regular_file(entry.path())) {
            count++;
        }
    }
    return count-2; // Subtracting 2 for dir.json and dir_struct
}

void new_branch(const string& branch_name) {
    fs::path version_history = fs::current_path() / ".bvcs" / "version_history" / branch_name;
    if (fs::exists(version_history)) {
        cerr << "Error: Branch '" << branch_name << "' already exists." << endl;
        return; // Return an error code
    }
    fs::path first_commit = fs::current_path() / ".bvcs" / "version_history" / branch_name / "0";
    fs::create_directories(version_history);
    fs::create_directory(first_commit);
    ofstream inputFile(first_commit / "bk_ptr.json", ios::binary);
    if (!inputFile.is_open()) {
        cerr << "Error: Unable to create backup pointer file for new branch." << endl;
        return; // Return an error code
    }
    inputFile << branch << "\n" << version;
    inputFile.close();
    branch = branch_name; // Update the current branch
    version = 1; // Reset version for the new branch
    config_writer(branch_name, 1);
    cout << "New branch '" << branch_name << "' created successfully." << endl;
}

void recursive_copy(string branch_name, int version, long long remain) {
    if (remain <= 0) {
        return; // No files to process
    }
    fs::path version_history = fs::current_path() / ".bvcs" / "version_history" / branch_name;
    fs::path target_path = version_history / to_string(version);
    if(version == 0 && branch_name == "main") {
        fs::path current_commit = fs::current_path() / ".bvcs" / "current_commit";
        fs::copy (fs::current_path() / ".bvcs" / "version_history" / branch_name / to_string(version), current_commit, fs::copy_options::recursive | fs::copy_options::skip_existing);
        return; // Base case for recursion
    }
    else if (version == 0) {
        ifsteam inputFile(target_path / "bk_ptr.json", ios::binary);
        if (!inputFile.is_open()) {
            cerr << "Error: Unable to open backup pointer file for version 0." << endl;
            return; // Return an error code
        }
        string prev_branch;
        getline(inputFile, prev_branch);
        int prev_version;
        inputFile >> prev_version;
        inputFile.close();
        recursive_copy(prev_branch, prev_version, remain);
        return; 
    }
    fs::path current_commit = fs::current_path() / ".bvcs" / "current_commit";
    for(const auto& entry : fs::directory_iterator(target_path)) {
        string file_name = entry.path().filename().string();
        fs::path dest_path = current_commit / file_name;
        if (fs::exists(dest_path)) {
            continue; // Skip copying if the file already exists
        }
        fs::copy(entry.path(), dest_path);
        remain--;
        if (remain <= 0) {
            break; // Stop copying if we have reached the limit
        }
    }
    if(remain > 0){
        recursive_copy(branch_name, version - 1, remain);
    }
}

void cc_builder(int version, string branch_name = "main") {
    fs::path current_commit = fs::current_path() / ".bvcs" / "current_commit";
    fs::path version_history = fs::current_path() / ".bvcs" / "version_history" / branch_name;
    fs::remove_all(current_commit);
    fs::create_directory(current_commit);
    if(!fs::exists(version_history)) {
        cerr << "Error: Version history for branch '" << branch_name << "' does not exist." << endl;
        return; // Return an error code
    }
    fs::copy(version_history / to_string(version), current_commit, fs::copy_options::recursive);
    fs::path dir_json_path = current_commit / "dir.json";
    long long total_files = fs::file_size(dir_json_path)/130;
    long long i = counter(current_commit) - total_files;
    if(i==0){
        return; // No files to process
    }
    int j = version-1;
    recursive_copy(branch_name, j, i);
    return;
}

void versioning(){
    config_parser();
    if (version < 0) {
        cerr << "Fatal Error! Version number is not set." << endl;
        return; // Return an error code
    }
    fs::path current_commit = fs::current_path() / ".bvcs" / "current_commit";
    fs::path staging_area = fs::current_path() / ".bvcs" / "staging_area";
    fs::path version_history = fs::current_path() / ".bvcs" / "version_history" / branch;
    if (!fs::exists(version_history)) {
        fs::create_directories(version_history);
    }

    fs::path version_path = version_history / to_string(++version);
    fs::copy(staging_area, version_path, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    config_writer(branch, version);
    
    if (!fs::exists(current_commit)) {
        cerr << "Fatal Error! Current commit directory does not exist." << endl;
        return; // Return an error code
    }
    
    if (!fs::exists(staging_area)) {
        cerr << "Fatal Error! Staging area directory does not exist." << endl;
        return; // Return an error code
    }
    cc_builder(version-1, branch);
}

void merger(const string& branch_name) {
    fs::path current_commit = fs::current_path() / ".bvcs" / "current_commit";
    fs::path version_history = fs::current_path() / ".bvcs" / "version_history" / branch_name;
    if (!fs::exists(version_history)) {
        cerr << "Error: Version history for branch '" << branch_name << "' does not exist." << endl;
        return; // Return an error code
    }
    fs::copy(version_history, current_commit, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    cout << "Merged branch '" << branch_name << "' into current commit successfully." << endl;
}

int main(){
    config_parser();
    versioning();
    cout << "Versioning completed successfully!" << endl;
    return 0;
}