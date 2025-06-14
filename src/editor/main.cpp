//
#define VOXELLER_LIB

#include <voxeller/voxeller.h>
#include <gl/glad.h>
#include <GLFW/glfw3.h>
#include <voxeller/Log/Log.h>
#include <GUI/ImGuiInit.h>

#include <voxeller/GreedyMesher.h>
#include <iostream>


using namespace VoxellerEditor;

ImGuiApp imgui{};

void Render(GLFWwindow* window)
{
	glfwPollEvents();
	f32 color = 0.05f;

	glClearColor(color, color, color, 1.0);
	//glClearColor(77.0f / 255.0f, 83.0f / 255.0f, 90.0f / 255.0f, 1.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	imgui.Update();
	glfwSwapBuffers(window);
}

void drop_callback(GLFWwindow* window, int count, const char** paths) {
	for (int i = 0; i < count; ++i) {
		std::cout << "Dropped file: " << paths[i] << std::endl;
		// Save paths[i] somewhere to use in ImGui
	}
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
	glfwSetDropCallback(win, drop_callback);

	int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	imgui.Init(win);

	Voxeller::ExportOptions exportOptions{};
	exportOptions.OutputFormat = Voxeller::ModelFormat::FBX;
	exportOptions.ConvertOptions.NoTJunctions = false;
	exportOptions.ConvertOptions.WeldVertices = true;
	exportOptions.ConvertOptions.FlatShading = true;
	//exportOptions.ConvertOptions.Scale = .01f;
	exportOptions.ConvertOptions.ExportFramesSeparatelly = true;
	exportOptions.ConvertOptions.ExportMeshesSeparatelly = true;
	exportOptions.ConvertOptions.SeparateTexturesPerMesh = false;

	//std::string path = "B:/Projects/voxeller/bin/Debug/testvox/nda/Ambulance_1.vox"; // Test this!
	std::string path = "B:/Projects/voxeller/bin/Debug/testvox/room.vox"; 
	//std::string path = "B:/Projects/voxeller/bin/Debug/testvox/nda/Island_7.vox";
	std::string output = "B:/Projects/voxeller/bin/Debug/testvox/nda/export/Output.fbx";
	Voxeller::GreedyMesher::ExportVoxToModel(path, output, exportOptions);

	while (!glfwWindowShouldClose(win))
	{
		Render(win);
	}

	glfwTerminate();
	return -1;
}
