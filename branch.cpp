#include "branch.hpp"
#include "utils.hpp"

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
    config_parser();
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
    // cout<<branch<<" "<<version;
    inputFile << branch << "\n" << version;
    inputFile.close();
    branch = branch_name; // Update the current branch
    version = 0; // Reset version for the new branch
    config_writer(branch_name, version);
    // cout << "New branch '" << branch_name << "' created successfully." << endl;
}

void recursive_copy(const string& branch_name, int version, long long remain) {
    if (remain <= 0) {
        return; // No files to process
    }
    fs::path version_history = fs::current_path() / ".bvcs" / "version_history" / branch_name;
    if (!fs::exists(version_history)) {
        cerr << "Error: Version history for branch '" << branch_name << "' does not exist." << endl;
        return; // Return an error code
    }
    fs::path target_path = version_history / to_string(version);
    cout<< "Target path: " << target_path <<" "<<remain<< endl;
    if(version == 0 && branch_name == "Main") {
        fs::path current_commit = fs::current_path() / ".bvcs" / "current_commit";

        if (!fs::exists(current_commit)) {
            cerr << "Fatal Error! Current commit directory does not exist." << endl;
            return; // Return an error code
        }
        if (!fs::exists(target_path)) {
            cerr << "Error: Version 0 does not exist for branch '" << branch_name << "'." << endl;
            return; // Return an error code
        }

        // cout<< "Copying version 0 of branch '" << branch_name << "' to current commit." << endl;
        copy (target_path, current_commit, copy_options::Recursive | copy_options::Skip_inner);
        cout << "Version 0 copied successfully." << endl;
        return; // Base case for recursion
    }
    else if (version == 0) {
        ifstream inputFile(target_path / "bk_ptr.json", ios::binary);
        cout << target_path / "bk_ptr.json" << endl;
        if (!fs::exists(target_path / "bk_ptr.json")) {
            cerr << "Error: Backup pointer file for version 0 does not exist." << endl;
            return; // Return an error code
        }
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
        copy(entry.path(), dest_path, copy_options::None);
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
    // cout<< "Building current commit for branch '" << branch_name << "' at version " << version << "." << endl;
    if(!fs::exists(version_history)) {
        cerr << "Error: Version history for branch '" << branch_name << "' does not exist." << endl;
        return; // Return an error code
    }
    copy(version_history / to_string(version), current_commit, copy_options::Recursive);
    fs::path dir_json_path = current_commit / "dir.json";
    long long total_files = fs::file_size(dir_json_path)/130;
    long long i = total_files - counter(current_commit);
    // cout << "Total files to process: " << i << endl;
    // cout<< counter(current_commit) << " files already present in current commit." << endl;
    // cout << total_files << " files in total." << endl;
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
        cout << version_history << endl;
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

void limited_recursive_copy(fs::path target, fs::path source, int current_version){
    if (!fs::exists(source)) {
        cerr << "Source path does not exist: " << source << endl;
        return; // Return an error code
    }
    if (!fs::exists(target)) {
        fs::create_directories(target);
    }
    if(current_version == 0) {
        return; // Base case for recursion
    }
    copy_options options = copy_options::Skip_inner;
    copy(source / to_string(current_version), target, options);
    limited_recursive_copy(target, source, current_version - 1);
}


void branch_merge(){
    config_parser();
    fs::path version_history = fs::current_path() / ".bvcs" / "version_history";
    if (!fs::exists(version_history)) {
        cerr << "Branch not found! Error x1" << endl;
        return; // Return an error code
    }
    if (version == 0 || branch == "Main") {
        cerr << "Cannot merge from version 0 or Main." << endl;
        return; // Return an error code
    }
    fs::path bk_ptr_path = version_history / branch / "0" / "bk_ptr.json";
    if (!fs::exists(bk_ptr_path)) {
        cerr << "Backup pointer file does not exist for version 0." << endl;
        return; // Return an error code
    }
    ifstream inputFile(bk_ptr_path, ios::binary);
    if (!inputFile.is_open()) {
        cerr << "Error: Unable to open backup pointer file for version 0." << endl;
        return; // Return an error code
    }
    string prev_branch;
    int prev_version;
    getline(inputFile, prev_branch);
    inputFile >> prev_version;
    inputFile.close();
    cout << "Current Branch: " << branch << ", Version: " << version << endl;
    cout << "Merging to Branch: " << prev_branch << ", Version: " << prev_version + 1 << endl;
    cout<< "Press \'Y\' to continue merging as next version or any other key to cancel." << endl;
    if(cin.get() == 'Y'){
        cout << "Merging..." << endl;
        fs::path target_path = version_history / prev_branch / to_string(prev_version + 1);
        limited_recursive_copy(target_path, version_history / branch, version);
        config_writer(prev_branch, prev_version + 1);
        cc_builder(prev_version + 1, prev_branch);
        // fs::remove_all(version_history / branch / to_string(version));
        cout << "Merge completed successfully." << endl;

    } else {
        cout << "Merge cancelled." << endl;
        return; // Return an error code
    }
}

void delete_branch(const string& branch_name) {
    config_parser();
    if (branch_name == "Main" || branch_name == branch) {
        cerr << "Error: Cannot delete the "<< branch_name <<" branch." << endl;
        return; // Return an error code
    }
    fs::path version_history = fs::current_path() / ".bvcs" / "version_history" / branch_name;
    if (!fs::exists(version_history)) {
        cerr << "Error: Branch '" << branch_name << "' does not exist." << endl;
        return; // Return an error code
    }
    cout << "Are you sure you want to delete the branch '" << branch_name << "'? This action cannot be undone. (Y/N): ";
    char confirmation;
    std::cin >> confirmation;
    if (confirmation != 'Y' && confirmation != 'y') {
        cout << "Branch deletion cancelled." << endl;
        return; // Return an error code
    }
    fs::remove_all(version_history);
    cout << "Branch '" << branch_name << "' deleted successfully." << endl;
}