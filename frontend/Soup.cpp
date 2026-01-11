#include <cmath>
#include <stack>
#include "Soup.hpp"
#include "universal.hpp"
#include <algorithm>
// #include "subprocess.hpp"

Soup::Soup(fs::path file) : text_changed(true), scanner_shouldrun(true) {
    if (!fs::exists(file) || !fs::is_regular_file(file)) {
        throw std::runtime_error("File does not exist.");
    }
    else if (!isTextFile(file.string())) {
        throw std::runtime_error("File is not a text file: " + file.string());
    }
    std::vector<uint8_t> raw = readBinaryFile(file.string());
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

    // Start the scanner thread
    scan_thread = std::thread(&Soup::scanner, this);
    scan_thread.detach();
}

Soup::~Soup() {
    // Signal the scanner thread to stop
    scanner_shouldrun = false;

    if (scan_thread.joinable()) {
        scan_thread.join();
    }
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
    ImGui::SameLine();
    ImGui::Text("Errors: %d", static_cast<int>(errors.size()));
    // if (errors.size() > 0) {
    //     ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Compile Error at Line %d, Column %d, Message %s", errors[0].line, errors[0].column, errors[0].message.c_str());
    //     ImGui::SameLine();
    // }
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
    int visible_chars = (int) (remain_space.x / char_font.x) + 1;

    ImVec2 canvas_top_left = ImGui::GetCursorScreenPos();
    DrawEditor(remain_space, canvas_top_left, line_height, visible_lines, visible_chars);

    ImGui::InvisibleButton("canvas", remain_space, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    HandleMouseInput(canvas_top_left, line_height);
    HandleKeyboardInput();
    ScrollToCursor(visible_lines, visible_chars);
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
        case wordtype::plain: return IM_COL32(225, 255, 255, 255); // White
        case wordtype::keyword: return IM_COL32(86, 156, 214, 255); // Blue
        case wordtype::number: return IM_COL32(181, 206, 168, 255); // Light Green
        case wordtype::string: return IM_COL32(206, 145, 120, 255); // Light Red
        case wordtype::charliteral: return IM_COL32(206, 145, 120, 255); // Light Red
        case wordtype::punctuation: return IM_COL32(220, 220, 220, 255); // Light Gray
        case wordtype::preprocessor: return IM_COL32(155, 155, 155, 255); // Gray
        case wordtype::control: return IM_COL32(138, 43, 226, 255);      // Blue Violet
        case wordtype::identifier: return IM_COL32(78, 201, 176, 255);  // Light Teal
        case wordtype::type: return IM_COL32(173, 216, 230, 255); // Turquoise
        case wordtype::function: return IM_COL32(220, 220, 170, 255); // Light Yellow
        case wordtype::preprocidentifier: return IM_COL32(155, 155, 155, 255); // Gray
        case wordtype::comment: return IM_COL32(87, 166, 74, 255); // Green
        case wordtype::multilinecomment: return IM_COL32(87, 166, 74, 255); // Green
        case wordtype::background: return IM_COL32(30, 30, 30, 255); // Dark Gray
        case wordtype::cursor: return IM_COL32(255, 255, 255, 255); // White
        case wordtype::selection: return IM_COL32(51, 153, 255, 100); // Light Blue with transparency
        case wordtype::errormarker: return IM_COL32(255, 0, 0, 200); // Red with transparency
        case wordtype::breakpoint: return IM_COL32(255, 0, 0, 200); // Red with more opacity
        case wordtype::linenumber: return IM_COL32(128, 128, 128, 255); // Gray
        case wordtype::currentlinefill: return IM_COL32(50, 50, 50, 255); // Darker Gray
        case wordtype::currentlinefillinactive: return IM_COL32(40, 40, 40, 255); // Even Darker Gray
        case wordtype::currentlineedge: return IM_COL32(200, 200, 200, 255); // Light Gray
        case wordtype::bracket: return IM_COL32(255, 255, 255, 255); // White as fallback
        default: return IM_COL32(255, 255, 255, 255); // Fallback to white
    }
}

