#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cassert>
#include <filesystem>
#include "imgui.h"
#include "utils.hpp"
// #include "connector.hpp"

namespace fs = std::filesystem;

class Soup {
    public:
        Soup(fs::path file);
        ~Soup();

        enum class wordtype {
            plain,
            keyword,
            number,
            string,
            charliteral,
            punctuation,
            preprocessor,
            identifier,
            knownidentifier,
            preprocidentifier,
            comment,
            multilinecomment,
            background,
            cursor,
            selection,
            errormarker,
            breakpoint,
            linenumber,
            currentlinefill,
            currentlinefillinactive,
            currentlineedge,
            max
        };

        struct glyph {
            uint8_t character;
            wordtype color_type = wordtype::plain;
            bool comment : 1;
            bool multiline_comment : 1;
            bool preprocessor : 1;
            glyph(uint8_t c, wordtype ct) : character(c), color_type(ct),
                comment(false), multiline_comment(false), preprocessor(false) {}
        };

        struct editor{
            int line;
            int column;
            bool focused = false;

            editor() : line(0), column(0), focused(false) {}
            editor(int l, int c) : line(l), column(c), focused(false) {
                assert(l >= 0);
                assert(c >= 0);
            }

            static editor invalid() { 
                editor e;
                e.line = -1;
                e.column = -1;
                return e;
             }
        };

        typedef std::vector<glyph> line;
        typedef std::vector<line> lines;

        bool render();
        

        
    private:

        lines filelines;
        lines buffered_lines; // For undo/redo functionality in the future
        lines clipboard; 
        int undo_stack_size = 50; // Maximum number of undo states to keep
        bool clipboard_is_cut = false;
        bool clipboard_has_content = false;
        float line_spacing = 1.0f;
        float char_height = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
        ImVec2 char_font = ImVec2(char_height, ImGui::GetTextLineHeightWithSpacing() * line_spacing);
        int tab_size = 4;
        std::string file_name;
        std::string file_path;
        int scroll_x = 0;
        int scroll_y = 0;

        editor selection_start = editor::invalid();
        editor selection_end = editor::invalid();
        bool selecting = false;

        editor edit;  // stores cursor position and focus state

        float GetColumnPixelOffset(const line& current_line, int target_column, ImFont* font, float font_size, int tab_size);
        bool saveFile();
        ImU32 GetColor(wordtype type);
};