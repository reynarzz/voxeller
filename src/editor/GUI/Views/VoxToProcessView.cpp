#include "VoxToProcessView.h"
#include <imgui/imgui.h>
#include <vector>
#include <voxeller/Types.h>
#include <imgui/imgui_internal.h>
#include <string>
#include <algorithm>


void VoxToProcessView::OnShowView()
{
}

std::vector<std::string> voxTest{ "Vox1", "vox animated", "another vox" };
s32 currentSelection = 0;
int prevSelection = 0;
// Draw a rounded button with a fill fraction [0..1] and return true if clicked
bool RoundedProgressButton(
	const char* label,
	const ImVec2& size,
	float fillFraction,             // 0.0 = empty, 1.0 = full
	const ImU32 bgCol,              // background color
	const ImU32 fillCol,            // fill color
	const ImU32 textCol,
	float rounding = 30.0f)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems) return false;

	// ID & layout
	ImGuiContext& g = *GImGui;
	const ImGuiID  id = window->GetID(label);
	ImVec2         pos = window->DC.CursorPos;
	ImVec2         sz = ImGui::CalcItemSize(size, ImGui::CalcTextSize(label).x + g.Style.FramePadding.x * 2,
		ImGui::GetTextLineHeightWithSpacing() + g.Style.FramePadding.y * 2);
	ImRect         bb(pos, ImVec2(pos.x + sz.x, pos.y + sz.y));
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, id)) return false;

	// Invisible button behavior
	bool clicked = ImGui::ButtonBehavior(bb, id, nullptr, nullptr);

	// Draw
	ImDrawList* draw = window->DrawList;
	// 1) background
	draw->AddRectFilled(bb.Min, bb.Max, bgCol, rounding);

	// 2) fill portion
	if (fillFraction > 0.0f)
	{
		float fillW = ImClamp(fillFraction, 0.0f, 1.0f) * sz.x;
		ImVec2 fillMax(bb.Min.x + fillW, bb.Max.y);
		draw->AddRectFilled(bb.Min, fillMax, fillCol, rounding, ImDrawFlags_::ImDrawFlags_RoundCornersLeft);
		// for right side we draw a clipped round corner on the right only if full
		if (fillFraction >= 1.0f)
			draw->AddRectFilled(bb.Min, bb.Max, fillCol, rounding);
	}

	// 3) label
	ImVec2 textSize = ImGui::CalcTextSize(label);
	ImVec2 textPos = ImVec2(
		bb.Min.x + (sz.x - textSize.x) * 0.5f,
		bb.Min.y + (sz.y - textSize.y) * 0.5f
	);
	draw->AddText(textPos, textCol, label);

	return clicked;
}


bool CornerButton(
	const char* label,
	const ImVec2& size,
	ImU32          bgColor,
	ImU32          textColor,
	float          rounding,
	ImDrawFlags    cornerFlags = ImDrawFlags_RoundCornersAll)   // no namespace qualifier
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems) return false;

	// compute bounding box
	ImGuiContext& g = *GImGui;
	ImVec2 textSize = ImGui::CalcTextSize(label);
	ImVec2 btnSize = ImGui::CalcItemSize(
		size,
		textSize.x + g.Style.FramePadding.x * 2,
		textSize.y + g.Style.FramePadding.y * 2
	);
	ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + btnSize.x, window->DC.CursorPos.y + btnSize.y));

	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, window->GetID(label))) return false;

	// button logic
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, window->GetID(label), &hovered, &held);

	// draw background with selective rounds
	window->DrawList->AddRectFilled(
		bb.Min, bb.Max,
		bgColor,
		rounding,
		cornerFlags
	);

	// draw label
	ImU32 col = held ? ImGui::GetColorU32(ImGuiCol_TextDisabled)
		: hovered ? ImGui::GetColorU32(ImGuiCol_Text)
		: textColor;
	ImVec2 textPos = {
		bb.Min.x + (btnSize.x - textSize.x) * 0.5f,
		bb.Min.y + (btnSize.y - textSize.y) * 0.5f
	};
	window->DrawList->AddText(textPos, col, label);

	return pressed;
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
	float offsetX = window->Pos.x + window->WindowPadding.x + (availW - textSize.x) * 0.5f;

	// set cursor
	ImGui::SetCursorScreenPos(ImVec2(offsetX, window->Pos.y + window->WindowPadding.y + 10));
	// draw text
	ImGui::TextUnformatted(title);
}

