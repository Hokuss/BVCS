#include "mainspace.hpp"
#include "connector.hpp"
#include "IconsFontAwesome6.h"
#include "Soup.hpp"

bool side_menu_opened = true;
bool main_text = false;
bool compare_text = false;

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
        ImGui::TreeNodeEx((void*)file, file_flags,"%s %s", icon, file->name.c_str());
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
        node_open = ImGui::TreeNodeEx((void*)dir, node_flags, "");

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

void main_menu() {
    if(error_occured){
        Error();
    }
    ImGui::Text("Project: BVCS V-0.1");
    ImGui::SameLine();
    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - ImGui::CalcTextSize("Version: 0.1").x - 10, ImGui::GetTextLineHeight() + ImGui::CalcTextSize("Close").y + 30));
    ImGui::Text("Version: 0.1");

    ImGui::Separator();
    if(ImGui::Button("Side Menu")) {
        side_menu_opened = !side_menu_opened;
    }

    if (side_menu_opened) {

        float side_menu_width = 300.0f;

        ImVec2 remain_space = ImGui::GetContentRegionAvail();
        float side_menu_height = remain_space.y;

        if (ImGui::BeginChild("SideMenu", ImVec2(side_menu_width, side_menu_height), true)) {
            ImGui::Text("Side Menu");
            ImGui::Separator();

            ImGui::BeginChild("Actions", ImVec2(side_menu_width, side_menu_width/2), true);
            a.branches();

            Dropdown(a.current_branch, a.branch_names, "Branches");
            // Fix the branches function to return the correct version names instead of empty vector
            Dropdown(a.current_version, a.version_names, "Versions");

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
            
            ShowFileExplorerSidebar(&a.f, ImVec2(side_menu_width, remain_space.y - 10));

            ImGui::EndChild();

            
        }
    }

    ImGui::SameLine();

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
        else if (main_soup) {
            if (!main_soup->render()) {
                delete main_soup;
                main_soup = nullptr;
                main_text = false;
            }
        }
    }

    // if(ImGui::Button("Test")){
    //     f.refresh();
    //     f.show();
    // }
    

}