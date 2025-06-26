#include "branch.hpp"

int version;
string branch;

void config_parser() {
    ifstream inputfile(".bvcs/cnfg.bin", ios::binary);
    if (!inputfile.is_open()) {
        cerr << "Fatal Error! Configuration Missing. If this is the first time the project is using the BVCS. Please use 'Start' command." << endl;
        return; // Return an error code
    }
    
    std::string line;
    
    // --- Parse Version ---
    if (std::getline(inputfile, line)) {
        // Find the colon, then extract the number after it.
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            try {
                // Substring from the character after the colon and space
                version = std::stoi(line.substr(colon_pos + 2));
            } catch (const std::invalid_argument& ia) {
                std::cerr << "Fatal Error! Invalid version number in configuration." << std::endl;
                inputfile.close();
                return;
            }
        } else {
            std::cerr << "Fatal Error! Version line malformed in configuration." << std::endl;
            inputfile.close();
            return;
        }
    } else {
        std::cerr << "Fatal Error! Configuration file is empty or corrupted." << std::endl;
        inputfile.close();
        return;
    }

    // --- Parse Branch ---
    if (std::getline(inputfile, line)) {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            // Substring from the character after the colon and space
            branch = line.substr(colon_pos + 2);
        } else {
             std::cerr << "Fatal Error! Branch line malformed in configuration." << std::endl;
             inputfile.close();
             return;
        }
    } else {
        std::cerr << "Fatal Error! Branch name missing in configuration." << std::endl;
        inputfile.close();
        return;
    }

    if (branch.empty()) {
        std::cerr << "Fatal Error! Branch name missing in configuration." << std::endl;
        // The return is handled by the check above, but this is a good safeguard.
    }

    inputfile.close();
}

void config_writer(const string& branch_name, int version) {
    ofstream outputfile(".bvcs/cnfg.bin", ios::trunc | ios::binary);
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
    fs::create_directory(version_history);
    fs::create_directory(first_commit);
    ofstream inputFile(first_commit / "bk_ptr.json", ios::binary);
    if (!inputFile.is_open()) {
        cerr << "Error: Unable to create backup pointer file for new branch." << endl;
        return; // Return an error code
    }
    inputFile << branch << "\n" << version;
    inputFile.close();
    branch = branch_name; // Update the current branch
    version = 0; // Reset version for the new branch
    config_writer(branch_name, version);
    cout << "New branch '" << branch_name << "' created successfully." << endl;
}

void recursive_copy(const string& branch_name, int version, long long remain) {
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
        ifstream inputFile(target_path / "bk_ptr.json", ios::binary);
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

void cc_builder(int version,const string& branch_name) {
    fs::path current_commit = fs::current_path() / ".bvcs" / "current_commit";
    fs::path version_history = fs::current_path() / ".bvcs" / "version_history" / branch_name;
    fs::remove_all(current_commit);
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
    fs::path current_commit = fs::current_path() / ".bvcs" / "current_commit";
    fs::path staging_area = fs::current_path() / ".bvcs" / "staging_area";
    fs::path version_history = fs::current_path() / ".bvcs" / "version_history" / branch;
    if (!fs::exists(version_history)) {
        cerr<< "Branch not found! Error x1" <<endl;
        cout << branch << endl;
        return;
    }
    fs::directory_iterator dir_iter(staging_area);
    if (dir_iter == fs::end(dir_iter)) {
        cerr << "Staging area is empty.! Error x101" << endl;
        return; // Return an error code
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
    cc_builder(version, branch);

    fs::remove_all(staging_area);
    fs::create_directory(staging_area);
}

// void merger(const string& branch_name) {
//     fs::path current_commit = fs::current_path() / ".bvcs" / "current_commit";
//     fs::path version_history = fs::current_path() / ".bvcs" / "version_history" / branch_name;
//     if (!fs::exists(version_history)) {
//         cerr << "Error: Version history for branch '" << branch_name << "' does not exist." << endl;
//         return; // Return an error code
//     }
//     fs::copy(version_history, current_commit, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
//     cout << "Merged branch '" << branch_name << "' into current commit successfully." << endl;
// }

// int main(){
//     config_parser();
//     new_branch("test_branch");
//     return 0;
// }