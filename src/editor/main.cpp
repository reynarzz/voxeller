#include <Unvoxeller/Unvoxeller.h>
#include <gl/glad.h>
#include <GLFW/glfw3.h>
#include <GUI/ImGuiInit.h>

#include <Unvoxeller/GreedyMesher.h>
#include <Unvoxeller/Math/VoxMath.h>
#include <Unvoxeller/Log/Log.h>
#include <GUI/Utils/DropHoverEvents.h>

#include <Unvoxeller/File.h>
#include <iostream>
#include <imgui/imgui.h>

// TODO:
// - Fix small offset happening.

using namespace VoxellerEditor;

std::unique_ptr<ImGuiApp> imgui = nullptr;

void Render(GLFWwindow* window)
{
	//Cursor::SetMode(CursorMode::NoDrop);

	glfwPollEvents();
	f32 color = 0.05f;

	glClearColor(color, color, color, 1.0);
	//glClearColor(77.0f / 255.0f, 83.0f / 255.0f, 90.0f / 255.0f, 1.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	imgui->Update();

	glfwSwapBuffers(window);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{

	// tell OpenGL about it:

	Render(window);
}


int Init()
{
	VoxellerInit();

	if (glfwInit() == GLFW_TRUE)
	{
		LOG_INFO("Success GLFW initialization");
	}
	else
	{
		LOG_ERROR("glfw init error");
		return -1;
	}

	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	// TODO: move to another function.
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// set: 'glfwGetPrimaryMonitor()' to make the window full screen
	GLFWwindow* win = glfwCreateWindow(1000, 600, "Unvoxeller", nullptr, nullptr);

	if (!win)
	{
		LOG_ERROR("Error: GLFW window creation error");
		glfwTerminate();
		return -1;
	}
	else
	{
		LOG_INFO("Success: GLFW window creation");
	}
	// openGL 
	glfwMakeContextCurrent(win);

	const s32 status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	if (status == 0)
	{
		LOG_ERROR("Error: Glad initialization");
		return -1;
	}
	else
	{
		LOG_INFO("Success: Glad initialization");
	}

	glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
	imgui = std::make_unique<ImGuiApp>();

	DropAndDrop::Initialize(win);

	DropAndDrop::SetDropCallback([](DropEvent info)
		{
			LOG_INFO("Dropped position ({0}, {1}): ", info.x, info.y);

			for (size_t i = 0; i < info.paths.size(); i++)
			{
				LOG_INFO("Droped: " + info.paths[i]);
			}
		});

	DropAndDrop::SetHoverCallback([](HoverEvent info)
		{

			LOG_INFO("Hover position ({0}, {1}): ", info.x, info.y);
		});

	LOG_INFO("Dir: {0}", Unvoxeller::File::GetExecutableDir());

	//Unvoxer
	imgui->Init(win);

	Unvoxeller::ExportOptions exportOptions{};
	exportOptions.OutputFormat = Unvoxeller::ModelFormat::FBX;
	exportOptions.ConvertOptions.RemoveTJunctions = false;
	exportOptions.ConvertOptions.WeldVertices = false;
	exportOptions.ConvertOptions.FlatShading = true;
	exportOptions.ConvertOptions.SeparateTexturesPerMesh = false;
	exportOptions.ConvertOptions.MaterialPerMesh = true;
	exportOptions.ConvertOptions.Scale = { 1.3f, 1.3f, 1.3f };
	//exportOptions.ConvertOptions.Pivot = { 0.5f, 0.0f, 0.5f };
	exportOptions.ConvertOptions.ExportFramesSeparatelly = true;
	exportOptions.ConvertOptions.ExportMeshesSeparatelly = false;

	exportOptions.ConvertOptions.GenerateMaterials = true;
	exportOptions.ConvertOptions.MeshesToWorldCenter = false;
	exportOptions.ConvertOptions.TexturesPOT = false;


	// TODO:
	exportOptions.ConvertOptions.RemoveOccludedFaces = false;
	exportOptions.ConvertOptions.OptimizeTextures = false;
	exportOptions.ConvertOptions.TextureType = {};

	Unvoxeller::vox_quat s{};
	Unvoxeller::vox_vec3 as;
	Unvoxeller::vox_vec3 a2s = -as;

	//Chicken_van_2.vox
	std::string path = Unvoxeller::File::GetExecutableDir() + "/testvox/nda/Ambulance_1.vox"; // Test this!
	//std::string path = Unvoxeller::File::GetExecutableDir() + "/testvox/monu2.vox"; // Test this!
	//std::string path = Unvoxeller::File::GetExecutableDir() + "/testvox/room.vox";
	//std::string path = "testvox/nda/Ambulance_1.vox";
	//std::string output = "testvox/nda/export/Output.fbx";
	std::string output = Unvoxeller::File::GetExecutableDir() + "/testvox/nda/export/Output.fbx";

	Unvoxeller::GreedyMesher::ExportVoxToModel(path, output, exportOptions);

	while (!glfwWindowShouldClose(win))
	{
		Render(win);
	}

	glfwTerminate();
}


#ifdef _WIN32
		#include <windows.h>

		int WINAPI WinMain(
			HINSTANCE hInstance,      // handle to current instance
			HINSTANCE hPrevInstance,  // always NULL in modern Windows
			LPSTR     lpCmdLine,      // command-line as a single string
			int       nCmdShow        // how the window should be shown
		)
		{
			return Init();
		}

		int main()
		{
			return Init();
		}
#else
	int main()
	{
		return Init();
	}
#endif


