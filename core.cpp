#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <filesystem> 
#include <algorithm>
#include "utils.hpp"
#include "branch.hpp"
using namespace std;
namespace fs = std::filesystem;

vector<string> file_list;
vector<string> directory_list;

void starter_function(){
    ifstream inputFile("ignore.txt");
    if (!inputFile.is_open()) {
        cerr << "Error Ignore File Not Found" << endl;
        return; // Return an error code
    }
    string line;
    bool files = false;
    bool directories = false;
    while (getline(inputFile, line)) {
        if(line == "--files"){
            files = true;
        }
        else if(line == "--directories"){
            directories = true;
            files = false;
        }
        else if(files){
            file_list.push_back(line);
        }
        else if(directories){
            directory_list.push_back(line);
        }
        else if(line == "--end"){
            break;
        }
    }
    return;
}

void n_store(const string& filename, const string& path = fs::current_path().string()){
    ofstream outputfile(".bvcs\\staging_area\\dir.json", ios::app | ios::binary);
    if (!outputfile) {
        cerr << "Error opening file for writing! 10" << endl;
        return; // Return an error code
    }
    ifstream inputFile(path+'\\'+filename);
    if (!inputFile.is_open()) {
        cerr << "Error opening file! 1" << endl;
        return; // Return an error code
    }
    stringstream buffer;
    buffer << inputFile.rdbuf(); 
    string fileContent = buffer.str();
    string hash = sha256(fileContent);
    string hashname = sha256(filename);
    outputfile << hashname << "," << hash << "\n";
    outputfile.close();
    inputFile.close();
    vector<uint8_t> compressedData = lz4Compress(reinterpret_cast<const uint8_t*>(fileContent.data()), fileContent.size());
    ofstream compressedFile(".bvcs\\staging_area\\" + hashname + ".lz4", ios::binary);
    if (!compressedFile) {
        cerr << "Error opening file for writing! 11" << endl;
        return; // Return an error code
    }
    compressedFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedData.size());
    compressedFile.close();
    return;
}

void n_change(const string& filename, const string& hash, streampos pos, const string& path = fs::current_path().string()){
    ifstream inputFile(path+'\\'+filename, ios::binary);
    if (!inputFile.is_open()) {
        cerr << "Error opening file! 0" << endl;
        return; // Return an error code
    }
    stringstream buffer;
    buffer << inputFile.rdbuf(); 
    string fileContent = buffer.str();
    // cout << "Checking file: " << filename << endl;
    // cout<< "File Content: " << fileContent << endl;
    string hash1 = sha256(fileContent);
    // cout<<hash1 << "," << hash << endl;
    if(hash1 == hash){
        return;
    } 
    else{
        cout << filename << " has been changed!" << endl;
        fstream file(".bvcs\\staging_area\\dir.json", ios::in | ios::out | ios::binary);
        if (!file) {
            cerr << "Error opening file for writing! 12" << endl;
            return; // Return an error code
        }
        cout<< pos << endl;
        string line;
        file.seekp(pos);
        getline(file, line,',');
        vector<uint8_t> compressedData = lz4Compress(reinterpret_cast<const uint8_t*>(fileContent.data()), fileContent.size());
        ofstream compressedFile(".bvcs\\staging_area\\" + line + ".lz4", ios::binary);
        if (!compressedFile) {
            cerr << "Error opening file for writing! 13" << endl;
            return; // Return an error code
        }
        compressedFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedData.size());
        compressedFile.close();
        file.seekg(pos+static_cast<streamoff>(65));
        file << hash1 << "\n";
        file.close();
        return;
    }
}

void checker(const string& filename, const string& path = fs::current_path().string()){ 
    ifstream inputFile(".bvcs\\current_commit\\dir.json", ios::binary);
    if (!inputFile.is_open()) {
        cerr << "Error opening file! 2" << endl;
        return; // Return an error code
    }
    string comp = sha256(filename);
    string line;
    bool found = false;
    // while (getline(inputFile, line)) {
    //     size_t pos = line.find(',');
    //     if (pos == string::npos) {
    //         cerr << "Invalid line format!" << endl;
    //         return;
    //     }
    //     string storedHash = line.substr(0, pos);
    //     if (comp == storedHash){
    //         found = true;
    //         streampos posix = inputFile.tellg();
    //         cout<< "before calling" << posix << endl;
    //         n_change(filename, line.substr(pos + 1), posix, path);
    //         break;
    //     }
    // }

    while (true) {
        streampos lineStart = inputFile.tellg(); // Store position BEFORE reading
        if (!getline(inputFile, line)) break;    // Read the line
        
        size_t pos = line.find(',');
        if (pos == string::npos) {
            cerr << "Invalid line format!" << endl;
            return;
        }
        string storedHash = line.substr(0, pos);
        if (comp == storedHash){
            found = true;
            // cout << "Line start position: " << lineStart << endl;
            n_change(filename, line.substr(pos + 1), lineStart, path);
            break;
        }
    }
    inputFile.close();
    if(!found){
        n_store(filename, path);
    }
}

