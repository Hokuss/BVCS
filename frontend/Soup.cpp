#include "Soup.hpp"
#include <cmath>

Soup::Soup(fs::path file) {
    if (!fs::exists(file) || !fs::is_regular_file(file)) {
        throw std::runtime_error("File does not exist.");
    }
    else if (!isTextFile(file.string())) {
        throw std::runtime_error("File is not a text file: " + file.string());
    }
    vector<uint8_t> raw = readBinaryFile(file.string());
    filelines.clear();
    filelines.emplace_back();
    for (uint8_t byte : raw) {
        if (byte == '\n') {
            filelines.emplace_back();
        } else {
            filelines.back().emplace_back(byte, wordtype::plain);
        }
    }
    file_name = file.filename().string();
    file_path = file.string();
}

Soup::~Soup() {
    // Cleanup if necessary
}

bool Soup::render() {
    ImVec2 remain_space_outer = ImGui::GetContentRegionAvail();
    ImGui::BeginChild(file_name.c_str(), ImVec2(remain_space_outer.x, remain_space_outer.y), ImGuiChildFlags_Borders);
    if (remain_space_outer.x <= 0 || remain_space_outer.y <= 0 || ImGui::Button("Close")) {
        ImGui::EndChild();
        return false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        saveFile();
    }
    ImGui::Separator();

    ImVec2 remain_space = ImGui::GetContentRegionAvail();
    ImGui::BeginChild((file_name + "_inner").c_str(), ImVec2(remain_space.x, remain_space.y), ImGuiChildFlags_None);

    float line_height = ImGui::GetTextLineHeightWithSpacing();
    if(line_height<=0.0f) line_height = 1.0f;

    if (remain_space.x <= 0 || remain_space.y <= 0) {
        ImGui::EndChild();
        ImGui::EndChild();
        return true;
    }

    // Calculate visible lines and characters
    int visible_lines = (int) (remain_space.y / line_height) + 1;
    int lines_count = scroll_y + visible_lines;

    int visible_chars = (int) (remain_space.x / char_font.x) + 1;
    int chars_count = scroll_x + visible_chars;

    char line_buffer[4096];

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImFont* font = ImGui::GetFont();
    float font_size = ImGui::GetFontSize();
    // float line_height = ImGui::GetTextLineHeightWithSpacing();

    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    if (canvas_size.y < line_height) {
        canvas_size.y = line_height;
    }

    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

    draw_list->PushClipRect(cursor_pos, ImVec2(cursor_pos.x + canvas_size.x, cursor_pos.y + canvas_size.y), IM_COL32(30, 30, 30, 255));

    int lines_drawn = 0;

    const ImU32 selection_color = IM_COL32(51, 153, 255, 100);
    editor start = selection_start;
    editor end = selection_end;
    if (start.line > end.line || (start.line == end.line && start.column > end.column)) {
        std::swap(start, end);
    }

    bool has_selection = start.line != -1 && end.line != -1 && (start.line != end.line || start.column != end.column);
    
    for(int i = scroll_y; i < lines_count && i < filelines.size(); i++) {
        const line& current_line = filelines[i];
        int cnt = 0;
        wordtype last_type = wordtype::max; // Invalid initial type to ensure first glyph is processed

        ImVec2 line_start_pos = ImVec2(cursor_pos.x, cursor_pos.y + (i - scroll_y) * line_height);

        float scroll_offset = GetColumnPixelOffset(current_line, scroll_x, font, font_size, tab_size);
        float line_start_x = line_start_pos.x - scroll_offset;

        if (has_selection && i >= start.line && i <= end.line) {
            int sel_start_col = (i == start.line) ? start.column : 0;
            int sel_end_col = (i == end.line) ? end.column : current_line.size();

            // Clamp selection columns to visible range
            sel_start_col = std::max(sel_start_col, scroll_x);
            sel_end_col = std::min(sel_end_col, chars_count);

            if (sel_start_col < sel_end_col) {
                float sel_start_x = line_start_x + GetColumnPixelOffset(current_line, sel_start_col, font, font_size, tab_size);
                float sel_end_x = line_start_x + GetColumnPixelOffset(current_line, sel_end_col, font, font_size, tab_size);

                // Draw selection rectangle
                draw_list->AddRectFilled(ImVec2(sel_start_x, line_start_pos.y), ImVec2(sel_end_x, line_start_pos.y + line_height), selection_color);
            }
        }

        float line_end_x = line_start_pos.x;

        for(int j = scroll_x; j < chars_count && j < current_line.size(); j++) {
            const glyph& g = current_line[j];
            if (g.color_type != last_type) {
                if (cnt > 0) {
                    line_buffer[cnt] = '\0';
                    ImU32 color = GetColor(last_type);

                    ImVec2 segment_size = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, line_buffer, line_buffer + cnt);

                    draw_list->AddText(ImVec2(line_end_x, line_start_pos.y), color, line_buffer, line_buffer + cnt);

                    line_end_x += segment_size.x;
                    cnt = 0;
                }
                last_type = g.color_type;
            }

            if (g.character == '\t') {
                int spaces_to_next_tab = tab_size - (cnt % tab_size);
                for (int s = 0; s < spaces_to_next_tab; s++) {
                    line_buffer[cnt++] = ' ';
                }
            } else if (cnt < sizeof(line_buffer) - 1) {
                line_buffer[cnt++] = g.character;
            } else {
                // Buffer full, flush it
                line_buffer[cnt] = '\0';
                ImU32 color = GetColor(last_type);

                ImVec2 segment_size = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, line_buffer, line_buffer + cnt);

                draw_list->AddText(ImVec2(line_end_x, line_start_pos.y), color, line_buffer, line_buffer + cnt);

                line_end_x += segment_size.x;
                cnt = 0;
                line_buffer[cnt++] = g.character; // Add current character to new buffer
            }
        }

        if (cnt > 0) {
            line_buffer[cnt] = '\0';
            ImU32 color = GetColor(last_type);

            ImVec2 segment_size = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, line_buffer, line_buffer + cnt);

            draw_list->AddText(ImVec2(line_end_x, line_start_pos.y), color, line_buffer, line_buffer + cnt);

            line_end_x += segment_size.x;
            cnt = 0;
        }

        lines_drawn++;
    }

    bool cursor_visible = edit.focused && edit.line >= scroll_y && edit.line < lines_drawn + scroll_y &&
                         edit.column >= scroll_x && edit.column < chars_count + scroll_x && edit.line < filelines.size() && (fmod(ImGui::GetTime(), 1.0) < 0.5);
    if (cursor_visible) {
        float caret_x = cursor_pos.x;
        float caret_y = cursor_pos.y + (edit.line - scroll_y) * line_height;

        const line& current_line = filelines[edit.line];
        std::string before_cursor;
        for (int j = scroll_x; j < edit.column && j < current_line.size(); j++) {
            const glyph& g = current_line[j];
            if (g.character == '\t') {
                int spaces_to_next_tab = tab_size - (before_cursor.size() % tab_size);
                before_cursor.append(spaces_to_next_tab, ' ');
            } else {
                before_cursor += g.character;
            }
        }

        ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, before_cursor.c_str(), NULL);

        caret_x += text_size.x;

        draw_list->AddLine(ImVec2(caret_x, caret_y), ImVec2(caret_x, caret_y + line_height), GetColor(wordtype::cursor), 1.0f);

    }

    draw_list->PopClipRect();

    ImGui::InvisibleButton("canvas", canvas_size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    if (ImGui::IsItemFocused()) {
        edit.focused = true;
    } else if (ImGui::IsItemActive()) {
        edit.focused = true;
    }

    if (ImGui::IsItemDeactivated()) {
        edit.focused = false;
    }

    if (ImGui::IsItemHovered()) {
        ImGuiIO& io = ImGui::GetIO();

        if (io.MouseWheel != 0.0f) {
            scroll_y -= (int)io.MouseWheel;
            
            scroll_y = std::max(0, scroll_y);
            if (!filelines.empty()) {
                scroll_y = std::min(scroll_y, (int)filelines.size() - 1);
            }
        }

        if (io.MouseWheelH != 0.0f) {
            scroll_x += (int)(io.MouseWheelH * tab_size);
            scroll_x = std::max(0, scroll_x);
        }
    }

    // Handle mouse input for the text area
    // Make sure this is right after the ImGui::InvisibleButton or item
    // that defines your text editor's interactive area.

    ImVec2 click_pos = ImGui::GetIO().MousePos;
    bool is_hovering = ImGui::IsItemHovered();

    // 1. Stop selection on mouse release
    if (ImGui::IsMouseReleased(0)) {
        selecting = false;
    }

    // 2. Start or update selection on click
    if (ImGui::IsItemClicked()) {
        selecting = true;
        
        // --- Find Line ---
        float relative_y = click_pos.y - cursor_pos.y;
        int clicked_line = scroll_y + static_cast<int>(relative_y / line_height);
        
        // Clamp to valid lines
        if (clicked_line < 0) clicked_line = 0;
        if (clicked_line >= filelines.size()) clicked_line = filelines.size() - 1;
        
        if (clicked_line >= 0 && clicked_line < filelines.size()) {
            edit.line = clicked_line;
            const line& current_line = filelines[edit.line];

            // --- Find Column (Accurate Method) ---
            float scroll_x_pixels = GetColumnPixelOffset(current_line, scroll_x, font, font_size, tab_size);
            float target_x = (click_pos.x - cursor_pos.x) + scroll_x_pixels;

            // Find the column closest to the target_x
            int new_col = 0;
            float last_col_offset = 0.0f;
            for (int j = 0; j < current_line.size(); j++) {
                float next_col_offset = GetColumnPixelOffset(current_line, j + 1, font, font_size, tab_size);
                float col_center = last_col_offset + (next_col_offset - last_col_offset) / 2.0f;

                if (target_x < col_center) {
                    new_col = j;
                    break;
                }
                last_col_offset = next_col_offset;
                new_col = j + 1; // If it's past the center, it belongs to the next char
            }
            edit.column = new_col;
            
            // --- Update Selection ---
            if (ImGui::GetIO().KeyShift) {
                // Shift-click: Extend selection to new cursor position
                selection_end = edit;
            } else {
                // Normal click: Start a new selection
                selection_start = edit;
                selection_end = edit;
            }
        }
    }

    // 3. Update selection on drag
    if (selecting && ImGui::IsMouseDragging(0) && is_hovering) {
        // This logic is identical to IsItemClicked, but it *only* updates
        // the end of the selection.
        
        // --- Find Line ---
        float relative_y = click_pos.y - cursor_pos.y;
        int clicked_line = scroll_y + static_cast<int>(relative_y / line_height);
        
        if (clicked_line < 0) clicked_line = 0;
        if (clicked_line >= filelines.size()) clicked_line = filelines.size() - 1;

        if (clicked_line >= 0 && clicked_line < filelines.size()) {
            edit.line = clicked_line;
            const line& current_line = filelines[edit.line];

            // --- Find Column (Accurate Method) ---
            float scroll_x_pixels = GetColumnPixelOffset(current_line, scroll_x, font, font_size, tab_size);
            float target_x = (click_pos.x - cursor_pos.x) + scroll_x_pixels;

            int new_col = 0;
            float last_col_offset = 0.0f;
            for (int j = 0; j < current_line.size(); j++) {
                float next_col_offset = GetColumnPixelOffset(current_line, j + 1, font, font_size, tab_size);
                float col_center = last_col_offset + (next_col_offset - last_col_offset) / 2.0f;

                if (target_x < col_center) {
                    new_col = j;
                    break;
                }
                last_col_offset = next_col_offset;
                new_col = j + 1;
            }
            edit.column = new_col;

            // --- Update Selection ---
            // When dragging, we *only* move the end of the selection
            selection_end = edit;
        }
    }

    if(edit.focused){
        ImGuiIO& io = ImGui::GetIO();

        for(int i = 0; i < io.InputQueueCharacters.Size; i++){
            unsigned int c = io.InputQueueCharacters[i];
            if (c>=32){ //Printable characters
                if(edit.line >= filelines.size()){
                    edit.line = filelines.size() - 1;
                }
                line& current_line = filelines[edit.line];
                if(edit.column > current_line.size()){
                    edit.column = current_line.size();
                }
                current_line.insert(current_line.begin() + edit.column, glyph(static_cast<char>(c), wordtype::plain));
                edit.column++;
            }
        }
        io.InputQueueCharacters.resize(0);

        if(ImGui::IsKeyPressed(ImGuiKey_Backspace)){
            if(edit.line < filelines.size()){
                line& current_line = filelines[edit.line];
                if(edit.column > 0 && edit.column <= current_line.size()){
                    current_line.erase(current_line.begin() + edit.column - 1);
                    edit.column--;
                } else if(edit.column == 0 && edit.line > 0){
                    // Merge with previous line
                    int prev_line_length = filelines[edit.line - 1].size();
                    filelines[edit.line - 1].insert(filelines[edit.line - 1].end(), current_line.begin(), current_line.end());
                    filelines.erase(filelines.begin() + edit.line);
                    edit.line--;
                    edit.column = prev_line_length;
                }
            }
        }

        if(ImGui::IsKeyPressed(ImGuiKey_Enter)){
            if(edit.line < filelines.size()){
                line& current_line = filelines[edit.line];
                line new_line;
                if(edit.column < current_line.size()){
                    new_line.insert(new_line.end(), current_line.begin() + edit.column, current_line.end());
                    current_line.erase(current_line.begin() + edit.column, current_line.end());
                }
                filelines.insert(filelines.begin() + edit.line + 1, new_line);
                edit.line++;
                edit.column = 0;
            } else if(edit.line == filelines.size()){
                filelines.emplace_back();
                edit.column = 0;
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
            if (edit.column > 0) edit.column--;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
            if (edit.column < filelines[edit.line].size()) edit.column++;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
            if (edit.line > 0){
                edit.line--;
                edit.column = std::min(edit.column, (int)filelines[edit.line].size());
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
            if (edit.line< filelines.size() - 1){
                edit.line++;
                edit.column = std::min(edit.column, (int)filelines[edit.line].size());
            }
        }

        if (edit.line < scroll_y) {
            scroll_y = edit.line;
        }
        if (edit.line >= lines_count){
            scroll_y = edit.line - visible_lines + 1;
        }

        if (edit.column < scroll_x){
            scroll_x = edit.column;
        }
        if (edit.column > chars_count){
            scroll_x = edit.column - visible_chars + 1;
        }
    }

    ImGui::EndChild();

    ImGui::EndChild();
    return true;
}

// This function calculates the pixel offset of a given column index
float Soup::GetColumnPixelOffset(
    const line& current_line, 
    int target_column, 
    ImFont* font,
    float font_size,
    int tab_size   
) {
    if (target_column <= 0) {
        return 0.0f;
    }

    // Use a buffer size that matches your render loop
    char line_buffer[1024]; 
    int cnt = 0;
    wordtype last_type = wordtype::max;
    float total_width = 0.0f;

    for (int j = 0; j < current_line.size() && j < target_column; j++) {
        const glyph& g = current_line[j];

        if (g.color_type != last_type) {
            if (cnt > 0) {
                line_buffer[cnt] = '\0';
                // Use the passed-in font and font_size
                ImVec2 segment_size = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, line_buffer, line_buffer + cnt);
                total_width += segment_size.x;
                cnt = 0;
            }
            last_type = g.color_type;
        }

        if (g.character == '\t') {
            // Use the passed-in tab_size
            int spaces_to_next_tab = tab_size - (cnt % tab_size);
            for (int s = 0; s < spaces_to_next_tab; s++) {
                line_buffer[cnt++] = ' ';
            }
        } else if (cnt < sizeof(line_buffer) - 1) {
            line_buffer[cnt++] = g.character;
        } else {
            // Buffer full
            line_buffer[cnt] = '\0';
            ImVec2 segment_size = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, line_buffer, line_buffer + cnt);
            total_width += segment_size.x;
            cnt = 0;
            line_buffer[cnt++] = g.character;
        }
    }

    // Flush remaining
    if (cnt > 0) {
        line_buffer[cnt] = '\0';
        ImVec2 segment_size = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, line_buffer, line_buffer + cnt);
        total_width += segment_size.x;
    }

    return total_width;
}

