#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cassert>
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <unordered_set>
#include <future>
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
            control,
            type,
            number,
            identifier,
            function,
            string,
            charliteral,
            punctuation,
            preprocessor,
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
            max,
            preprocidentifier,
            bracket
        };

        struct glyph {
            uint8_t character;
            wordtype color_type = wordtype::plain;
            bool bracket = false;
            int number_of_brackets = 0; // for bracket colorization
            glyph(uint8_t c, wordtype ct) : character(c), color_type(ct) {}
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

            bool is_valid() const { return line >= 0 && column >= 0; }
            bool operator==(const editor& other) const { return line == other.line && column == other.column; }
            bool operator!=(const editor& other) const { return !(*this == other); }
        };

        typedef std::vector<glyph> line;
        typedef std::vector<line> lines;

        bool render();
        

        
    private:
        lines filelines;
        lines buffered_lines; // For undo/redo functionality in the future
        std::vector<lines> undo_stack;
        std::vector<lines> redo_stack;
        int undo_stack_size = 50; // Maximum number of undo states to keep
        float line_spacing = 1.0f;
        float char_height = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
        ImVec2 char_font = ImVec2(char_height, ImGui::GetTextLineHeightWithSpacing() * line_spacing);
        int tab_size = 4;
        std::string file_name;
        std::string file_path;
        int scroll_x = 0;
        int scroll_y = 0;

        //Error handling
        struct error_data {
            int line;
            int column;
            std::string message;
        };
        std::vector<error_data> errors;
        static std::future<void> compiler_task;
        std::atomic<bool> compilation_pending = false;
        std::chrono::steady_clock::time_point last_change_time;
        int num = 0;
        std::string alt_file_path;

        editor selection_start = editor::invalid();
        editor selection_end = editor::invalid();
        bool selecting = false;

        editor edit;  // stores cursor position and focus state

        void DrawEditor(ImVec2 size, ImVec2 canvas_top_left, float line_heinght, int visible_lines, int visible_chars);
        editor GetCoordinatesFromMousePos(ImVec2 canvas_top_left, float line_height);
        void HandleKeyboardInput();
        void HandleMouseInput(ImVec2 canvas_top_left, float line_height);
        void ScrollToCursor(int visible_lines, int visible_chars);
        void HandleCtrlShortcuts();
        void DeleteSelection();

        float GetColumnPixelOffset(const line& current_line, int target_column, ImFont* font, float font_size, int tab_size);
        bool saveFile();
        ImU32 GetColor(wordtype type);
        ImU32 GetColor(wordtype type, int variant); // for bracket colorization

        std::thread scan_thread;
        std::mutex line_mutex;
        std::atomic<bool> text_changed;
        std::atomic<bool> scanner_shouldrun;

        // scanner functions for different languages
        void scancplusplus(lines& lines_to_scan);
        void compilecplusplus(lines &lines_to_scan);
        // void scanpython(lines& lines_to_scan);
        // void scanjson(lines& lines_to_scan);

        void scanner();
};