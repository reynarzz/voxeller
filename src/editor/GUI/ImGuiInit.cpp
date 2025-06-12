#include "ImGuiInit.h"
#include <glfw/glfw3.h>
#include "imgui.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

namespace VoxellerEditor
{
    void ImGuiApp::Init(void* internalWindow)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        
        // Init ImGui GLFW+OpenGL3 backend
        ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(internalWindow), true);
        ImGui_ImplOpenGL3_Init("#version 150");  // or "#version 330 core", depending on your OpenGL

    }

    void ImGuiApp::Update()
    {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

           
            ImGui::ShowDemoWindow();
        
            // Render
            ImGui::Render();
            /*int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);*/
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    ImGuiApp::~ImGuiApp()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}




