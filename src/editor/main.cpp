#include <Unvoxeller/Unvoxeller.h>
#include <gl/glad.h>
#include <GLFW/glfw3.h>
#include <GUI/ImGuiInit.h>

#include <Unvoxeller/GreedyMesher.h>
#include <Unvoxeller/Math/VoxMath.h>
#include <Unvoxeller/Log/Log.h>
#include <GUI/Utils/DropHoverEvents.h>
#include <Rendering/Camera.h>
#include <Unvoxeller/File.h>
#include <iostream>

#include <Rendering/RenderingSystem.h>

// TODO:
// - Fix small offset happening.
// - Reuse colors in texture (check which faces have the same colors, and set an id, to ref an island).
// - Pallete textures (decide the max width/height. Note not override POT)
// - Localization: English, Spanish, French, Chinese, Japanese, German.
// - Finish shader creation.
// - Implement frame buffer for OpenGL
// - Fix app UX

std::unique_ptr<ImGuiApp> _imgui = nullptr;
std::unique_ptr<RenderingSystem> _renderingSystem = nullptr;
std::unique_ptr<Camera> _camera = nullptr;

void Render(GLFWwindow* window)
{
	glfwPollEvents();

	_camera->Update();
	
	_renderingSystem->Update(_camera->GetState());

	_imgui->Update();

	glfwSwapBuffers(window);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
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

	_renderingSystem = std::make_unique<RenderingSystem>();
	_renderingSystem->Initialize();
	
	
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
	_camera = std::make_unique<Camera>();
	_imgui = std::make_unique<ImGuiApp>();
 
	_imgui->Init(win);

	Unvoxeller::ExportOptions exportOptions{};
	exportOptions.OutputFormat = Unvoxeller::ModelFormat::FBX;
	exportOptions.Converting.Meshing.RemoveTJunctions = false;
	exportOptions.Converting.Meshing.WeldVertices = false;
	exportOptions.Converting.Meshing.FlatShading = false;
	exportOptions.Converting.Meshing.MaterialPerMesh = true;
	exportOptions.Converting.Scale = { 1.3f, 1.3f, 1.3f };
	exportOptions.Converting.Pivots = { { 0.5f, 0.0f, 0.5f } };
	exportOptions.Converting.ExportFramesSeparatelly = true;
	exportOptions.Converting.ExportMeshesSeparatelly = false;

	exportOptions.Converting.Meshing.GenerateMaterials = true;
	exportOptions.Converting.Meshing.MeshesToWorldCenter = false;

	// Texturing:
	exportOptions.Converting.Texturing.SeparateTexturesPerMesh = false;
	exportOptions.Converting.Texturing.TexturesPOT = false;
	exportOptions.Converting.Texturing.OptimizeTextures = false;
	exportOptions.Converting.Texturing.TextureType = {};

	// TODO:
	exportOptions.Converting.Meshing.RemoveOccludedFaces = false;

	// V2
	exportOptions.Converting.Lods = { 0.8f, 0.4f };


	Unvoxeller::vox_quat s{};
	Unvoxeller::vox_vec3 as;
	Unvoxeller::vox_vec3 a2s = -as;

	//Chicken_van_2.vox
	//std::string path = Unvoxeller::File::GetExecutableDir() + "/testvox/nda/Ambulance_1.vox"; // Test this!
	std::string path = Unvoxeller::File::GetExecutableDir() + "/testvox/monu2.vox"; // Test this!
	//std::string path = Unvoxeller::File::GetExecutableDir() + "/testvox/room.vox";
	//std::string output = "testvox/nda/export/Output.fbx";
	std::string output = Unvoxeller::File::GetExecutableDir() + "/testvox/nda/export/Output.fbx";
	
	
	Unvoxeller::GreedyMesher::ExportVoxToModel(path, output, exportOptions);
	
	std::string texPath = Unvoxeller::File::GetExecutableDir() + "/testvox/nda/export/Output_atlas.png";

	auto renderable = std::make_shared<RenderableObject>();

	renderable->GetTransform().SetPosition({});
		
	//RenderingSystem::PushRenderable(renderable.get());

	
	while (!glfwWindowShouldClose(win))
	{
		if (glfwGetWindowAttrib(win, GLFW_ICONIFIED))
		{
			// Skip rendering or pause updates
		}
		else
		{
			Render(win);
		}
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