void ImageRounded(
	ImTextureID tex,
	const ImVec2& size,
	float rounding = 8.0f,
	ImDrawFlags corners = ImDrawFlags_RoundCornersAll,
	const ImVec2& uv0 = ImVec2(0, 0),
	const ImVec2& uv1 = ImVec2(1, 1),
	const ImVec4& tint_col = ImVec4(1, 1, 1, 1),
	const ImVec4& border_col = ImVec4(0, 0, 0, 0))
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems) return;

	// Reserve the item space
	ImGuiContext& g = *GImGui;
	ImVec2 pos = window->DC.CursorPos;
	ImRect bb(pos, { pos.x + size.x, pos.y + size.y });
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, 0)) return;

	// Draw the image with rounded mask
	ImDrawList* draw = window->DrawList;
	draw->AddImageRounded(
		tex,
		bb.Min, bb.Max,
		uv0, uv1,
		ImGui::GetColorU32(tint_col),
		rounding,
		corners
	);

	// Optional border
	if (border_col.w > 0.0f)
		draw->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(border_col), rounding, corners);
}

const f32 toolBarHeight = 40;
const f32 windowsSpacingY = 6;
const f32 windowsSpacingX = 5;

void ToolBar() 
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	bool open = true;
	ImGui::SetNextWindowSize({ ImGui::GetIO().DisplaySize.x - 10, toolBarHeight }, ImGuiCond_Always);
	ImGui::SetNextWindowPos({ 5, 6 }, ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(30, 30, 30, 0));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::Begin("Toolbar", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
	ImGui::SetCursorPosY(toolBarHeight / 4.0f);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15);

	ImGui::Text("View: Vox name");
	ImGui::End();
	ImGui::PopStyleVar(4);
	ImGui::PopStyleColor(2);
}

