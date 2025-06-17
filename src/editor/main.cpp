//
#define VOXELLER_LIB

#include <Voxeller/Voxeller.h>
#include <gl/glad.h>
#include <GLFW/glfw3.h>
#include <GUI/ImGuiInit.h>

#include <Voxeller/GreedyMesher.h>
#include <voxeller/Math/VoxMath.h>
#include <Voxeller/Log/Log.h>
#include <GUI/Utils/DropHoverEvents.h>

#include <Voxeller/File.h>

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
		LOG_EDITOR_INFO("Success GLFW initialization");
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

	const s32 status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	if (status == 0)
	{
		LOG_EDITOR_ERROR("Error: Glad initialization");
	}
	else
	{
		LOG_CORE_INFO("Success: Glad initialization");
	}

	DropHoverEvents::Initialize(win);

	DropHoverEvents::SetDropCallback([](DropEvent info)
		{
			LOG_EDITOR_INFO("Dropped position ({0}, {1}): ", info.x, info.y);

			for (size_t i = 0; i < info.paths.size(); i++)
			{
				LOG_EDITOR_INFO("Droped: " + info.paths[i]);
			}
		});

	DropHoverEvents::SetHoverCallback([](HoverEvent info)
		{
			LOG_EDITOR_INFO("Hover position ({0}, {1}): ", info.x, info.y);
		});

	LOG_CORE_INFO("Dir: {0}", Voxeller::File::GetExecutableDir());


	//Unvoxer
	imgui.Init(win);

	Voxeller::ExportOptions exportOptions{};
	exportOptions.OutputFormat = Voxeller::ModelFormat::FBX;
	exportOptions.ConvertOptions.RemoveTJunctions = false;
	exportOptions.ConvertOptions.WeldVertices = false;
	exportOptions.ConvertOptions.FlatShading = true;
	exportOptions.ConvertOptions.SeparateTexturesPerMesh = false;
	exportOptions.ConvertOptions.MaterialPerMesh = true;
	exportOptions.ConvertOptions.Scale = { 12.3f, 12.3f, 12.3f };
	//exportOptions.ConvertOptions.Pivot = { 0.5f, 0.0f, 0.5f };
	exportOptions.ConvertOptions.ExportFramesSeparatelly = true;
	exportOptions.ConvertOptions.ExportMeshesSeparatelly = false;

	exportOptions.ConvertOptions.GenerateMaterials = true;
	exportOptions.ConvertOptions.MeshesToWorldCenter = false;
	exportOptions.ConvertOptions.TexturesPOT = false;

	Voxeller::vox_quat s{};
	Voxeller::vox_vec3 as;
	Voxeller::vox_vec3 a2s = -as;

	//Chicken_van_2.vox
	std::string path = Voxeller::File::GetExecutableDir() + "/testvox/nda/Ambulance_1.vox"; // Test this!
	//std::string path = "B:/Projects/Voxeller/bin/Debug/testvox/room.vox"; 
	//std::string path = "testvox/nda/Ambulance_1.vox";
	//std::string output = "testvox/nda/export/Output.fbx";
	std::string output = Voxeller::File::GetExecutableDir() + "/testvox/nda/export/Output.fbx";

	//Voxeller::GreedyMesher::ExportVoxToModel(path, output, exportOptions);

	while (!glfwWindowShouldClose(win))
	{
		Render(win);
	}

	glfwTerminate();
	return -1;
}