void folder_struct_store(const string &current_directory,vector<string> &files, vector<string> &directories){
    ofstream outputfile(".bvcs\\staging_area\\dir_struct", ios::app);
    if(!outputfile.is_open()){
        cerr << "Error opening file! 3" << endl;
        return;
    }
    outputfile << current_directory <<"-";
    for (const string& file : files) {
        outputfile << file << ",";
    }
    outputfile << "-";
    for (const string& directory : directories) {
        outputfile << directory << ",";
    }
    outputfile << endl;
    outputfile.close();
    return;
}

void file_check(fs::path dirPath){

    vector<string> files;
    vector<string> directories;
    vector<fs::path> dir_list;

    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
        cerr << "Error: Path does not exist or is not a directory: "
             << dirPath.string() << endl;
        return; // Return an error code
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            string filename = entry.path().filename().string();
            if(find(file_list.begin(), file_list.end(), filename) != file_list.end()){
                continue;
            }
            files.push_back(filename);
        } else if (entry.is_directory()) {
            string directory_name = entry.path().filename().string();
            if(find(directory_list.begin(), directory_list.end(), directory_name) != directory_list.end()){
                continue;
            }
            directories.push_back(directory_name);
            dir_list.push_back(entry.path());
        }
    }
    string directory_name = dirPath.filename().string();
    folder_struct_store(directory_name,files, directories);
    for (const string& file : files) {
        checker(file, dirPath.string());
    }
    for (const fs::path& directory_iterator : dir_list) {
        file_check(directory_iterator);
    }
}



bool createHiddenDirectory(const std::string& dirName) {
    try {
        // Create the directory first
        std::filesystem::create_directory(dirName);
        
        // Use Windows attrib command to hide it
        std::string command = "attrib +h \"" + dirName + "\"";
        int result = std::system(command.c_str());
        
        if (result == 0) {
            std::cout << "Created hidden directory: " << dirName << std::endl;
            return true;
        } else {
            std::cerr << "Warning: Could not hide directory " << dirName << std::endl;
            return false;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
        return false;
    }
}

void start(){
    fs::path dirPath = fs::current_path(); 
    if(fs::exists(".bvcs")){
        cout << "BVCS already initialized!" << endl;
        return;
    }
    if (!createHiddenDirectory(".bvcs")) {
        cerr << "Error creating hidden directory!" << endl;
        return;
    }
    ofstream outputfile(".bvcs/cnfg.bin", ios::app | ios::binary);
    fs::path base = fs::u8path(dirPath.string() + "\\.bvcs");
    outputfile<<"Current Version: -1"<<endl;
    outputfile<<"Current Branch: Main"<<endl;
    outputfile.close();
    ifstream inputfile(".bvcs/cnfg.bin", ios::binary);
    if (!inputfile) {
        cerr << "Error 2!" << endl;
        return;
    }
    fs::create_directory(base/"current_commit");
    fs::create_directory(base/"staging_area");
    fs::create_directory(base/"version_history");
    fs::create_directory(base/"version_history"/"Main");
    cout<<"Initializing BVCS..."<<endl;
    ofstream outputfile2("ignore.txt");
    if (!outputfile2) {
        cerr << "Error opening file for writing!" << endl;
        return; // Return an error code
    }
    outputfile2 << "--files" << endl;
    outputfile2<< "Place Filenames here!" << endl;
    outputfile2 << "ignore.txt" << endl;
    outputfile2 << "core.exe" << endl;
    outputfile2 << "--directories" << endl;
    outputfile2<< "Place Directory Names here!" << endl;
    outputfile2 << ".bvcs" << endl;
    outputfile2 << "--end" << endl;
    outputfile2.close();
    cout<< "Please add files/directories to the ignore list in 'ignore.txt' file!" << endl;
    cout<< "Then press 'Start' to start staging!" << endl;
    if(cin.get() == '\n'){
        cout<< "Starting to stage the current project..." << endl;
        starter_function();
        ofstream outputfile1(".bvcs/current_commit/dir.json", ios::app);
        outputfile1.close();
        file_check(dirPath);
        return;
    }
    else{
        cout<< "BVCS not started!" << endl;
        return;
    }
    inputfile.close();
    return;
}

bool change(){
    fs::path base = fs::current_path() / ".bvcs"/"staging_area";
    fs::remove_all(base);
    fs::create_directory(base);
    fs::copy_file(fs::current_path()/".bvcs"/"current_commit"/"dir.json", base/"dir.json");
    file_check(fs::current_path());
    bool flag = false;
    ifstream inputFile(base.string() + "\\dir_struct");
    if (!inputFile.is_open()) {
        cerr << "Error opening file! 4" << endl;
        return false; // Return an error code
    }
    ifstream inputfile(fs::current_path().string() + "\\.bvcs\\current_commit\\dir_struct");
    if (!inputfile) {
        cerr << "Error opening file! 5" << endl;
        return false; // Return an error code
    }
    stringstream buffer;
    buffer << inputfile.rdbuf();
    string fileContent = buffer.str();
    string hash = sha256(fileContent);
    buffer.str("");
    buffer.clear();
    buffer << inputFile.rdbuf();
    string fileContent1 = buffer.str();
    string hash1 = sha256(fileContent1);
    if(hash!=hash1){
        cout << "Changes detected!" << endl;
        flag = true;
    }
    inputFile.close();
    inputfile.close();
    if(flag) return flag;
    for(const fs::directory_entry& file: fs::directory_iterator(base)){
        if(file.path().filename().string() != "dir_struct" && file.path().filename().string() != "dir.json"){
            flag = true;
            break;
        }
    }
    if(flag) return flag;
    else {
        cout << "No changes detected!" << endl;
        return flag;
    }
}

void change_upp(){
    bool go = change();
    if(!go){
        fs::remove_all(".bvcs/staging_area");
        fs::create_directory(".bvcs/staging_area");
        return;
    }
    return;
}

void remover(fs::path path, vector<string>& filenames){
    fs::path dirPath = path;
    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
        cerr << "Error: Path does not exist or is not a directory: "
             << dirPath.string() << endl;
        return; // Return an error code
    }
    for (const fs::directory_entry& entry : fs::directory_iterator(dirPath)) {
        string name = entry.path().filename().string();
        if(find(filenames.begin(), filenames.end(), name) != filenames.end() | find(directory_list.begin(), directory_list.end(), name) != directory_list.end() | find(file_list.begin(), file_list.end(),name) != file_list.end()){
            cout << "File is in ignore list!" << endl;
            continue;
        }
        fs::remove_all(entry.path());
    }
}

