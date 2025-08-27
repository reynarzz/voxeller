#include "VoxToProcessView.h"

#include <vector>
#include <string>
#include <algorithm>
#include <Unvoxeller/File.h>
#include <Unvoxeller/Log/Log.h>
#include <Unvoxeller/Types.h>
#include <filesystem>
#include <GUI/Utils/GUIUtils.h>
#include <GUI/Utils/FileDialog.h>
#include <GUI/Screen.h>
#include <GUI/Utils/TextureLoader.h>

#include <Rendering/RenderingSystem.h>
#include <Data/VoxFileToProcessData.h>
#include <imgui/imgui_internal.h>
#include <GUI/VoxGUI.h>
#include <Rendering/Camera.h> // Remove this
#include <Unvoxeller/Unvoxeller.h>
#include <filesystem>

std::shared_ptr<Texture> blackImage = nullptr;
static std::vector<VOXFileToProcessData> _testVoxFiles{};
static Unvoxeller::Unvoxeller unvox{};
 
// Icons
std::shared_ptr<Texture> _trashIcon = nullptr;
std::shared_ptr<Texture> _addFileIcon = nullptr;
std::shared_ptr<Texture> _addFolderIcon = nullptr;
std::shared_ptr<Texture> _configIcon = nullptr;
std::shared_ptr<Texture> _cubeFillIcon = nullptr;
std::shared_ptr<Texture> _uvIcon = nullptr;
std::shared_ptr<Texture> _wireFrameIcon = nullptr;
std::shared_ptr<Texture> _lightIcon = nullptr;


std::vector<std::string> voxTest{ "Vox1", "vox animated", "another vox" };
s32 currentSelection = 0;
int prevSelection = 0;

const f32 toolBarHeight = 40;
const f32 windowsSpacingY = 6;
const f32 windowsSpacingX = 5;
//const ImVec4 WindowsBgColor = ImVec4(34.0f / 255.0f, 39.0f / 255.0f, 44.0f / 255.0f, 1.0f);
const ImVec4 WindowsBgColor = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
f32 _windowsRound = 7.0f;
s32 selectedIndex = 0;

f32 donuFill = 0;
std::string searchBar = "";

static std::string GetFileName(const std::string& fullPath)
{
	s32 start = fullPath.find_last_of(std::filesystem::path::preferred_separator) + 1;
	
	return fullPath.substr(start, fullPath.find_last_of(".") - start);
}

static std::string GetFileExtension(const std::string& fullPath)
{
	return fullPath.substr(fullPath.find_last_of(".")+1);
}

static Unvoxeller::ConvertOptions GetConvertOptions()
{
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
	convertOptions.Meshing.MergeMeshes = false;

	return convertOptions;
}

VoxToProcessView::VoxToProcessView()
{
	u8 black[] = { 0xFF, 0x00, 0xFF, 0xFF };
	TextureDescriptor tex = { 1, 1, 4, black };

	blackImage = Texture::Create(&tex);

	const std::string imagesRootFolder = Unvoxeller::File::GetExecutableDir() + "/assets/Images";

	_trashIcon = TextureLoader::LoadTexture(imagesRootFolder + "/icons8-cancel-30.png");
	_addFileIcon = TextureLoader::LoadTexture(imagesRootFolder + "/icons8-plus-64.png");
	_addFolderIcon = TextureLoader::LoadTexture(imagesRootFolder + "/icons8-open-file-64.png");
	_configIcon = TextureLoader::LoadTexture(imagesRootFolder + "/icons8-config-48.png");
	_cubeFillIcon = TextureLoader::LoadTexture(imagesRootFolder + "/icons8-cube-64.png");
	_uvIcon = TextureLoader::LoadTexture(imagesRootFolder + "/icons8-image-24.png");
	_wireFrameIcon = TextureLoader::LoadTexture(imagesRootFolder + "/icons8-cube-50.png");
	_lightIcon = TextureLoader::LoadTexture(imagesRootFolder + "/icons8-light-48.png");

	if (_trashIcon == nullptr || _addFileIcon == nullptr || _addFolderIcon == nullptr || _configIcon == nullptr)
	{
		LOG_ERROR("Icons null");
	}

	LOG_INFO("Initialize icons");
	//_testVoxFiles.resize(30);
}

