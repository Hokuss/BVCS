#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "IconsFontAwesome6.h"
#include "mainspace.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include "font.h"

void LoadFonts(ImGuiIO& io) {
    // Setup config as you did before
    static ImFontConfig config;
    config.MergeMode = true;
    config.PixelSnapH = true;
    
    // Setup ranges as you did before
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

    // --- OLD METHOD ---
    // io.Fonts->AddFontFromFileTTF("Font Awesome 7 Free-Solid-900.otf", 16.0f, &config, icons_ranges);

    // --- NEW STATIC METHOD ---
    // Use the array name and length variable from the generated header file
    io.Fonts->AddFontFromMemoryTTF(
        (void*)Font_Awesome_7_Free_Solid_900_otf,      // The array from the header
        sizeof(Font_Awesome_7_Free_Solid_900_otf),  // The size from the header
        16.0f,                                  // Size in pixels
        &config,                                // Config
        icons_ranges                            // Ranges
    );
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create window
    GLFWwindow* window = glfwCreateWindow(800, 600, "BVCS Innit", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); 
    io.Fonts->AddFontDefault();

    ImFontConfig config;
    config.MergeMode = true;
    config.PixelSnapH = true;

    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };

    // io.Fonts->AddFontFromFileTTF("Font Awesome 7 Free-Solid-900.otf", 16.0f, &config, icons_ranges);
    LoadFonts(io);

    // io.Fonts->Build();
    // (void)io;

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    bool opened = true;
    int trip = 0;
    float slider = 0.0f;
    // Main loop
    while (!glfwWindowShouldClose(window) && opened) {

        glfwWaitEvents();
        // glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(1200, 900), ImGuiCond_FirstUseEver);
        // Create a simple window
        ImGui::Begin("BVCS V-0.1", &opened, ImGuiWindowFlags_NoCollapse);

        main_menu();

        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}