ImU32 Soup::GetColor(wordtype type, int variant) {
    // Define a set of colors for bracket variants
    // The colors are changed from fully saturated primary colors to softer, pastel/muted tones.
    static const ImU32 bracket_colors[] = {
        IM_COL32(230, 140, 140, 255),    // Soft Red/Coral
        IM_COL32(160, 220, 160, 255),    // Soft Green/Mint
        IM_COL32(140, 180, 230, 255),    // Soft Blue/Sky Blue
        IM_COL32(230, 230, 140, 255),    // Soft Yellow/Cream 
        IM_COL32(230, 180, 100, 255),    // Muted Orange/Peach
        IM_COL32(180, 140, 230, 255)     // Muted Purple/Lavender
    };
    
    // Calculate the number of colors available
    static const int num_bracket_colors = sizeof(bracket_colors) / sizeof(bracket_colors[0]);

    if (type == wordtype::bracket) {
        // Use modulo to cycle through the defined colors
        return bracket_colors[variant % num_bracket_colors];
    } else {
        // Fallback to the non-variant color function
        return GetColor(type);
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

void Soup::DrawEditor(ImVec2 size, ImVec2 canvas_top_left, float line_heinght, int visible_lines, int visible_chars) {

    std::lock_guard<std::mutex> lock(line_mutex);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImFont* font = ImGui::GetFont();
    float font_size = ImGui::GetFontSize();

    int lines_count = scroll_y + visible_lines;
    int chars_count = scroll_x + visible_chars;

    char line_buffer[4096]; // Buffer for rendering text segments

    draw_list->PushClipRect(canvas_top_left, ImVec2(canvas_top_left.x + size.x, canvas_top_left.y + size.y), true);

    const ImU32 selection_color = GetColor(wordtype::selection);
    editor sel_start = selection_start;
    editor sel_end = selection_end;
    if (sel_start.line > sel_end.line || (sel_start.line == sel_end.line && sel_start.column > sel_end.column)) {
        std::swap(sel_start, sel_end);
    }
    bool has_selection = !(sel_start.line == sel_end.line && sel_start.column == sel_end.column);

    int lines_drawn = 0;

    for(int i = scroll_y; i < filelines.size() && lines_drawn < visible_lines; i++, lines_drawn++) {
        const line& current_line = filelines[i];
        ImVec2 line_start_pos = ImVec2(canvas_top_left.x, canvas_top_left.y + lines_drawn * line_heinght);

        float scroll_offset = GetColumnPixelOffset(current_line, scroll_x, font, font_size, tab_size);
        float line_start_x = line_start_pos.x - scroll_offset;

        // Select and highlight text
        if (has_selection && i >= sel_start.line && i <= sel_end.line) {
            int sel_start_col = (i == sel_start.line) ? sel_start.column : 0;
            int sel_end_col = (i == sel_end.line) ? sel_end.column : current_line.size();

            float sel_start_x = line_start_x + GetColumnPixelOffset(current_line, sel_start_col, font, font_size, tab_size);
            float sel_end_x = line_start_x + GetColumnPixelOffset(current_line, sel_end_col, font, font_size, tab_size);

            // Clamp selection rectangle to the visible area
            sel_start_x = std::max(sel_start_x, canvas_top_left.x);
            sel_end_x = std::min(sel_end_x, canvas_top_left.x + size.x);

            if (sel_start_x < sel_end_x) {
                draw_list->AddRectFilled(
                    ImVec2(sel_start_x, line_start_pos.y),
                    ImVec2(sel_end_x, line_start_pos.y + line_heinght),
                    selection_color
                );
            }
        }

        // Error underlines
        for (const auto& err : errors) {
            if (err.line == i) {
                int err_col = err.column;
                float err_start_x = line_start_x + GetColumnPixelOffset(current_line, err_col, font, font_size, tab_size);
                float err_end_x = line_start_x + GetColumnPixelOffset(current_line, err_col + 1, font, font_size, tab_size);

                // Clamp error underline to the visible area
                err_start_x = std::max(err_start_x, canvas_top_left.x);
                err_end_x = std::min(err_end_x, canvas_top_left.x + size.x);

                if (err_start_x < err_end_x) {
                    draw_list->AddLine(
                        ImVec2(err_start_x, line_start_pos.y + line_heinght - 2),
                        ImVec2(err_end_x, line_start_pos.y + line_heinght - 2),
                        GetColor(wordtype::errormarker),
                        2.0f
                    );
                }

                editor mouse_pos = GetCoordinatesFromMousePos(canvas_top_left, line_heinght);
                if (mouse_pos.line == err.line && mouse_pos.column == err.column) {
                    ImGui::SetTooltip("%s", err.message.c_str());
                }
            }
        }

        int cnt = 0;
        wordtype last_type = wordtype::max;
        float line_x = line_start_x;

        for(int j = 0; j < current_line.size() && j < chars_count; j++) {
            const glyph& g = current_line[j];

            if (g.color_type != last_type) {
                if (cnt > 0) {
                    line_buffer[cnt] = '\0';
                    ImVec2 segment_size = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, line_buffer, line_buffer + cnt);
                    if (line_x + segment_size.x > canvas_top_left.x) {
                        ImU32 color;
                        if (last_type == wordtype::bracket) {
                            const glyph& last_glyph = current_line[j - 1];
                            color = GetColor(wordtype::bracket, (last_glyph.number_of_brackets % 6));
                        } else {
                            color = GetColor(last_type);
                        }
                        draw_list->AddText(font, font_size, ImVec2(line_x, line_start_pos.y), color, line_buffer, line_buffer + cnt);
                    }
                    line_x += segment_size.x;
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
                line_buffer[cnt] = '\0';
                ImVec2 segment_size = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, line_buffer, line_buffer + cnt);
                if (line_x + segment_size.x > canvas_top_left.x) {
                    ImU32 color;
                        if (last_type == wordtype::bracket) {
                            const glyph& last_glyph = current_line[j - 1];
                            color = GetColor(wordtype::bracket, (last_glyph.number_of_brackets % 6));
                        } else {
                            color = GetColor(last_type);
                        }
                        draw_list->AddText(font, font_size, ImVec2(line_x, line_start_pos.y), color, line_buffer, line_buffer + cnt);
                }
                line_x += segment_size.x;
                cnt = 0;
                line_buffer[cnt++] = g.character;
            }
        }

        if (cnt > 0) {
            line_buffer[cnt] = '\0';
            ImVec2 segment_size = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, line_buffer, line_buffer + cnt);
            if (line_x + segment_size.x > canvas_top_left.x) {
                ImU32 color;
                if (last_type == wordtype::bracket) {
                    const glyph& last_glyph = current_line[current_line.size() - 1];
                    color = GetColor(wordtype::bracket, (last_glyph.number_of_brackets % 6));
                } else {
                    color = GetColor(last_type);
                }
                draw_list->AddText(font, font_size, ImVec2(line_x, line_start_pos.y), color, line_buffer, line_buffer + cnt);
            }
            line_x += segment_size.x;
        }
    }

    bool cursor_visible = edit.focused && edit.line >= scroll_y && edit.line < scroll_y + visible_lines && (fmod(ImGui::GetTime(), 1.0) < 0.5);
    if (cursor_visible) {
        float caret_x_offset = GetColumnPixelOffset(filelines[edit.line], edit.column, font, font_size, tab_size);
        float scroll_offset = GetColumnPixelOffset(filelines[edit.line], scroll_x, font, font_size, tab_size);
        float cursor_x = canvas_top_left.x + caret_x_offset - scroll_offset;
        float cursor_y = canvas_top_left.y + (edit.line - scroll_y) * line_heinght;

        // Draw cursor as a vertical line
        draw_list->AddLine(
            ImVec2(cursor_x, cursor_y),
            ImVec2(cursor_x, cursor_y + line_heinght),
            GetColor(wordtype::cursor),
            1.0f
        );
    }

    draw_list->PopClipRect();
}

