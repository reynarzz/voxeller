
#define VOXELLER_LIB

#include <voxeller/voxeller.h>
#include <gl/glad.h>
#include <GLFW/glfw3.h>
#include <voxeller/Log/Log.h>
#include <GUI/ImGuiInit.h>

#include <voxeller/GreedyMesher.h>

using namespace VoxellerEditor;

   ImGuiApp imgui{};

void Render(GLFWwindow* window)
{
glfwPollEvents();
      glClearColor(1.0, 0.2, 0.2, 1.0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      imgui.Update();
      glfwSwapBuffers(window);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    
    // tell OpenGL about it:

     Render(window);
}


int main()
{
   VoxellerInit();

   if (glfwInit() == GLFW_TRUE)
   {
      LOG_EDITOR_INFO("glfw initialization successfull!");
   }
   else
   {
      LOG_EDITOR_ERROR("glfw init error");
   }
   
   glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
   
   // TODO: move to another function.
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   
   // set: 'glfwGetPrimaryMonitor()' to make the window full screen
   GLFWwindow* win = glfwCreateWindow(1000, 600, "Voxeller", nullptr, nullptr);

   // openGL 
   glfwMakeContextCurrent(win);
   glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);

   int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
   
   imgui.Init(win);
    
   Voxeller::ExportOptions exportOptions{};
   exportOptions.OutputFormat = Voxeller::ModelFormat::OBJ;
   exportOptions.ConvertOptions.NoTJunctions = true;
   exportOptions.ConvertOptions.WeldVertices = true;
   exportOptions.ConvertOptions.FlatShading = true;
   exportOptions.ConvertOptions.Scale = .01f;
   
   //Voxeller::GreedyMesher::ExportVoxToModel("testvox/room.vox", "Output.fbx", exportOptions);

   while(!glfwWindowShouldClose(win))
   {
      Render(win);
   }

   glfwTerminate();
   return -1;
}