ImU32 Soup::GetColor(wordtype type){
    switch(type) {
        case wordtype::plain: return IM_COL32(255, 255, 255, 255); // White
        case wordtype::keyword: return IM_COL32(86, 156, 214, 255); // Blue
        case wordtype::number: return IM_COL32(181, 206, 168, 255); // Light Green
        case wordtype::string: return IM_COL32(206, 145, 120, 255); // Light Red
        case wordtype::charliteral: return IM_COL32(206, 145, 120, 255); // Light Red
        case wordtype::punctuation: return IM_COL32(220, 220, 220, 255); // Light Gray
        case wordtype::preprocessor: return IM_COL32(155, 155, 155, 255); // Gray
        case wordtype::identifier: return IM_COL32(255, 255, 255, 255); // White
        case wordtype::knownidentifier: return IM_COL32(78, 201, 176, 255); // Turquoise
        case wordtype::preprocidentifier: return IM_COL32(155, 155, 155, 255); // Gray
        case wordtype::comment: return IM_COL32(87, 166, 74, 255); // Green
        case wordtype::multilinecomment: return IM_COL32(87, 166, 74, 255); // Green
        case wordtype::background: return IM_COL32(30, 30, 30, 255); // Dark Gray
        case wordtype::cursor: return IM_COL32(255, 255, 255, 255); // Light Gray with transparency
        case wordtype::selection: return IM_COL32(51, 153, 255, 100); // Light Blue with transparency
        case wordtype::errormarker: return IM_COL32(255, 0, 0, 100); // Red with transparency
        case wordtype::breakpoint: return IM_COL32(255, 0, 0, 200); // Red with more opacity
        case wordtype::linenumber: return IM_COL32(128, 128, 128, 255); // Gray
        case wordtype::currentlinefill: return IM_COL32(50, 50, 50, 255); // Darker Gray
        case wordtype::currentlinefillinactive: return IM_COL32(40, 40, 40, 255); // Even Darker Gray
        case wordtype::currentlineedge: return IM_COL32(200, 200, 200, 255); // Light Gray
        default: return IM_COL32(255, 255, 255, 255); // Fallback to white
    }
}

bool Soup::saveFile() {
    std::ofstream ofs(file_path, std::ios::binary);
    if (!ofs) {
        return false;
    }
    for (const auto& line : filelines) {
        for (const auto& g : line) {
            ofs.put(g.character);
        }
        ofs.put('\n');
    }
    return true;
}