Soup::editor Soup::GetCoordinatesFromMousePos(ImVec2 canvas_top_left, float line_height) {
    ImVec2 click_pos = ImGui::GetIO().MousePos;
    ImFont* font = ImGui::GetFont();
    float font_size = ImGui::GetFontSize();
    
    editor coords;

    // --- Find Line ---
    float relative_y = click_pos.y - canvas_top_left.y;
    coords.line = scroll_y + static_cast<int>(relative_y / line_height);
    
    // Clamp to valid lines
    if (coords.line < 0) coords.line = 0;
    if (coords.line >= filelines.size()) coords.line = filelines.empty() ? 0 : filelines.size() - 1;
    
    if (coords.line < 0 || coords.line >= filelines.size()) {
        coords.line = 0;
        coords.column = 0;
        return coords; // No lines in file
    }

    // --- Find Column (Accurate Method) ---
    const line& current_line = filelines[coords.line];
    float scroll_x_pixels = GetColumnPixelOffset(current_line, scroll_x, font, font_size, tab_size);
    float target_x = (click_pos.x - canvas_top_left.x) + scroll_x_pixels;

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
    coords.column = new_col;

    return coords;
}

void Soup::HandleMouseInput(ImVec2 canvas_top_left, float line_height) {
    
    // --- 1. Handle Focus ---
    if (ImGui::IsItemFocused() || ImGui::IsItemActive()) {
        edit.focused = true;
    }
    if (ImGui::IsItemDeactivated()) {
        edit.focused = false;
    }

    bool is_hovering = ImGui::IsItemHovered();

    // --- 2. Handle Mouse Wheel Scrolling ---
    if (is_hovering) {
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

    // --- 3. Handle Mouse Clicks and Drags ---
    if (ImGui::IsMouseReleased(0)) {
        selecting = false;
    }

    if (ImGui::IsItemClicked()) {
        selecting = true;
        edit = GetCoordinatesFromMousePos(canvas_top_left, line_height);
        
        if (ImGui::GetIO().KeyShift) {
            selection_end = edit; // Extend selection
        } else {
            selection_start = edit; // Start new selection
            selection_end = edit;
        }
    }

    if (selecting && ImGui::IsMouseDragging(0) && is_hovering) {
        edit = GetCoordinatesFromMousePos(canvas_top_left, line_height);
        selection_end = edit; // Always update the end of the selection when dragging
    }
}

void Soup::HandleKeyboardInput() {
    if (!edit.focused) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    // Handle special keys
    if (io.KeyCtrl) {
        HandleCtrlShortcuts();
        return; // Skip other key handling when Ctrl is pressed
    }

    // --- 1. Printable Characters ---
    for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
        unsigned int c = io.InputQueueCharacters[i];
        if (c >= 32) { // Printable characters
            if (edit.line >= filelines.size()) {
                 // Handle edge case where cursor is on a non-existent line
                edit.line = filelines.empty() ? -1 : filelines.size() - 1;
                if (edit.line == -1) { // File is empty
                    filelines.emplace_back();
                    edit.line = 0;
                }
            }
            line& current_line = filelines[edit.line];
            if (edit.column > current_line.size()) {
                edit.column = current_line.size();
            }
            DeleteSelection(); 
            current_line.insert(current_line.begin() + edit.column, glyph(static_cast<char>(c), wordtype::plain));
            edit.column++;
            text_changed = true;
            
        }
    }
    io.InputQueueCharacters.resize(0);
    // --- 2. Backspace ---
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        if (selection_start.line != selection_end.line || selection_start.column != selection_end.column) {
            DeleteSelection();
        } else if (edit.line >= 0 && edit.line < filelines.size()) {
            line& current_line = filelines[edit.line];
            if (edit.column > 0 && edit.column <= current_line.size()) {
                current_line.erase(current_line.begin() + edit.column - 1);
                edit.column--;
            } else if (edit.column == 0 && edit.line > 0) {
                // Merge with previous line
                int prev_line_length = filelines[edit.line - 1].size();
                filelines[edit.line - 1].insert(filelines[edit.line - 1].end(), current_line.begin(), current_line.end());
                filelines.erase(filelines.begin() + edit.line);
                edit.line--;
                edit.column = prev_line_length;
            }
        }
        text_changed = true;
    }

    // --- 3. Enter Key ---
    if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        if (edit.line >= 0 && edit.line < filelines.size()) {
            line& current_line = filelines[edit.line];
            line new_line;
            if (edit.column < current_line.size()) {
                new_line.insert(new_line.end(), current_line.begin() + edit.column, current_line.end());
                current_line.erase(current_line.begin() + edit.column, current_line.end());
            }
            filelines.insert(filelines.begin() + edit.line + 1, new_line);
            edit.line++;
            edit.column = 0;
        } else if (filelines.empty()) {
            filelines.emplace_back();
            edit.line = 0;
            edit.column = 0;
        }
        text_changed = true;
    }

    // --- 4. Arrow Keys ---
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
        if (edit.column > 0) edit.column--;
        else if (edit.line > 0) {
            edit.line--;
            edit.column = filelines[edit.line].size();
        }
        selection_start = selection_end = editor::invalid(); // Clear selection on move
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
        if (edit.line < filelines.size() && edit.column < filelines[edit.line].size()) edit.column++;
        else if (edit.line < filelines.size() - 1) {
            edit.line++;
            edit.column = 0;
        }
        selection_start = selection_end = editor::invalid(); // Clear selection on move
    }
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
        if (edit.line > 0) {
            edit.line--;
            edit.column = std::min(edit.column, (int)filelines[edit.line].size());
        }
        selection_start = selection_end = editor::invalid(); // Clear selection on move
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
        if (edit.line < filelines.size() - 1) {
            edit.line++;
            edit.column = std::min(edit.column, (int)filelines[edit.line].size());
        }
        selection_start = selection_end = editor::invalid(); // Clear selection on move
    }
}