void VoxToProcessView::OnShowView()
{
}

void TitleLabel(const char* title)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGuiIO& io = ImGui::GetIO();

	// measure text size
	ImVec2 textSize = ImGui::CalcTextSize(title);

	// compute available content width
	float availW = window->ContentRegionRect.Max.x - window->ContentRegionRect.Min.x;

	// calculate starting X so text is centered
	//float offsetX = window->Pos.x + window->WindowPadding.x + (availW - textSize.x) * 0.5f;
	float offsetX = window->Pos.x + 10;

	// set cursor
	ImGui::SetCursorScreenPos(ImVec2(offsetX, window->Pos.y + window->WindowPadding.y + 10));
	// draw text
	ImGui::TextUnformatted(title);
}

void ToolBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, _windowsRound);
	bool open = true;
	ImGui::SetNextWindowSize({ ImGui::GetIO().DisplaySize.x - 10, toolBarHeight }, ImGuiCond_Always);
	ImGui::SetNextWindowPos({ 5, 6 }, ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(30, 30, 30, 0));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, {0,0,0,0});
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::Begin("Toolbar", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
	const f32 elementsHeight = 25.0f;
	ImGui::SetCursorPosY((toolBarHeight - elementsHeight) / 2.0f);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 3);

	// Code Here

	ImGui::End();
	ImGui::PopStyleVar(4);
	ImGui::PopStyleColor(2);
}

void VoxToProcessView::ViewportWindow()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, _windowsRound);
	bool open = true;
	ImVec2 windowSize = { ImGui::GetIO().DisplaySize.x / 2 - 60.0f, ImGui::GetIO().DisplaySize.y - toolBarHeight - windowsSpacingY - 13 };
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
	ImGui::SetNextWindowPos({ 5, toolBarHeight + windowsSpacingY * 2 }, ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(30, 30, 30, 0));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, WindowsBgColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::Begin("Viewport", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	//if (ImGui::IsWindowHovered()) 
	{
		Camera::Update();
	}

	Screen::_width = ImGui::GetWindowWidth();
	Screen::_height = ImGui::GetWindowHeight();

	const f32 cursorX = ImGui::GetCursorPosX();

	// Cast it through intptr_t so the full 64-bit range is preserved,
	// then to ImTextureID (which on most backends is just void*)

	//ImageRounded(TEXTURE_TO_IMGUI(tex), ImGui::GetWindowSize(), 10);
	VoxGUI::ImageRounded(RENDER_TARGET_TO_IMGUI(RenderingSystem::GetRenderTarget().lock().get()), ImGui::GetWindowSize(), _windowsRound);


	//ImGui::Image(nullptr, ImGui::GetWindowSize());

	f32 spacing = 1;
	f32 buttonDownWidth = 20;
	f32 buttonDOwnHeight = 20;
	f32 xStartPos = 5;
	f32 yStartPos = 5;

	std::string modelName = "Model converted Name";

	auto textSize = ImGui::CalcTextSize(modelName.c_str());

	ImGui::SetCursorPosY(ImGui::GetWindowSize().y - 67);
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0 - textSize.x / 2);

	ImGui::Text(modelName.c_str());

	ImGui::SetCursorPosX(xStartPos);
	ImGui::SetCursorPosY(20);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yStartPos);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(spacing, 0));
	VoxGUI::ImageButton("##Fill_View", TEXTURE_TO_IMGUI2(_cubeFillIcon), { buttonDownWidth, buttonDOwnHeight }, { 1,1,1,1 }, { 1,1,1,1 }, { 1,1,1,0.5f }, ImDrawFlags_RoundCornersAll, ImVec2(0, 0), ImVec2(1, 1));
	ImGui::SetCursorPosX(xStartPos);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yStartPos);


	VoxGUI::ImageButton("##_WireFrame_View", TEXTURE_TO_IMGUI2(_wireFrameIcon), { buttonDownWidth, buttonDOwnHeight }, { 1,1,1,1 }, { 1,1,1,1 }, { 1,1,1,0.5f }, ImDrawFlags_RoundCornersAll, ImVec2(0, 0), ImVec2(1, 1));

	ImGui::SetCursorPosX(xStartPos);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yStartPos);
	VoxGUI::ImageButton("##_Light_View", TEXTURE_TO_IMGUI2(_lightIcon), { buttonDownWidth, buttonDOwnHeight }, { 1,1,1,1 }, { 1,1,1,1 }, { 1,1,1,0.5f }, ImDrawFlags_RoundCornersAll, ImVec2(0, 0), ImVec2(1, 1));


	ImGui::SetCursorPosX(xStartPos);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yStartPos);


	VoxGUI::ImageButton("##_UV_View", TEXTURE_TO_IMGUI2(_uvIcon), { buttonDownWidth, buttonDOwnHeight }, { 1,1,1,1 }, { 1,1,1,1 }, { 1,1,1,0.5f }, ImDrawFlags_RoundCornersAll, ImVec2(0, 0), ImVec2(1, 1));




	ImGui::PopStyleVar(2);
	ImGui::End();
	ImGui::PopStyleVar(4);
	ImGui::PopStyleColor(2);
}

