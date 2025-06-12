
#define VOXELLER_LIB

#include <iostream>
#include <filesystem>
#include <voxeller/voxeller.h>
#include <gl/glad.h>
#include <GLFW/glfw3.h>
#include <voxeller/Log/Log.h>
#include <GUI/ImGuiInit.h>

using namespace VoxellerEditor;

int main()
{
   if (glfwInit() == GLFW_TRUE)
   {
      std::cout << "glfw initialization successfull!\n";
   }
   else
   {
      std::cout << "glfw init error\n";
   }
   
   //vulkan later.
   //--glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
   
   glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
   
   // TODO: move to another function.
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   
   // set: 'glfwGetPrimaryMonitor()' to make the window full screen
   GLFWwindow* win = glfwCreateWindow(1000, 600, "Voxeller", nullptr, nullptr);

   // openGL 
   glfwMakeContextCurrent(win);
   int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
   
    
    VoxellerInit();
    
    ImGuiApp imgui{};
    imgui.Init(win);
    
    
   //std::shared_ptr<vox_file> file = vox_parser::read_vox_file("testvox/chr_knight.vox");
   std::shared_ptr<vox_file> file = vox_parser::read_vox_file("testvox/purple_mushroom_1.vox");
    
   LOG_EDITOR_INFO("This is the editor");
    
   if (file != nullptr && file->isValid)
   {
       std::cout << "header: " << file->header.id << ", name: " << file->name << ", version: " << file->header.version << '\n';

       mesh_texturizer::export_pallete_png("pallete.png", file->pallete);
       vox_mesh_builder::build_mesh_voxel(file);
   }
   else 
   {
       std::cout << "invalid vox, can't continue.\n";
   }


   
   while(!glfwWindowShouldClose(win))
   {
      glfwPollEvents();
      glClearColor(1.2, 0.2, 0.2, 1.0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      imgui.Update();
      glfwSwapBuffers(win);

   }

   glfwTerminate();
   return -1;
}