void Soup::HandleCtrlShortcuts() {
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsKeyPressed(ImGuiKey_A)) {
        // Select All
        if (!filelines.empty()) {
            selection_start = editor(0, 0);
            selection_end = editor(filelines.size() - 1, filelines.back().size());
            edit = selection_end; // Move cursor to end
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_X) || ImGui::IsKeyPressed(ImGuiKey_C)) {
        // Cut or Copy
        if (selection_start != selection_end && selection_start.is_valid() && selection_end.is_valid()) {
            editor sel_start = selection_start;
            editor sel_end = selection_end;
            if (sel_start.line > sel_end.line || (sel_start.line == sel_end.line && sel_start.column > sel_end.column)) {
                std::swap(sel_start, sel_end);
            }

            std::string text_to_copy;

            if (sel_start.line == sel_end.line) {
                // Single Line
                for(int j = sel_start.column; j < sel_end.column; j++) {
                    if (j < filelines[sel_start.line].size()) {
                        text_to_copy += filelines[sel_start.line][j].character;
                    }
                }
            } else {
                // Multi-Line
                for(int j = sel_start.column; j < filelines[sel_start.line].size(); j++) {
                    text_to_copy += filelines[sel_start.line][j].character;
                }
                text_to_copy += '\n';
                for(int i = sel_start.line + 1; i < sel_end.line; i++) {
                    for(const auto& g : filelines[i]) {
                        text_to_copy += g.character;
                    }
                    text_to_copy += '\n';
                }
                for(int j = 0; j < sel_end.column; j++) {
                    if (j < filelines[sel_end.line].size()) {
                        text_to_copy += filelines[sel_end.line][j].character;
                    }
                }
            }

            ImGui::SetClipboardText(text_to_copy.c_str());
            if (ImGui::IsKeyPressed(ImGuiKey_X)) {
                DeleteSelection();
                selection_start = selection_end = editor::invalid();
            }
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_V)) {
        const char* clipboard_text = ImGui::GetClipboardText();

        if (clipboard_text && clipboard_text[0] != '\0') {
            if (selection_start != selection_end && selection_start.is_valid() && selection_end.is_valid()) {
                DeleteSelection();
                selection_start = selection_end = editor::invalid();
            }

            std::string text_to_paste(clipboard_text);

            if(filelines.empty()) {
                filelines.emplace_back();
                edit.line = 0;
                edit.column = 0;
            }

            line& first_line_ref = filelines[edit.line];

            line reamining_line(first_line_ref.begin() + edit.column, first_line_ref.end());
            first_line_ref.erase(first_line_ref.begin() + edit.column, first_line_ref.end());

            std::istringstream iss(text_to_paste);
            std::string line_from_clipboard;
            bool first_line = true;

            while (std::getline(iss, line_from_clipboard)) {
                line new_line;
                for (char ch : line_from_clipboard) {
                    new_line.emplace_back(ch, wordtype::plain);
                }
                if (first_line) {
                    first_line_ref.insert(first_line_ref.end(), new_line.begin(), new_line.end());
                    edit.column = first_line_ref.size();
                    first_line = false;
                } else {
                    edit.line++;
                    filelines.insert(filelines.begin() + edit.line, new_line);
                    edit.column = new_line.size();
                }
            }

            line& last_line_ref = filelines[edit.line];
            last_line_ref.insert(last_line_ref.end(), reamining_line.begin(), reamining_line.end());
        }
    }
}

