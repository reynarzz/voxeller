#include "ImGuiInit.h"
#include <glfw/glfw3.h>
#include "imgui.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GUI/Views/VoxToProcessView.h>


namespace VoxellerEditor
{
    VoxToProcessView view{};

    void LoadFont()
    {
               ImGuiIO& io = ImGui::GetIO(); 
            io.Fonts->Clear(); // if you want only your custom font

    // 3) Add your font(s)
    //   - path: the file path to your .ttf
    //   - size: font size in pixels
    //   - glyphRanges: optional to limit which glyphs get baked
    ImFont* myFont = io.Fonts->AddFontFromFileTTF(
        "assets/fonts/ProductSans-Medium.ttf",
        16.0f,               // size in pixels
        nullptr,             // ImFontConfig*, or nullptr
        io.Fonts->GetGlyphRangesDefault()
    );
    IM_ASSERT(myFont != nullptr);  // make sure it loaded

    // 4) Build atlas texture
    // This will automatically be done by the backend on the first NewFrame()
    // or you can call:
    //unsigned char* pixels; int width, height;
    //io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    // then upload the pixels to a GPU texture and set io.Fonts->TexID = yourTextureID

    // 5) (Optionally) tell your renderer backend
    // e.g. for OpenGL:
    //ImGui_ImplOpenGL3_DestroyFontsTexture();
    //ImGui_ImplOpenGL3_CreateFontsTexture();
    io.FontDefault = myFont;
    }
    void ImGuiApp::Init(void* internalWindow)
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); 
        (void)io;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        
        // Init ImGui GLFW+OpenGL3 backend
        ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(internalWindow), true);
        ImGui_ImplOpenGL3_Init("#version 150");  // or "#version 330 core", depending on your OpenGL

        LoadFont();
    }

    void ImGuiApp::Update()
    {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

           view.UpdateGUI();
        
            // Render
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    ImGuiApp::~ImGuiApp()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}




