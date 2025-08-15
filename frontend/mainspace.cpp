#include "mainspace.hpp"
#include "connector.hpp"

bool side_menu_opened = true;
bool main_text = false;
bool compare_text = false;

static Connector a;
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
            explorer->renameFile(((File*)renaming_item)->name, rename_buffer, parent);
        else
            explorer->renameDirectory(((Directory*)renaming_item)->name, rename_buffer, parent);
    }
    renaming_item = nullptr;
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
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##rename_file", rename_buffer, IM_ARRAYSIZE(rename_buffer),
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            EndRename(explorer, parent, true);
        }
        if (!ImGui::IsItemActive() && !ImGui::IsItemHovered())
            EndRename(explorer, parent, true);
    }
    else
    {
        ImGui::TreeNodeEx((void*)file, file_flags, ICON_FILE " %s", file->name.c_str());

        if (ImGui::IsItemClicked()){
            bool ctrl = ImGui::GetIO().KeyCtrl;
            bool shift = ImGui::GetIO().KeyShift;

            if (ctrl) {
                if (selected_items.count(file)) {
                    selected_items.erase(file);
                    last_selected_item = nullptr;
                } else {
                    selected_items.insert(file);
                    last_selected_item = file;
                }
            } else if (shift && last_selected_item) {
                auto it = selected_items.find(last_selected_item);
                if (it != selected_items.end()) {
                    selected_items.erase(it);
                }
                selected_items.insert(file);
                last_selected_item = file;
            } else {
                selected_items.clear();
                selected_items.insert(file);
                last_selected_item = file;
            }
        }

        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Open")){
                a.loadfile(file->name, parent);
                main_text = true;
            }
            if (ImGui::MenuItem("Save"))
                explorer->saveFile(file->name, a.data, parent);
            if (ImGui::MenuItem("Rename"))
                BeginRename(file, file->name);
            if (ImGui::MenuItem("Delete"))
                explorer->removeFile(file->name, parent);
            ImGui::EndPopup();
        }
    }
}

// Render directories recursively
void RenderDirectory(Directory* dir, FileExplorer* explorer, Directory* parent = nullptr)
{
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                    ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                    ImGuiTreeNodeFlags_SpanAvailWidth;

    if (last_selected_item == dir)
        node_flags |= ImGuiTreeNodeFlags_Selected;

    bool node_open = false;

    if (renaming_item == dir)
    {
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##rename_dir", rename_buffer, IM_ARRAYSIZE(rename_buffer),
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            EndRename(explorer, parent, false);
        }
        if (!ImGui::IsItemActive() && !ImGui::IsItemHovered())
            EndRename(explorer, parent, false);

        node_open = ImGui::TreeNodeEx((void*)dir, node_flags, "");
    }
    else
    {
        node_open = ImGui::TreeNodeEx((void*)dir, node_flags, ICON_FOLDER " %s", dir->name.c_str());

        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()){
            bool ctrl = ImGui::GetIO().KeyCtrl;
            bool shift = ImGui::GetIO().KeyShift;

            if (ctrl) {
                if (selected_items.count(dir)) {
                    selected_items.erase(dir);
                    last_selected_item = nullptr;
                } else {
                    selected_items.insert(dir);
                    last_selected_item = dir;
                }
            } else if (shift && last_selected_item) {
                auto it = selected_items.find(last_selected_item);
                if (it != selected_items.end()) {
                    selected_items.erase(it);
                }
                selected_items.insert(dir);
                last_selected_item = dir;
            } else {
                selected_items.clear();
                selected_items.insert(dir);
                last_selected_item = dir;
            }
        }
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Add File"))
                explorer->addFile("new_file.txt", "", dir);
            if (ImGui::MenuItem("Add Folder"))
                explorer->addDirectory("New Folder", dir);
            if (ImGui::MenuItem("Rename"))
                BeginRename(dir, dir->name);
            if (ImGui::MenuItem("Delete"))
                explorer->removeDirectory(dir->name, parent);
            ImGui::EndPopup();
        }
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

void ShowFileExplorerSidebar(FileExplorer* explorer)
{
    ImGui::SetNextWindowSize(ImVec2(250, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Explorer", nullptr, ImGuiWindowFlags_NoCollapse);

    RenderDirectory(&explorer->root_directory, explorer);

    ImGui::End();
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
            if (ImGui::Button("Ignore Files/Directory")) {
                a.Ignore();
                main_text = true;
            }

            if (ImGui::Button("Dismantle")) {
                a.dsmantle();
            }

            ImGui::EndChild();
        }
    }

    ImGui::SameLine();

    if(main_text && compare_text){
        ImGui::InputTextMultiline("##MainText", 
            const_cast<char*>(a.data.c_str()), 
            a.data.size() + 1,
            ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 2), 
            ImGuiInputTextFlags_CallbackResize, 
            a.TextEditCallback, 
            (void*)&a.data);
    
    } else if (main_text) {
        ImGui::BeginChild(a.file_name1.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() * 2), true);
        ImGui::Text("File: %s", a.file_name1.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Save & Close")) {
            a.save(a.file_name1);
            main_text = false;
        }
        ImGui::Separator();
        ImGui::InputTextMultiline("##MainText", 
            a.data.data(), 
            a.data.size() + 1,
            ImVec2(-1.0f, -1.0f), 
            ImGuiInputTextFlags_CallbackResize, 
            a.TextEditCallback, 
            (void*)&a.data);
        ImGui::EndChild();
    } else {
        ImGui::Text("No Content Selected");
    }

    // if(ImGui::Button("Test")){
    //     f.refresh();
    //     f.show();
    // }

    ShowFileExplorerSidebar(&a.f);
    

}