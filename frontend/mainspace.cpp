#include "universal.hpp"
#include "mainspace.hpp"
#include "connector.hpp"
#include "IconsFontAwesome6.h"
#include "Soup.hpp"
#include "json.hpp"
#include "imgui_stdlib.h"

bool side_menu_opened = true;
bool main_text = false;
bool compare_text = false;
bool options = false;

static Connector a;
Soup* main_soup = nullptr;
static std::unordered_set<void*> selected_items;
void* last_selected_item = nullptr;
static void* renaming_item = nullptr;
static char rename_buffer[256] = "";

void Error(){
        ImGui::OpenPopup("Error");
        if(ImGui::BeginPopupModal("Error", &error_occured, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Error: %s", error_message.c_str());
            if (ImGui::Button("Closer")) {
                // error_occured = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
}

void Dropdown(int &control, std::vector<std::string> &list, const std::string &label){

    if (list.empty() || control < 0 || control >= list.size()) {
        ImGui::Text("No items available");
        return;
    }

    if (ImGui::BeginCombo(label.c_str(), list[control].c_str())) {
        for (int i = 0; i < list.size(); i++) {
            bool is_selected = (control == i);
            if (ImGui::Selectable(list[i].c_str(), is_selected)) {
                control = i;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
}

// Start renaming an item
void BeginRename(void* item, const std::string& current_name)
{
    renaming_item = item;
    strncpy(rename_buffer, current_name.c_str(), sizeof(rename_buffer));
    rename_buffer[sizeof(rename_buffer) - 1] = '\0';
}

// Finish renaming and apply changes
void EndRename(FileExplorer* explorer, Directory* parent = nullptr, bool is_file = false)
{
    if (renaming_item && strlen(rename_buffer) > 0)
    {
        if (is_file)
            explorer->renameFile(((File*)renaming_item)->name, rename_buffer, parent, parent->path);
        else
            explorer->renameDirectory(((Directory*)renaming_item)->name, rename_buffer, parent, parent->path);
        explorer->refresh();
    }
    renaming_item = nullptr;
}

const char* GetFileIcon(const std::string& filename) {
    std::string ext;
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        ext = filename.substr(dot_pos + 1);
    }
    
    if (ext == "cpp" || ext == "cxx" || ext == "h" || ext == "hpp") {
        return ICON_FA_FILE_CODE;
    } else if (ext == "txt" || ext == "md") {
        return ICON_FA_FILE_LINES;
    } else if (ext == "jpg" || ext == "png" || ext == "gif") {
        return ICON_FA_FILE_IMAGE;
    } else if (ext == "pdf") {
        return ICON_FA_FILE_PDF;
    }
    // Add more conditions for other file types
    return ICON_FA_FILE;
}

// Helper function to get the color based on file extension
ImVec4 GetFileColor(const std::string& filename) {
    std::string ext;
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        ext = filename.substr(dot_pos + 1);
    }

    if (ext == "cpp" || ext == "cxx" || ext == "h" || ext == "hpp") {
        return ImVec4(0.0f, 0.5f, 1.0f, 1.0f); // Blue for C/C++
    } else if (ext == "txt" || ext == "md") {
        return ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // Gray for text
    } else if (ext == "jpg" || ext == "png" || ext == "gif") {
        return ImVec4(1.0f, 0.3f, 0.8f, 1.0f); // Pink for images
    } else if (ext == "pdf") {
        return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for PDFs
    }
    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // Default to white
}

void RenderFile(File* file, Directory* parent, FileExplorer* explorer)
{
    ImGuiTreeNodeFlags file_flags = ImGuiTreeNodeFlags_Leaf |
                                    ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                    ImGuiTreeNodeFlags_SpanAvailWidth;

    if (selected_items.count(file))
        file_flags |= ImGuiTreeNodeFlags_Selected;

    if (renaming_item == file)
    {
        ImGui::SetKeyboardFocusHere(0);
        ImGui::SetNextItemWidth(-1);
        bool rename_finished = ImGui::InputText("##rename_file", rename_buffer, IM_ARRAYSIZE(rename_buffer),
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        if (rename_finished || ImGui::IsItemDeactivated())
            EndRename(explorer, parent, true);
    }
    else
    {
        const char* icon = GetFileIcon(file->name);
        ImVec4 color = GetFileColor(file->name);

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TreeNodeEx(file->path.c_str(), file_flags, "%s %s", icon, file->name.c_str());
        ImGui::PopStyleColor();

        if (ImGui::IsItemClicked()){
            bool ctrl = ImGui::GetIO().KeyCtrl;

            if (ctrl) {
                if (selected_items.count(file)) {
                    selected_items.erase(file);
                    last_selected_item = nullptr;
                } else {
                    selected_items.insert(file);
                    last_selected_item = file;
                }
            }
        }

        if (ImGui::BeginPopupContextItem())
        {   
            if(selected_items.size()>=1){
                if (ImGui::MenuItem("Cut"))
                    explorer->cutSelected(selected_items);
                if (ImGui::MenuItem("Copy"))
                    explorer->copySelected(selected_items);
                if (ImGui::MenuItem("Delete"))
                    explorer->removeSelected(selected_items);
            }
            else {
                if (ImGui::MenuItem("Open")){
                    a.loadfile(file->name, parent);
                    main_text = true;
                }
                if (ImGui::MenuItem("Rename"))
                    BeginRename(file, file->name);
                if (ImGui::MenuItem("Delete"))
                    explorer->removeFile(file->name, parent, parent->path);
            }
            ImGui::EndPopup();
        }
    }
}

// Render directories recursively
void RenderDirectory(Directory* dir, FileExplorer* explorer, Directory* parent = nullptr)
{
    if (!dir) return;
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                    ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                    ImGuiTreeNodeFlags_SpanAvailWidth;

    if (selected_items.count(dir))
        node_flags |= ImGuiTreeNodeFlags_Selected;

    bool node_open = false;

    if (renaming_item == dir)
    {
        node_open = ImGui::TreeNodeEx((void*)dir, node_flags, "");
        ImGui::SameLine();
        ImGui::SetKeyboardFocusHere(0);
        ImGui::SetNextItemWidth(-1);
        bool rename_finished = ImGui::InputText("##rename_dir", rename_buffer, IM_ARRAYSIZE(rename_buffer),
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        if (rename_finished || ImGui::IsItemDeactivated())
        {
            EndRename(explorer, parent, false);
        }
    }
    else
    {
        const char* icon = dir->open ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER;
        dir->color = dir->open ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 0.5f, 1.0f);

        ImGui::SetNextItemOpen(dir->open);
        node_open = ImGui::TreeNodeEx(dir->path.c_str(), node_flags, "");

        dir->open = node_open;
        ImGui::SameLine();
        ImGui::TextColored(dir->color, icon);
        ImGui::SameLine();
        ImGui::TextUnformatted(dir->name.c_str());

        if (ImGui::IsItemClicked()){
            bool ctrl = ImGui::GetIO().KeyCtrl;

            if (ctrl) {
                if (selected_items.count(dir)) {
                    selected_items.erase(dir);
                    last_selected_item = nullptr;
                } else {
                    selected_items.insert(dir);
                    last_selected_item = dir;
                }
            }
        }

        std::string dir_id = "dir_context_menu" + dir->name + std::to_string(reinterpret_cast<uintptr_t>(dir));
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
           // Unique ID for each directory
            ImGui::OpenPopup(dir_id.c_str());
        }


        if (ImGui::BeginPopup(dir_id.c_str()))
        {
            if(selected_items.size()>=1){
                if (ImGui::MenuItem("Cut"))
                    explorer->cutSelected(selected_items);
                if (ImGui::MenuItem("Copy"))
                    explorer->copySelected(selected_items);
                if (ImGui::MenuItem("Delete"))
                    explorer->removeSelected(selected_items);
            }
            else {
                if (ImGui::MenuItem("Add File"))
                    explorer->addFile("new_file.txt", "", dir, dir->path);
                if (ImGui::MenuItem("Add Folder"))
                    explorer->addDirectory("New Folder", dir, dir->path);
                if (ImGui::MenuItem("Rename"))
                    BeginRename(dir, dir->name);
                if (ImGui::MenuItem("Delete"))
                    explorer->removeDirectory(dir->name, parent, parent->path);
                if (explorer->clipboard_active && ImGui::MenuItem("Paste"))
                {
                    explorer->paste(dir);
                }
            }
            ImGui::EndPopup();
        }
        // return;
    }

    if (node_open)
    {
        // Render files
        for (auto& file : dir->files)
            RenderFile(file, dir, explorer);

        // Render subdirectories
        for (auto& subdir : dir->subdirectories)
            RenderDirectory(subdir, explorer, dir);

        ImGui::TreePop();
    }
}

void ShowFileExplorerSidebar(FileExplorer* explorer, ImVec2 size = ImVec2(250, 0))
{
    ImGui::SetNextWindowSize(ImVec2(250, 0), ImGuiCond_FirstUseEver);
    ImGui::BeginChild("FileExplorerSidebar", size, true, ImGuiWindowFlags_MenuBar);

    if(ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()){
        selected_items.clear();
        last_selected_item = nullptr;
    }

    RenderDirectory(&explorer->root_directory, explorer);

    ImGui::EndChild();
}

void OptionsWindow(){
    ImGui::OpenPopup("Options Window");
    if(ImGui::BeginPopupModal("Options Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::Button("Save & Close")) {
            options = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::Text("Default Compiler: ");
        ImGui::SameLine();
        ImGui::InputText("##default_compiler", &project_options.default_compiler);

        static int index = -1;
        static char command_buffer[1024] = "";
        ImGui::Separator();
        ImGui::Text("Build Commands:");

        float list_height = 200.0f;

        ImGui::BeginChild("CommandButtons", ImVec2(100,list_height), false, ImGuiWindowFlags_NoScrollbar);

        if (ImGui::Button("Add Command")) {
            project_options.commands.push_back("new_command");
            index = project_options.commands.size() - 1;
            strncpy(command_buffer, "new_command", sizeof(command_buffer));
            ImGui::OpenPopup("RenameCommand");
        }

        bool item_selected = index >= 0 && index < project_options.commands.size();
        ImGui::BeginDisabled(!item_selected);
        {
            if (ImGui::Button("Remove Command")) {
                if (item_selected) {
                    project_options.commands.erase(project_options.commands.begin() + index);
                    index = -1;
                }
            }

            if (ImGui::Button("Change Command")) {
                if (item_selected) {
                    strncpy(command_buffer, project_options.commands[index].c_str(), sizeof(command_buffer));
                    ImGui::OpenPopup("RenameCommand");
                }
            }

            if (ImGui::Button("Move Up")) {
                if (item_selected && index > 0) {
                    std::swap(project_options.commands[index], project_options.commands[index - 1]);
                    index--;
                }
            }

            if (ImGui::Button("Move Down")) {
                if (item_selected && index < project_options.commands.size() - 1) {
                    std::swap(project_options.commands[index], project_options.commands[index + 1]);
                    index++;
                }
            }
        }
        ImGui::EndDisabled();
        ImGui::EndChild();

        ImGui::SameLine();

        // Fixes 


        size_t index_to_remove = -1;
        size_t index_to_rename = -1;
        size_t index_to_move_up = -1;
        size_t index_to_move_down = -1;

        float avail_width = ImGui::GetContentRegionAvail().x;
        ImGui::BeginChild("CommandList", ImVec2(avail_width, -1), true);

        for (size_t i = 0; i < project_options.commands.size(); ++i) {
            const bool is_selected = (index == i);
            if (ImGui::Selectable(project_options.commands[i].c_str(), is_selected)) {
                index = i;
            }

            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Remove Command")) {
                    index_to_remove = i;
                }
                if (ImGui::MenuItem("Change Command")) {
                    index_to_rename = i;
                }
                if (ImGui::MenuItem("Move Up")) {
                    index_to_move_up = i;
                }
                if (ImGui::MenuItem("Move Down")) {
                    index_to_move_down = i;
                }
                ImGui::EndPopup();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                index_to_rename = i;
            }
        }
        ImGui::EndChild();

        // Process deferred actions
        if (index_to_remove != (size_t)-1) {
            project_options.commands.erase(project_options.commands.begin() + index_to_remove);
            if (index >= index_to_remove && index > 0) {
                index--;
            }
        }
        if (index_to_rename != (size_t)-1) {
            index = index_to_rename;
            strncpy(command_buffer, project_options.commands[index].c_str(), sizeof(command_buffer));
            ImGui::OpenPopup("RenameCommand");
        }
        if (index_to_move_up != (size_t)-1) {
            size_t i = index_to_move_up;
            if (i > 0) {
                std::swap(project_options.commands[i], project_options.commands[i - 1]);
                if (index == i) index--;
                else if (index == i - 1) index++;
            }
        }
        if (index_to_move_down != (size_t)-1) {
            size_t i = index_to_move_down;
            if (i < project_options.commands.size() - 1) {
                std::swap(project_options.commands[i], project_options.commands[i + 1]);
                if (index == i) index++;
                else if (index == i + 1) index--;
            }
        }

        // Rename Command Popup
        if (ImGui::BeginPopupModal("RenameCommand", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("##rename_command", command_buffer, IM_ARRAYSIZE(command_buffer));

            if (ImGui::Button("OK")) {
                if (index >= 0 && index < project_options.commands.size() && strlen(command_buffer) > 0) {
                    project_options.commands[index] = std::string(command_buffer);
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::EndPopup();
    }
}

void main_menu() {
    if(error_occured){
        Error();
    }
    if(ImGui::Button("Side Menu")) {
        side_menu_opened = !side_menu_opened;
    }
    ImGui::SameLine();
    if(ImGui::Button("Options")){
        options = true;
    }
    if(options){
        OptionsWindow();
    }
    ImGui::SameLine();
    if (ImGui::Button("Run Commands")) {
        a.run_commands(project_options.commands);
        ImGui::OpenPopup("Command Output");
    }
    ImGui::SameLine();
    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - ImGui::CalcTextSize("Version: 0.1").x - 10, ImGui::GetTextLineHeight() + ImGui::CalcTextSize("Close").y));
    ImGui::Text("Version: 0.1");

    ImGui::Separator();

    if (side_menu_opened) {

        float side_menu_width = 300.0f;

        ImVec2 remain_space = ImGui::GetContentRegionAvail();
        float side_menu_height = remain_space.y;

        if (ImGui::BeginChild("SideMenu", ImVec2(side_menu_width, side_menu_height), true)) {
            ImGui::Text("Side Menu");
            ImGui::Separator();

            ImGui::BeginChild("Actions", ImVec2(side_menu_width, side_menu_width/2), true);
            try {
                a.branches();

                Dropdown(a.current_branch, a.branch_names, "Branches");
                
                Dropdown(a.current_version, a.version_names, "Versions");
            } catch (std::runtime_error &e) {
                ImGui::Text(e.what());
            }

            ImGui::EndChild();


            if (ImGui::Button("Start")) {
                a.start();
            }

            if (ImGui::Button("Change Detector")) {
                a.change_detector();
            }
            if (ImGui::Button("Next Version")) {
                a.next();
            }

            if (ImGui::Button("Dismantle")) {
                a.dsmantle();
            }
            // Immplement a Better file Explorer so that Automatc refresh could work
            // Also it will remove the bug of the Changing file/folder closing the file/folder

            // if(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - a.f.last) > std::chrono::milliseconds(2000))
            // {
            //     // 1. Don't refresh if user is currently renaming something (it interrupts typing)
            //     if (renaming_item == nullptr | last_selected_item != nullptr) 
            //     {
            //         a.f.refresh();
            

            //         a.f.last = std::chrono::steady_clock::now();
            //     }
            // }
            if(ImGui::Button("Refresh")) a.f.refresh();
            ShowFileExplorerSidebar(&a.f, ImVec2(side_menu_width, remain_space.y - 10));

            ImGui::EndChild();

            
        }
    }


    if (main_text) {
        if (!main_soup) {
            try {
                main_soup = new Soup(a.file_path1 / a.file_name1);
            } catch (const std::exception& e) {
                error_message = e.what();
                error_occured = true;
                main_text = false;
                main_soup = nullptr;
            }
        }
        
        ImGui::SameLine();

        if (main_soup) {
            if (!main_soup->render()) {
                delete main_soup;
                main_soup = nullptr;
                main_text = false;
            }
        }
    }

    if (ImGui::BeginPopupModal("Command Output", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Tasks:");
        ImGui::Separator();

        ImGui::BeginChild("OutputRegion", ImVec2(800, 400), true, ImGuiWindowFlags_HorizontalScrollbar);
        {
            std::vector<std::string> lines;
            {
                std::lock_guard<std::mutex> lock(a.mutex_);
                lines = a.result_lines;
            }
            for (int i = 0; i < project_options.commands.size(); ++i) {
                ImGui::TextUnformatted(("Command: " + project_options.commands[i]).c_str());
                ImGui::Separator();
                if (i < lines.size()) {
                    ImGui::TextUnformatted(("Command Output:\n" + lines[i]).c_str());
                } else {
                    ImGui::TextUnformatted("No output captured.");
                }
                ImGui::Separator();
            }
        }
        ImGui::EndChild();

        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

}