void Soup::DeleteSelection() {
    if (selection_start.line != selection_end.line || selection_start.column != selection_end.column) {
        editor sel_start = selection_start;
        editor sel_end = selection_end;
        if (sel_start.line > sel_end.line || (sel_start.line == sel_end.line && sel_start.column > sel_end.column)) {
            std::swap(sel_start, sel_end);
        }

        if (sel_start.line == sel_end.line) {
            // Single line selection
            line& line_ref = filelines[sel_start.line];
            line_ref.erase(line_ref.begin() + sel_start.column, line_ref.begin() + sel_end.column);
        } else {
            // Multi-line selection
            // Remove part from the start line
            line& start_line = filelines[sel_start.line];
            start_line.erase(start_line.begin() + sel_start.column, start_line.end());

            // Remove part from the end line
            line& end_line = filelines[sel_end.line];
            end_line.erase(end_line.begin(), end_line.begin() + sel_end.column);

            // Merge lines
            start_line.insert(start_line.end(), end_line.begin(), end_line.end());
            
            // Erase intermediate lines
            filelines.erase(filelines.begin() + sel_start.line + 1, filelines.begin() + sel_end.line + 1);
        }

        edit.line = sel_start.line;
        edit.column = sel_start.column;

        // Clear selection
        selection_start = selection_end = editor::invalid();
        text_changed = true;
    }
}

