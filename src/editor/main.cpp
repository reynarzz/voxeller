#include <Unvoxeller/Unvoxeller.h>
#include <gl/glad.h>
#include <GLFW/glfw3.h>
#include <GUI/ImGuiInit.h>

#include <Unvoxeller/Log/Log.h>
#include <GUI/Utils/DropHoverEvents.h>
#include <Rendering/Camera.h>
#include <Unvoxeller/File.h>
#include <iostream>

#include <Rendering/RenderingSystem.h>
#include <GUI/Utils/TextureLoader.h>
#include <Time/Time.h>

// TODO:
// - UV texture viewer.
// - Fix app UX: Feature set
// - Create config export menu, and individual
// - Modal progress UI.
// - Fix small offset happening.
// - Reuse colors in texture (check which faces have the same colors, and set an id, to ref an island).
// - Pallete textures (decide the max width/height. Note not override POT)
// - Localization: English, Spanish, French, Chinese, Japanese, German.
// - Finish shader creation.
// - Implement frame buffer for OpenGL [Done]
// - Multithreading

std::unique_ptr<ImGuiApp> _imgui = nullptr;
std::unique_ptr<RenderingSystem> _renderingSystem = nullptr;
std::unique_ptr<Camera> _camera = nullptr;
std::vector<std::shared_ptr<RenderableObject>> _renderables = {};


void Render(GLFWwindow* window)
{
	glfwPollEvents();

	Time::Update();

	LightState lightStateTest{};
	lightStateTest.lightIntensity = 1.5f;

	RendererState state{};
	state.ViewState = &_camera->GetState();
	state.LightState = &lightStateTest;
	
	_renderingSystem->Update(state);

	_imgui->Update();

	glfwSwapBuffers(window);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	Render(window);
}

static std::vector<std::shared_ptr<RenderableObject>> CreateFromGeometry(const std::vector<std::shared_ptr<Unvoxeller::UnvoxScene>>& scenes)
{
	std::vector<std::shared_ptr<RenderableObject>> renderables{};
	for (const auto& scene : scenes)
	{
		s32 meshesIdx = {};
		for (const auto& mesh : scene->Meshes)
		{
			std::unique_ptr<MeshDescriptor> mDesc = std::make_unique<MeshDescriptor>();
			mDesc->RenderType = MeshRenderType::TRIANGLES;
	
			auto renderable = std::make_shared<RenderableObject>();

			mDesc->Vertices.resize(mesh->Vertices.size());

			for (size_t i = 0; i < mesh->Vertices.size(); i++)
			{
				const auto& vert = Unvoxeller::vox_vec4(mesh->Vertices[i]);
				const auto& normal = mesh->Normals[i];
				const auto& uv = mesh->UVs[i];

				mDesc->Vertices[i] = { {vert.x, vert.y, vert.z}, {normal.x, normal.y, normal.z}, {uv.x, uv.y }};
			}
			
			meshesIdx++;

			mDesc->Indices.resize(mesh->Faces.size() * 3);
			s32 idx = {};
			for (size_t i = 0; i < mesh->Faces.size(); i++)
			{
				const auto& face = mesh->Faces[i];

				for (size_t j = 0; j < face.Indices.size(); j++)
				{
					mDesc->Indices[idx] = face.Indices[j];
					idx++;
				}
			}

			auto tex = scene->Textures[scene->Materials[mesh->MaterialIndex]->TextureIndex];
			TextureDescriptor tDesc{};
			tDesc.width = tex->Width;
			tDesc.height= tex->Height;
			tDesc.image = tex->Buffer.data();
			renderable->SetTexture(Texture::Create(&tDesc));

			renderable->SetMesh(Mesh::CreateMesh(mDesc.get()));
			renderable->SetRenderType(PipelineRenderType::NoTexture);
			renderable->SetDrawType(RenderDrawType::Lines);

			renderables.push_back(renderable);
		}
	}

	return renderables;
}

int Init()
{
	VoxellerApp::init();

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
	GLFWwindow* win = glfwCreateWindow(1100, 600, "Unvoxeller", nullptr, nullptr);

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
	
	
	DragAndDrop::Initialize(win);

	DragAndDrop::SetDropCallback([](DropEvent info)
		{
			LOG_INFO("Dropped position ({0}, {1}): ", info.x, info.y);

			for (size_t i = 0; i < info.paths.size(); i++)
			{
				LOG_INFO("Droped: " + info.paths[i]);
			}
		});

	DragAndDrop::SetHoverCallback([](HoverEvent info)
		{

			LOG_INFO("Hover position ({0}, {1}): ", info.x, info.y);
		});

	LOG_INFO("Dir: {0}", Unvoxeller::File::GetExecutableDir());

	//Unvoxer
	_camera = std::make_unique<Camera>();
	_imgui = std::make_unique<ImGuiApp>();
	
	_imgui->Init(win);
	
	Unvoxeller::ExportOptions exportOptions{};
	Unvoxeller::ConvertOptions convertOptions{};
	convertOptions.Meshing.RemoveTJunctions = false;
	convertOptions.Meshing.WeldVertices = false;
	convertOptions.Meshing.FlatShading = false;
	convertOptions.Meshing.MaterialPerMesh = true;
	convertOptions.Meshing.MeshType = MeshType::Greedy;
	convertOptions.Scale = { 1.3f, 1.3f, 1.3f };
	convertOptions.Pivots = { { 0.5f, 0.0f, 0.5f } };
	convertOptions.ExportFramesSeparatelly = true;
	convertOptions.ExportMeshesSeparatelly = false;

	convertOptions.Meshing.GenerateMaterials = true;
	convertOptions.Meshing.MeshesToWorldCenter = false;

	// Texturing:
	convertOptions.Texturing.SeparateTexturesPerMesh = false;
	convertOptions.Texturing.TexturesPOT = false;
	convertOptions.Texturing.OptimizeTextures = false;
	convertOptions.Texturing.TextureType = {};

	// TODO:
	convertOptions.Meshing.RemoveOccludedFaces = false;

	//Chicken_van_2.vox
	std::string path = Unvoxeller::File::GetExecutableDir() + "/testvox/nda/Ambulance_1.vox"; // Test this!
	//std::string path = Unvoxeller::File::GetExecutableDir() + "/testvox/nda/Bus_Green.vox"; // Test this!
	//std::string path = Unvoxeller::File::GetExecutableDir() + "/testvox/nda/Chicken_van_3.vox"; // Test this!

	
	//std::string path = Unvoxeller::File::GetExecutableDir() + "/testvox/chr_knight.vox"; // Test this!
	//std::string path = Unvoxeller::File::GetExecutableDir() + "/testvox/room.vox";
	//std::string output = "testvox/nda/export/Output.fbx";
	
	// V2
	exportOptions.OutputDir = Unvoxeller::File::GetExecutableDir() + "/testvox/nda/export";
	exportOptions.OutputName = "Output";
	exportOptions.InputPath = path;
	exportOptions.OutputFormat = Unvoxeller::ModelFormat::OBJ;
	
	Unvoxeller::Unvoxeller unvox{};
	//unvox.ExportVoxToModel(exportOptions, convertOptions);
	auto scene = unvox.VoxToMem(path, convertOptions);
	
	_renderables = CreateFromGeometry(scene.Scenes);
	for (auto renderables : _renderables)
	{
		RenderingSystem::PushRenderable(renderables.get());
	}

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


