#ifndef BRANCH_HPP
#define BRANCH_HPP

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;
namespace fs = std::filesystem;

extern int version;
extern string branch_name;

void config_parser();
void config_writer(const string& branch_name, int version);
long long counter(const fs::path& dir_path);
void new_branch(const string& branch_name);
void recursive_copy(const string& branch_name, int version, long long remain);
void cc_builder(int version,const string& branch_name = "main");
void versioning();
void branch_merge();
void delete_branch(const string& branch_name);


#endif // BRANCH_HPP