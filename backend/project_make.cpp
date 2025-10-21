#include <thread>
#include <chrono>
#include "project_make.hpp"

void make_project() {
    std::thread worker(build_project);
    worker.detach();
}

void build_project() {
    
}

