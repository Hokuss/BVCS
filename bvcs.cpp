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
        int result = std::system("cd C://Projects//gcc && g++ core.cpp utils.cpp -o core.exe");
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
    else if (command == "version"){
        if (argc < 3) {
            std::cerr << "Version number required!" << std::endl;
            return 1;
        }
        int version = std::stoi(argv[2]);
        int result = std::system(("core.exe version " + std::to_string(version)).c_str());
        if (result != 0) {
            std::cerr << "Error executing command: version " << version << std::endl;
            return result;
        }
    } else {
        int result = std::system(("core.exe " + command).c_str());
        if (result != 0) {
            std::cerr << "Error executing command: " << command << std::endl;
            return result;
        }
    }
}