vector<string> splitstring(const string& str, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(str);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void file_create(fs::path basepath){
    fs::path ccpath = fs::current_path() / ".bvcs" / "current_commit"/ "dir_struct";
    ifstream inputFile(ccpath);
    if (!inputFile.is_open()) {
        cerr << "Error opening file! 6" << endl;
        return; // Return an error code
    }
    string line;
    vector<string> tokens;
    while (getline(inputFile, line)) {
        tokens = splitstring(line, '-');
        string directory_name = tokens[0];
        if (directory_name == basepath.filename().string()) {
            break;
        }
    }
    inputFile.close();
    if(basepath.filename() != tokens[0] | tokens.size() != 3){
        cout << "Error: Directory name mismatch!" << endl;
        return;
    }
    cout<< "Processing line: " << line << endl;
    vector<string> files = splitstring(tokens[1], ',');
    vector<string> directories = splitstring(tokens[2], ',');
    for (const string& file : files) {
        fs::path file_path = basepath / file; 
        string hashname = sha256(file);
        fs::path file_path1 = fs::current_path() / ".bvcs" / "current_commit" / (hashname + ".lz4");
        ifstream inputFile(file_path1, ios::binary);
        if (!inputFile.is_open()) {
            cerr << "Error opening file! 7" << endl;
            return; // Return an error code
        }
        stringstream buffer;
        buffer << inputFile.rdbuf();
        string fileContent = buffer.str();
        vector<uint8_t> decompressedData = lz4Decompress(reinterpret_cast<const uint8_t*>(fileContent.data()), fileContent.size());
        ofstream outputFile(file_path, ios::binary);
        if (!outputFile) {
            cerr << "Error opening file for writing!" << endl;
            return; // Return an error code
        }
        outputFile.write(reinterpret_cast<const char*>(decompressedData.data()), decompressedData.size());
        outputFile.close();
    }
    for (const string& directory : directories) {
        fs::path directory_path = basepath / directory;
        fs::create_directory(directory_path);
        file_create(directory_path);
    }

}

void cc_fetch(){
    fs::path current_commit = fs::current_path() / ".bvcs" / "current_commit";
    fs::path current_dir = fs::current_path();
    vector<string> general;
    general.push_back(".bvcs");
    remover(current_dir, general);
    file_create(fs::current_path());
    return;
}

void dismantle(){
    fs::remove_all(".bvcs");
    fs::remove_all("ignore.txt");
    cout << "BVCS dismantled!" << endl;
}

int main(int argc, char* argv[]) {
    starter_function();
    if (argc < 2) {
        return 1;
    }
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "start") {
            start(); // Has to modify to include a few things
        } else if (arg == "versioning") {
            versioning();
        } else if (arg == "change") {
            change_upp();
        } else if (arg == "dismantle") {
            dismantle();
        } else if (arg == "fetch") {
            cc_fetch();
        } else if (arg == "next") {
            versioning();
        } else if (arg == "version") {
             if (i + 2 < argc) {  // Check if next argument exists
                string branch_name = argv[++i];
                int version_number = stoi(argv[++i]);
                cc_builder(version_number, branch_name);
            } else {
                cout << "Version command requires a number argument" << endl;
            }
        } else if (arg == "branch") {
            if (i + 1 < argc) {  // Check if next argument exists
                new_branch(argv[++i]);
            } else {
                cout << "Branch command requires a branch name argument" << endl;
            }
        } else {
            cout << "Unknown command: " << arg << endl;
        }
    }
    return 0;
}