void VoxToProcessView::TextureViewport()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, _windowsRound);
	bool open = true;

	const f32 xPos = ImGui::GetIO().DisplaySize.x / 2 - 60.0f + 10;
	const ImVec2 windowSize = {
		ImGui::GetIO().DisplaySize.x - 265.0f - xPos,
		ImGui::GetIO().DisplaySize.y - toolBarHeight - windowsSpacingY - 13
	};

	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
	ImGui::SetNextWindowPos({ xPos, toolBarHeight + windowsSpacingY * 2 }, ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(60, 60, 60, 255));
	ImGui::Begin("Texture Viewport", &open,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	// === Persistent state ===
	static float zoom = 1.0f;
	static ImVec2 pan(0.0f, 0.0f);
	static int selectedVertex = -1;

	ImVec2 winPos = ImGui::GetWindowPos();
	ImVec2 winSize = ImGui::GetWindowSize();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImGuiIO& io = ImGui::GetIO();

	// === Handle zoom/pan ===
	if (ImGui::IsWindowHovered()) {
		if (io.MouseWheel != 0.0f) {
			zoom *= (1.0f + io.MouseWheel * 0.1f);
			zoom = std::max(0.1f, std::min(zoom, 20.0f));
		}
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
			pan.x += io.MouseDelta.x;
			pan.y += io.MouseDelta.y;
		}
	}

	// === Draw texture as background ===
	// Replace `myTex` with your texture handle
	// ImGui::Image(TEXTURE_TO_IMGUI(myTex), winSize, ImVec2(0,1), ImVec2(1,0));

	// === Example mesh UVs (replace with your own data) ===
	static std::vector<ImVec2> uvs = {
		{0.1f, 0.1f}, {0.4f, 0.1f}, {0.25f, 0.4f},
		{0.6f, 0.2f}, {0.9f, 0.2f}, {0.75f, 0.5f}
	};
	static std::vector<int> indices = { 0,1,2, 3,4,5 };

	auto UVToScreen = [&](ImVec2 uv) -> ImVec2 {
		return ImVec2(
			winPos.x + pan.x + uv.x * winSize.x * zoom,
			winPos.y + pan.y + uv.y * winSize.y * zoom
		);
		};
	auto ScreenToUV = [&](ImVec2 pos) -> ImVec2 {
		return ImVec2(
			(pos.x - winPos.x - pan.x) / (winSize.x * zoom),
			(pos.y - winPos.y - pan.y) / (winSize.y * zoom)
		);
		};

	// === Draw triangles ===
	for (size_t i = 0; i < indices.size(); i += 3) {
		ImVec2 p0 = UVToScreen(uvs[indices[i]]);
		ImVec2 p1 = UVToScreen(uvs[indices[i + 1]]);
		ImVec2 p2 = UVToScreen(uvs[indices[i + 2]]);
		dl->AddLine(p0, p1, IM_COL32(255, 255, 255, 255), 1.0f);
		dl->AddLine(p1, p2, IM_COL32(255, 255, 255, 255), 1.0f);
		dl->AddLine(p2, p0, IM_COL32(255, 255, 255, 255), 1.0f);
	}

	// === Draw vertices (draggable) ===
	float handleSize = 6.0f;
	for (int i = 0; i < (int)uvs.size(); i++) {
		ImVec2 p = UVToScreen(uvs[i]);
		bool hovered = ImGui::IsMouseHoveringRect(
			ImVec2(p.x - handleSize, p.y - handleSize),
			ImVec2(p.x + handleSize, p.y + handleSize));

		ImU32 col = (hovered || selectedVertex == i)
			? IM_COL32(255, 128, 0, 255)
			: IM_COL32(0, 255, 255, 255);

		dl->AddCircleFilled(p, handleSize, col);

		// Begin drag if clicked
		if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			selectedVertex = i;
	}

	// === Drag selected vertex ===
	if (selectedVertex >= 0) {
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			ImVec2 mousePos = io.MousePos;
			uvs[selectedVertex] = ScreenToUV(mousePos);
			// Clamp to [0,1]
			uvs[selectedVertex].x = std::max(0.0f, std::min(1.0f, uvs[selectedVertex].x));
			uvs[selectedVertex].y = std::max(0.0f, std::min(1.0f, uvs[selectedVertex].y));
		}
		else {
			selectedVertex = -1; // release
		}
	}

	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}

