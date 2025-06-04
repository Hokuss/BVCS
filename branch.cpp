#include "branch.hpp"

uint32_t version = 0;
string branch_name = "main";

void config_parser(){
    fs::path config_path = fs::current_path() / ".bvcs" / "cnfg.txt";
    ifstream inputFile(config_path, ios::binary);
    if (!inputFile.is_open()) {
        cerr << "Fatal Error! Configuration Missing. If this is the first time the project is using the BVCS. Please use 'Start' command." << endl;
        return; // Return an error code
    }
    inputfile.seekg(18);
    inputfile >> version;
    inputFile.seekg(35);
    getline(inputFile, branch_name);
    if (branch_name.empty()) {
        cerr << "Fatal Error! Branch name missing in configuration." << endl;
        return; // Return an error code
    }
    inputFile.close();
}

void cc_builder(int version){
    fs::path current_commit = fs::current_path() / ".bvcs" / "current_commit";
    fs::path staging_area = fs::current_path() / ".bvcs" / "staging_area";
    fs::path version_history = fs::current_path() / ".bvcs" / "version_history" / branch_name;
    fs::remove_all(current_commit);
    fs::create_directory(current_commit);
    cout<<version << endl;
    for (int i = 0; i <= version; i++) {
        fs::path version = version_history / to_string(i);
        for (const auto& file: fs::directory_iterator(version)) {
            string name = file.path().filename().string();
            if (fs::exists(current_commit / name)) {
                fs::remove(current_commit / name);
            }
            fs::copy_file(file, current_commit/name);
        }
    }
}

void versioning(){
    string base = fs::current_path().string() + "\\.bvcs";
    ifstream inputFile(base + "\\cnfg.txt");
    if (!inputFile.is_open()) {
        cerr << "Fatal Error! Configuration Missing. If this is the first time the project is using the BVCS. Please use 'Start' command." << endl;
        return; // Return an error code
    }
    string line;
    while (getline(inputFile, line)) {
        if(line.find("Current Version: ") != string::npos){
            stringstream ss(line.substr(17));
            ss >> version;
            break;
        }
    }
    fs::path staging_area = fs::u8path(base + "\\staging_area");
    fs::path target_path = fs::u8path(base + "\\version_history\\"+to_string(version));
    if (fs::is_empty(staging_area)) {
        cout << "Staging area is empty!" << endl;
        return;
    }
    fs::copy(staging_area, target_path, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    version++;
    inputFile.close();
    
    ofstream outputfile(base + "\\cnfg.txt", ios::trunc);
    if (!outputfile) {
        cerr << "Error opening file for writing!" << endl;
        return; // Return an error code
    }
    outputfile << "Current Version: " << version << endl;
    outputfile.close();
    cc_builder(version-1);
    fs::remove_all(staging_area);
    fs::create_directory(staging_area);
}

int main(){
    config_parser();
    versioning();
    cout << "Versioning completed successfully!" << endl;
    return 0;
}