void Soup::ScrollToCursor(int visible_lines, int visible_chars) {
    if (!edit.focused) {
        return;
    }

    // --- Vertical Scroll ---
    if (edit.line < scroll_y) {
        scroll_y = edit.line;
    }
    if (edit.line >= scroll_y + visible_lines) {
        scroll_y = edit.line - visible_lines + 1;
    }

    // --- Horizontal Scroll ---
    if (edit.column < scroll_x) {
        scroll_x = edit.column;
    }
    // Note: 'chars_count' was calculated using scroll_x, so we use visible_chars
    if (edit.column >= scroll_x + visible_chars) {
        scroll_x = edit.column - visible_chars + 1;
    }
    
    // Clamp scroll values
    scroll_y = std::max(0, scroll_y);
    scroll_x = std::max(0, scroll_x);
}

void Soup::scanner() {
    while (scanner_shouldrun) {
        lines lines_to_scan;
        if (text_changed) {
            std::string filetype; // Make a copy to work on
            {
                std::lock_guard<std::mutex> lock(line_mutex);
                lines_to_scan = this->filelines;
                filetype = file_name;
            }

            filetype = filetype.substr(filetype.find_last_of('.') + 1);
            if(filetype.empty()) filetype = "txt";
            
            if (filetype == "cpp" || filetype == "cxx" || filetype == "cc" || filetype == "c" || filetype == "h" || filetype == "hpp" || filetype == "hxx") {
                scancplusplus(lines_to_scan);
            } 
            else if (filetype == "py") {
                // scanpython(lines_to_scan);
            } 
            else if (filetype == "js" || filetype == "jsx" || filetype == "ts" || filetype == "tsx") {
                // JavaScriptScanner::scan(lines_to_scan);
            } 
            else if (filetype == "java") {
                // JavaScanner::scan(lines_to_scan);
            } 
            else if (filetype == "rs") {
                // RustScanner::scan(lines_to_scan);
            } 
            else if (filetype == "go") {
                // GoScanner::scan(lines_to_scan);
            } 
            else if (filetype == "html" || filetype == "htm") {
                // HTMLScanner::scan(lines_to_scan);
            } 
            else if (filetype == "css") {
                // CSSScanner::scan(lines_to_scan);
            } 
            else if (filetype == "json") {
                // scanjson(lines_to_scan);
            } 
            else if (filetype == "xml") {
                // XMLScanner::scan(lines_to_scan);
            } 
            else {
                // No syntax highlighting for unknown file types
                // This block is equivalent to the 'default' case
            }
            

            {
                std::lock_guard<std::mutex> lock(line_mutex);
                this->filelines = lines_to_scan; // Update the main filelines
                compilation_pending = true; // Mark that recompilation is needed
                last_change_time = std::chrono::steady_clock::now();
                alt_file_path = file_path; // Update alternative file path
            }

            text_changed = false;
        } 
        if (compilation_pending && (!compiler_task.valid() || compiler_task.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_change_time).count();
            if (elapsed >= 500) {
                compilecplusplus(lines_to_scan);
                compilation_pending = false;
            }
        }

        // std::this_thread::sleep_for(std::chrono::milliseconds(50));

    }
}