void ViewportWindow()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	bool open = true;
	ImGui::SetNextWindowSize({ ImGui::GetIO().DisplaySize.x - 270.0f, ImGui::GetIO().DisplaySize.y - toolBarHeight - windowsSpacingY - 13 }, ImGuiCond_Always);
	ImGui::SetNextWindowPos({ 5, toolBarHeight + windowsSpacingY * 2 }, ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(30, 30, 30, 0));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::Begin("Viewport", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	ImageRounded(10, ImGui::GetWindowSize(), 0);
	//ImGui::Image(nullptr, ImGui::GetWindowSize());

	f32 spacing = 1;
	f32 buttonDownWidth = 80;
	f32 buttonDOwnHeight = 30;
	std::string modelName = "Model converted Name";

	auto textSize = ImGui::CalcTextSize(modelName.c_str());
	ImGui::SetCursorPosY(ImGui::GetWindowSize().y - 67);
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0 - textSize.x / 2);

	ImGui::Text(modelName.c_str());

	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0 - buttonDownWidth);
	ImGui::SetCursorPosY(20);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(spacing, 0));
	CornerButton("3D view", { buttonDownWidth, buttonDOwnHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 20, ImDrawFlags_RoundCornersLeft);
	ImGui::SameLine();
	CornerButton("Atlas view", { buttonDownWidth, buttonDOwnHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 30, ImDrawFlags_RoundCornersRight);
	ImGui::PopStyleVar(2);
	ImGui::End();
	ImGui::PopStyleVar(4);
	ImGui::PopStyleColor(2);
}

void VoxToProcessView::UpdateGUI()
{
	ToolBar();
	ViewportWindow();
	// Sidebar region (no frame)
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	bool open = true;
	ImGui::SetNextWindowSize(ImVec2(std::min(250.0f, ImGui::GetIO().DisplaySize.x - 20), ImGui::GetIO().DisplaySize.y - toolBarHeight - windowsSpacingY - 13), ImGuiCond_Always);
	ImGui::SetNextWindowPos(ImVec2(std::clamp(ImGui::GetIO().DisplaySize.x - 260.0f, 10.0f, ImGui::GetIO().DisplaySize.x), toolBarHeight + windowsSpacingY * 2), ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(30, 30, 30, 255));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));

	ImGui::Begin("Sidebar", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	TitleLabel("Voxels Bag");
	ImGui::SameLine();

	f32 buttonUpWidth = 30;
	f32 buttonUpHeight = 30;
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 0));
	CornerButton("+", { buttonUpWidth, buttonUpHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 20, ImDrawFlags_RoundCornersAll);
	ImGui::SameLine();
	CornerButton("++", { buttonUpWidth, buttonUpHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 30, ImDrawFlags_RoundCornersAll);
	ImGui::PopStyleVar(2);


	ImGui::Dummy(ImVec2(0, 10));

	const float footerHeight = 120.0f;   // enough for your two buttons + padding

	// 3) Begin a child that will scroll:
	//    width = full, height = window height minus footerHeight
	ImVec2 winSize = ImGui::GetWindowSize();
	ImVec2 winPos = ImGui::GetCursorScreenPos();
	ImVec2 childSize = ImVec2(0, winSize.y - footerHeight);
	float rounding = 30.0f;
	ImU32 bgColor = IM_COL32(30, 30, 30, 255);



	// 3) Push an invisible frame so scrolling still works
	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
	ImGui::BeginChild("##ScrollingList",
		childSize,
		false,                      // no border
		ImGuiWindowFlags_None);
	// 2) Draw the rounded background
	ImGui::GetWindowDrawList()->AddRectFilled(
		winPos,
		ImVec2(winPos.x + childSize.x, winPos.y + childSize.y),
		bgColor,
		rounding
	);
	// Scroll only this list
	for (int i = 0; i < 20; ++i)
	{
		bool isSelected = (i == currentSelection);


		//RoundedProgressButton(std::string("Button " + std::to_string(i)).c_str(), {ImGui::GetContentRegionAvail().x, 30}, 0.2f, IM_COL32(65,105,255,255), IM_COL32(255, 2, 255, 255), IM_COL32(255, 255, 255, 255) );
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 3));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 0));
		CornerButton(std::string("" + std::to_string(i)).c_str(), { 25, 25 }, IM_COL32(30, 30, 30, 255), IM_COL32(255, 255, 255, 255), 30, ImDrawFlags_RoundCornersAll);
		ImGui::SameLine();

		CornerButton(std::string("Vox " + std::to_string(i)).c_str(), { ImGui::GetContentRegionAvail().x - 40, 25 }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 30, ImDrawFlags_RoundCornersLeft);
		ImGui::SameLine();

		CornerButton(std::string("D " + std::to_string(i)).c_str(), { ImGui::GetContentRegionAvail().x, 25 }, IM_COL32(255, 05, 55, 255), IM_COL32(255, 255, 255, 255), 30, ImDrawFlags_RoundCornersRight);

		ImGui::PopStyleVar(2);

		//ImGui::Text(std::string("Vox " + std::to_string(i)).c_str());


			// Handle click
		if (isSelected && currentSelection != i) {
			prevSelection = currentSelection;
			currentSelection = i;
			// your on‐select logic…
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();

	// Botton buttons THese should not scroll
	ImGui::SetCursorPosY(ImGui::GetWindowSize().y - 47);

	f32 spacing = 1;
	f32 buttonDownWidth = ImGui::GetContentRegionAvail().x / 2 - 3;
	f32 buttonDOwnHeight = 35;
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0 - buttonDownWidth);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(spacing, 0));
	CornerButton("Export All", { buttonDownWidth, buttonDOwnHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 20, ImDrawFlags_RoundCornersLeft);
	ImGui::SameLine();
	CornerButton("Folder", { buttonDownWidth, buttonDOwnHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 30, ImDrawFlags_RoundCornersRight);
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