void ExportWin()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, _windowsRound);
	bool open = true;
	f32 posX = std::clamp(ImGui::GetIO().DisplaySize.x - 260.0f, 10.0f, ImGui::GetIO().DisplaySize.x);

	ImGui::SetNextWindowPos(ImVec2(posX, ImGui::GetIO().DisplaySize.y - 125 + windowsSpacingY * 2), ImGuiCond_Always);
	const f32 height = ImGui::GetIO().DisplaySize.y - toolBarHeight - windowsSpacingY - (ImGui::GetIO().DisplaySize.y - 170) - 20;//ImGui::GetIO().DisplaySize.y - toolBarHeight - windowsSpacingY - 13;

	ImGui::SetNextWindowSize(ImVec2(std::min(ImGui::GetIO().DisplaySize.x - posX - 5, ImGui::GetIO().DisplaySize.x - 13), height), ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(30, 30, 30, 255));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, WindowsBgColor);

	ImGui::Begin("##ExportWin", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);



	f32 spacing = 1;
	f32 buttonDownWidth = 75;
	f32 buttonDOwnHeight = 25;
	std::vector<std::string> options = { "Fbx", "Obj" };

	VoxGUI::Dropdown("Format", &selectedIndex, options, 10, 200, 70);
	ImGui::Text("Config");
	ImGui::SetCursorPosY(ImGui::GetWindowSize().y - buttonDOwnHeight - 40);
	VoxGUI::ProgressBar(0.2f, { ImGui::GetContentRegionAvail().x / 1.7f, 7 }, 12, ImColor(20, 20, 20, 255), ImColor(0, 220, 150, 255));
	VoxGUI::ProgressBar(0.2f, { ImGui::GetContentRegionAvail().x / 1.7f, 7 }, 12, ImColor(20, 20, 20, 255), ImColor(0, 220, 150, 255));

	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0 - buttonDownWidth / 2);
	ImGui::SetCursorPosY(ImGui::GetWindowSize().y - buttonDOwnHeight - 10);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(spacing, 0));
	const bool isBuild = VoxGUI::Button("Build", TextAlign::Center, { buttonDownWidth, buttonDOwnHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 10, ImDrawFlags_RoundCornersAll);

	if(isBuild)
	{

		if(_testVoxFiles.size() == 0)
		{
			LOG_WARN("Nothing to build");
		}

		const Unvoxeller::ConvertOptions cOptions = GetConvertOptions();

		for (const auto& fileInfo : _testVoxFiles)
		{
			if(!fileInfo.Enabled)
			{	
				LOG_INFO("Disabled file: '{0}', Not exported.", fileInfo.FileName);
				continue;
			}
			Unvoxeller::ExportOptions exportOptions{};
				
			// V2
			exportOptions.OutputDir = Unvoxeller::File::GetExecutableDir() + "/testvox/nda/export";
			exportOptions.OutputName = fileInfo.FileName;
			exportOptions.InputPath = fileInfo.FullPath;
			exportOptions.OutputFormat = static_cast<Unvoxeller::ModelFormat>(selectedIndex);
			
			unvox.ExportVoxToModel(exportOptions, cOptions);
		}
	}

	ImGui::PopStyleVar(2);
	ImGui::End();
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}