void Soup::scancplusplus(lines& lines_to_scan) {
    static const std::unordered_set<std::string> control = {
        "break", "case", "continue", "default", "do", "else", "for", "goto", "if", 
        "return", "switch", "try", "catch", "throw", "while",
        "co_await", "co_return", "co_yield",
        "delete", "new", "this", "asm", "operator",
        "false", "nullptr", "true"
    };
    static const std::unordered_set<std::string> type_modifiers_tools = {
        "alignas", "alignof", "auto", "decltype", "extern", "inline", "mutable", 
        "register", "sizeof", "static", "thread_local", "typedef", "typename", "volatile",
        "explicit", "friend", "private", "protected", "public", "virtual",
        "const_cast", "dynamic_cast", "reinterpret_cast", "static_cast",
        "and", "and_eq", "bitand", "bitor", "compl", "not", "not_eq", "or", "or_eq", "xor", "xor_eq",
        "enum", "union"
    };
    static const std::unordered_set<std::string> standard_types_modern = {
        "bool", "char", "char8_t", "char16_t", "char32_t", "int", "wchar_t",
        "const", "constexpr", "consteval", "constinit", "noexcept", "static_assert",
        "export", "import", "module", "namespace", "using",
        "concept", "requires", "template",
        "reflexpr", "synchronized", "atomic_cancel", "atomic_commit", "atomic_noexcept"
    };
    bool multiline_comment = false;
    bool in_string = false;
    bool in_char = false;
    bool in_preprocessor = false;
    std::stack<std::string> circle_bracket_stack;
    std::stack<std::string> angle_bracket_stack;
    std::stack<std::string> square_bracket_stack;
    for (auto& line : lines_to_scan) {
        in_preprocessor = false; // Preprocessor directives do not span multiple lines
        bool single_line_comment = false;
        std::string current_word;
        for (int i = 0; i < line.size(); i++) {
            char c = line[i].character;
            if (multiline_comment) {
                line[i].color_type = wordtype::multilinecomment;
                if (c == '*' && i + 1 < line.size() && line[i + 1].character == '/') {
                    line[i + 1].color_type = wordtype::multilinecomment;
                    multiline_comment = false;
                    i++;
                }
            } else if (single_line_comment) {
                line[i].color_type = wordtype::comment;
            } else if (in_string) {
                line[i].color_type = wordtype::string;
                if (c == '\\' && i + 1 < line.size()) {
                    line[i + 1].color_type = wordtype::string; // Escape sequence
                    i++;
                } else if (c == '"') {
                    in_string = false;
                }
            } else if (in_char) {
                line[i].color_type = wordtype::charliteral;
                if (c == '\\' && i + 1 < line.size()) {
                    line[i + 1].color_type = wordtype::charliteral; // Escape sequence
                    i++;
                } else if (c == '\'') {
                    in_char = false;
                }
            } else if (in_preprocessor) {
                line[i].color_type = wordtype::preprocessor;
            } else {
                if (c == '/' && i + 1 < line.size() && line[i + 1].character == '/') {
                    line[i].color_type = wordtype::comment;
                    line[i + 1].color_type = wordtype::comment;
                    single_line_comment = true;
                    i++;
                } else if (c == '/' && i + 1 < line.size() && line[i + 1].character == '*') {
                    line[i].color_type = wordtype::multilinecomment;
                    line[i + 1].color_type = wordtype::multilinecomment;
                    multiline_comment = true;
                    i++;
                } else if (c == '"') {
                    line[i].color_type = wordtype::string;
                    in_string = true;
                } else if (c == '\'') {
                    line[i].color_type = wordtype::charliteral;
                    in_char = true;
                } else if (isspace(c) || (ispunct(c) && c != '_')) {
                    if (!current_word.empty()) {
                        // Classify the current word
                        if (control.count(current_word)) {
                            for (int j = i - current_word.size(); j < i; j++) {
                                line[j].color_type = wordtype::control;
                            }
                        } else if (type_modifiers_tools.count(current_word)) {
                            for (int j = i - current_word.size(); j < i; j++) {
                                line[j].color_type = wordtype::type;
                            }
                        } else if (standard_types_modern.count(current_word)) {
                            for (int j = i - current_word.size(); j < i; j++) {
                                line[j].color_type = wordtype::keyword;
                            }
                        }  else if (isdigit(current_word[0])) {
                            for (int j = i - current_word.size(); j < i; j++) {
                                line[j].color_type = wordtype::number;
                            }
                        } else {
                            // Check if it's a function (followed by '(')
                            if (i < line.size() && line[i].character == '(') {
                                for (int j = i - current_word.size(); j < i; j++) {
                                    line[j].color_type = wordtype::function;
                                }
                            } else {
                                for (int j = i - current_word.size(); j < i; j++) {
                                    line[j].color_type = wordtype::identifier;
                                }
                            }
                        }
                    }
                    current_word.clear();
                    if (c == '(') {
                        circle_bracket_stack.push("(");
                        line[i].color_type = wordtype::bracket;
                        line[i].number_of_brackets = circle_bracket_stack.size();
                    } else if (c == ')') {
                        line[i].number_of_brackets = circle_bracket_stack.size();
                        if (!circle_bracket_stack.empty()) circle_bracket_stack.pop();
                        line[i].color_type = wordtype::bracket;
                    } else if (c == '{') {
                        angle_bracket_stack.push("<");
                        line[i].color_type = wordtype::bracket;
                        line[i].number_of_brackets = angle_bracket_stack.size();
                    } else if (c == '}') {
                        line[i].number_of_brackets = angle_bracket_stack.size();
                        if (!angle_bracket_stack.empty()) angle_bracket_stack.pop();
                        line[i].color_type = wordtype::bracket;
                    } else if (c == '[') {
                        square_bracket_stack.push("[");
                        line[i].color_type = wordtype::bracket;
                        line[i].number_of_brackets = square_bracket_stack.size();
                    } else if (c == ']') {
                        line[i].number_of_brackets = square_bracket_stack.size();
                        if (!square_bracket_stack.empty()) square_bracket_stack.pop();
                        line[i].color_type = wordtype::bracket;
                    } 
                    else 
                    if (ispunct(c) && c != '_') {
                        line[i].color_type = wordtype::punctuation;
                        if (c == '#') {
                            in_preprocessor = true;
                        }
                    } else {
                        line[i].color_type = wordtype::plain;
                    }
                } else {
                    current_word += c;
                    line[i].color_type = wordtype::plain; // Default, may change later
                }
            }
        }
    }
}

