#include <cstdlib>
#include <iostream>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;


int main(int argc, char* argv[]){
    if (argc < 2){
        std::cerr << "No command provided!" << std::endl;
        return 1;
    }

    std::string command = argv[1];
    fs::path base = fs::current_path();

    if (command == "start"){
        int result = std::system("cd C://Projects//gcc && g++ core.cpp utils.cpp branch.cpp json.cpp -o core.exe");
        if (result != 0) {
            std::cerr << "Error compiling core.cpp" << std::endl;
            return result;
        }
        std::cout << "Starting core.exe..." << std::endl;
        fs::copy( "C://Projects//gcc//core.exe" ,base / "core.exe", fs::copy_options::overwrite_existing);
        int hider = std::system("attrib +h core.exe");
        std::string command = "cd \"" + base.string() + "\" && core.exe start";
        result = std::system(command.c_str());
        if (result != 0) {
            std::cerr << "Error starting core.exe" << std::endl;
            return result;
        }
    }
    else if (command == "update"){
        int result = std::system("cd C://Projects//gcc && g++ core.cpp utils.cpp branch.cpp json.cpp -o core.exe");
        if (result != 0) {
            std::cerr << "Error compiling core.cpp" << std::endl;
            return result;
        }
        fs::remove(base / "core.exe");
        fs::copy("C://Projects//gcc//core.exe", base / "core.exe", fs::copy_options::overwrite_existing);
        std::cout << "core.exe updated successfully!" << std::endl;
    }
    // First, a minor but important fix: use logical OR '||' not bitwise OR '|'.
    else if (command == "version" || command == "branch") {

        if (command == "branch") {
            // The 'branch' command needs 3 arguments total (argc must be at least 3)
            if (argc < 3) {
                std::cerr << "Error: Branch name is required for the 'branch' command." << std::endl;
                return 1;
            }
            std::string branch_name = argv[2];
            int result = std::system(("core.exe branch " + branch_name).c_str());
            if (result != 0) {
                std::cerr << "Error executing command: branch " << branch_name << std::endl;
                return result;
            }
            // Successfully executed, return 0
            return 0; 
        }

        if (command == "version") {
            // The 'version' command needs 4 arguments total (argc must be at least 4)
            if (argc < 4) {
                std::cerr << "Error: Branch name and version number are required for the 'version' command." << std::endl;
                return 1;
            }
            // Now it's safe to access both argv[2] and argv[3]
            std::string branch_name = argv[2];
            std::string version_str = argv[3]; // It's safer to convert to string first
            
            try {
                int version = std::stoi(version_str);
                int result = std::system(("core.exe version " + branch_name + " " + std::to_string(version)).c_str());
                if (result != 0) {
                    std::cerr << "Error executing command: version " << branch_name << " " << version << std::endl;
                    return result;
                }
            } catch (const std::invalid_argument& e) {
                std::cerr << "Error: Invalid version number provided: '" << version_str << "'" << std::endl;
                return 1;
            } catch (const std::out_of_range& e) {
                std::cerr << "Error: Version number is out of range: '" << version_str << "'" << std::endl;
                return 1;
            }
            // Successfully executed, return 0
            return 0;
        }
    }
    else {
        int result = std::system(("core.exe " + command).c_str());
        if (result != 0) {
            std::cerr << "Error executing command: " << command << std::endl;
            return result;
        }
    }
    
    return 0;

    if (command == "dismantle") {
        fs::remove("core.exe");
    }
}