void VoxToProcessView::UpdateGUI()
{
	ImGuiStyle& style = ImGui::GetStyle();

	// Remove scrollbar roundness
	style.ScrollbarRounding = 10.0f;

	// Make scrollbar thinner
	style.ScrollbarSize = 8.0f;  // default is 16.0f

	// Remove background color
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);  // Fully transparent

	// Optional: also remove grab roundness and change grab color
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(1, 1, 1, 0.7f);  // White
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1, 1, 1, 0.5f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1, 1, 1, 1.0f);

	ExportWin();
	ToolBar();
	ViewportWindow();
	TextureViewport();

	// Sidebar region (no frame)
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, _windowsRound);
	bool open = true;
	f32 posX = std::clamp(ImGui::GetIO().DisplaySize.x - 260.0f, 10.0f, ImGui::GetIO().DisplaySize.x);

	ImGui::SetNextWindowPos(ImVec2(posX, toolBarHeight + windowsSpacingY * 2), ImGuiCond_Always);
	const f32 height = ImGui::GetIO().DisplaySize.y - 170;//ImGui::GetIO().DisplaySize.y - toolBarHeight - windowsSpacingY - 13;

	ImGui::SetNextWindowSize(ImVec2(std::min(ImGui::GetIO().DisplaySize.x - posX - 5, ImGui::GetIO().DisplaySize.x - 13), height), ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(30, 30, 30, 255));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, WindowsBgColor);

	ImGui::Begin("Sidebar", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	TitleLabel("Voxels Bag");

	if (VoxGUI::SearchBar(searchBar))
	{

	}


	ImGui::Dummy(ImVec2(0, 10));

	const float footerHeight = 120.0f;   // enough for your two buttons + padding

	// 3) Begin a child that will scroll:
	//    width = full, height = window height minus footerHeight
	ImVec2 winSize = ImGui::GetWindowSize();
	ImVec2 winPos = ImGui::GetCursorScreenPos();
	ImVec2 childSize = ImVec2(ImGui::GetContentRegionAvail().x, winSize.y - footerHeight);

	ImU32 bgColor = IM_COL32(30, 30, 30, 255);


	VoxGUI::RoundedChild("##ScrollingList", []()
		{
			// Scroll only this list
			f32 defCursor = ImGui::GetCursorPosX();

			s32 deletedIndex = -1;
			for (int i = 0; i < _testVoxFiles.size(); ++i)
			{
				auto& voxFile = _testVoxFiles[i];

				bool isSelected = (i == currentSelection);

				//RoundedProgressButton(std::string("Button " + std::to_string(i)).c_str(), {ImGui::GetContentRegionAvail().x, 30}, 0.2f, IM_COL32(65,105,255,255), IM_COL32(255, 2, 255, 255), IM_COL32(255, 255, 255, 255) );
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 0));
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 0));

				ImGui::SetCursorPosX(defCursor + 7);

				VoxGUI::Checkbox(std::string("##Checkbox " + std::to_string(i)).c_str(), &voxFile.Enabled);

				ImGui::SameLine(0);
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 5);

				ImGui::PushID((std::string("#VOX_FILE_") + std::to_string(i)).c_str());
				VoxGUI::Label(voxFile.FileName.c_str());
				ImGui::PopID();

				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetWindowSize().x - 50);
				VoxGUI::ImageButton((std::string("_CONFIG_") + std::to_string(i)).c_str(), TEXTURE_TO_IMGUI2(_configIcon), { 20, 20 }, { 1,1,1,1 }, { 1,1,1,1 }, { 1,1,1,0.5f });

				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetWindowSize().x - 30);

				const bool isDeleted = VoxGUI::ImageButton(std::string("##_Delete_Vox_" + std::to_string(i)).c_str(), TEXTURE_TO_IMGUI2(_trashIcon), { 19, 19 }, { 1,1,1,1 }, { 1,1,1,1 }, { 1,1,1,0.5f });

				if(isDeleted)
				{
					deletedIndex = i;
				}

				//ImGui::Image(TEXTURE_TO_IMGUI(_trashIcon), {20, 20});
				//VoxGUI::Button(std::string("T##D" + std::to_string(i)).c_str(), TextAlign::Center, {20, 20}, IM_COL32(255, 05, 55, 255), IM_COL32(255, 255, 255, 255), 10, ImDrawFlags_RoundCornersAll);

				ImGui::PopStyleVar(2);

				//ImGui::Text(std::string("Vox " + std::to_string(i)).c_str());

				// Handle click
				if (isSelected && currentSelection != i)
				{
					prevSelection = currentSelection;
					currentSelection = i;
					// your on‐select logic…
				}
			}

			if(deletedIndex >= 0)
			{
				_testVoxFiles.erase(_testVoxFiles.begin()+deletedIndex);
			}
		}, childSize, _windowsRound, IM_COL32(25, 25, 25, 255));


	// Botton buttons THese should not scroll

	/*std::vector<std::string> items = { "A", "B", "C" };
	std::vector<const char*> item_ptrs;
	for (auto& s : items) item_ptrs.push_back(s.c_str());
	static int current_item = 0;
	ImGui::Combo("Dynamic", &current_item, item_ptrs.data(), item_ptrs.size());*/

	f32 buttonUpWidth = 25;
	f32 buttonUpHeight = 25;
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 0));

	f32 spacing = 1;
	f32 buttonDownWidth = 25;
	f32 buttonDOwnHeight = 25;
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0 - buttonDownWidth);
	//bool pressed = VoxGUI::Button("+", TextAlign::Center, { buttonUpWidth, buttonUpHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 20, ImDrawFlags_RoundCornersAll);
	bool pressed = VoxGUI::ImageButton("_OPEN_VOX_FILE", TEXTURE_TO_IMGUI2(_addFileIcon), { 22, 22 }, { 1,1,1,1 }, { 1,1,1,1 }, { 1,1,1,0.5f });
	
	if (pressed)
	{
		auto selectedFiles = FileDialog::OpenFiles("", { { "Voxels", "vox"} });

		for (auto fullPath : selectedFiles)
		{
			auto it = std::find_if(_testVoxFiles.begin(), _testVoxFiles.end(), [&](const VOXFileToProcessData& item) {return item.FullPath == fullPath;});

			if(it == _testVoxFiles.end())
			{	
				VOXFileToProcessData dataFile{};
				dataFile.Enabled = true;
				dataFile.UseCustomConfig = false;
				dataFile.FileName = GetFileName(fullPath);
				dataFile.FullPath = fullPath;
				dataFile.Extension = GetFileExtension(fullPath);
				_testVoxFiles.push_back(dataFile);
				
				LOG_INFO("Selected files: {0}", fullPath);
			}
			else
			{
				LOG_WARN("Already added: {0}", GetFileName(fullPath));
			}
		}
	}

	ImGui::SameLine();
	VoxGUI::ImageButton("++", TEXTURE_TO_IMGUI2(_addFolderIcon), { 22, 22 }, { 1,1,1,1 }, { 1,1,1,1 }, { 1,1,1,0.5f });
	//VoxGUI::Button("++", TextAlign::Center, { buttonUpWidth, buttonUpHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 30, ImDrawFlags_RoundCornersAll);
	ImGui::SameLine();
	VoxGUI::Button("-", TextAlign::Center, { buttonUpWidth, buttonUpHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 30, ImDrawFlags_RoundCornersAll);
	ImGui::SameLine();
	VoxGUI::ImageButton("_config_", TEXTURE_TO_IMGUI2(_configIcon), { 20, 20 }, { 1,1,1,1 }, { 1,1,1,1 }, { 1,1,1,0.5f });
	ImGui::SameLine();

	VoxGUI::DonutProgressBar("radial", donuFill, 10, 2, ImColor(20, 20, 20, 255), ImColor(240, 240, 240, 255), false);

	donuFill += 0.001f;

	if (donuFill > 1.0f) {
		donuFill = 0;
	}
	ImGui::PopStyleVar(2);

	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}

void VoxToProcessView::OnCloseView()
{
}