std::future<void> Soup::compiler_task;

void Soup::compilecplusplus(lines &lines_to_compile) {
    if (project_options.default_compiler.empty() && lines_to_compile.size() <= 0) {
        return;
    }

    if(compiler_task.valid() && compiler_task.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        errors.clear();
        num++;
        errors.push_back({0, num, "Error: Failed to execute this issue."});
        return;
    }

    compiler_task = std::async(std::launch::async, [=]() {
        try {
        saveFile();
        
        std::string command = project_options.default_compiler + " -fsyntax-only \"" + alt_file_path + "\"";
        std::string output = exec_and_capture_hidden(command);

        std::vector<error_data> new_errors;
        std::istringstream iss(output);
        std::string line;

        while (std::getline(iss, line)) {
            size_t colon1 = line.find(':');
            if (colon1 != std::string::npos) {
                size_t colon2 = line.find(':', colon1 + 1);
                size_t colon3 = line.find(':', colon2 + 1);
                if (colon2 != std::string::npos && colon3 != std::string::npos) {
                    try {
                        int err_line = std::stoi(line.substr(colon1 + 1, colon2 - colon1 - 1)) - 1; // Zero-based
                        int err_column = std::stoi(line.substr(colon2 + 1, colon3 - colon2 - 1)) - 1; // Zero-based
                        std::string err_message = line.substr(colon3 + 2); // Skip ": "
                        new_errors.push_back({err_line, err_column, err_message});
                    } catch (const std::invalid_argument&) {
                        // Ignore lines that don't conform to expected format
                    } catch (const std::out_of_range&) {
                        // Ignore lines that don't conform to expected format
                    }
                }
            }
        }

        std::sort(new_errors.begin(), new_errors.end(), [](const error_data& a, const error_data& b) {
            if (a.line != b.line) {
                return a.line < b.line;
            }
            return a.column < b.column;
        });

        {
            std::lock_guard<std::mutex> lock(line_mutex);
            errors = new_errors;
        }
        
    } catch (...) {
        // Handle cases where the process could not be launched at all
        // For example, compiler_path is invalid
        std::lock_guard<std::mutex> lock(line_mutex);
        errors.clear();
        std::string message = "Error: Could not execute compiler: " + project_options.default_compiler;
        errors.push_back({0, 0, message});
    }
    });

}
