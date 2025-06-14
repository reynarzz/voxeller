#include "VoxToProcessView.h"
#include <imgui/imgui.h>
#include <vector>
#include <voxeller/Types.h>
#include <imgui/imgui_internal.h>
#include <string>
#include <algorithm>
#include <functional>

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

enum class TextAlign {
	Left,
	Center,
	Right
};

bool CornerButton(
	const char* label,
	TextAlign textAlign,
	const ImVec2& size,
	ImU32 bgColor,
	ImU32 textColor,
	float rounding,
	ImDrawFlags cornerFlags = ImDrawFlags_RoundCornersAll,
	ImU32 borderColor = 0,
	float borderThickness = 1.0f,
	float fontSize = 0.0f,                   // NEW: font size (0 = default)
	ImFont* fontOverride = nullptr           // NEW: optional custom font
)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems) return false;

	ImGuiContext& g = *GImGui;
	ImFont* font = fontOverride ? fontOverride : g.Font;
	float useFontSize = (fontSize > 0.0f) ? fontSize : 13;

	// Calculate text size with custom font and size
	ImVec2 textSize = font->CalcTextSizeA(useFontSize, FLT_MAX, 0.0f, label);

	ImVec2 btnSize = ImGui::CalcItemSize(
		size,
		textSize.x + g.Style.FramePadding.x * 2,
		textSize.y + g.Style.FramePadding.y * 2
	);

	ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + btnSize.x, window->DC.CursorPos.y + btnSize.y));
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, window->GetID(label))) return false;

	// Logic
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, window->GetID(label), &hovered, &held);

	// Background
	ImU32 bg = bgColor;

	if (held) {
		bg = ImGui::GetColorU32(ImGuiCol_ButtonActive);  // or a custom color
	}
	else if (hovered) {
		bg = ImGui::GetColorU32(ImGuiCol_ButtonHovered); // or a custom color
	}

	if ((bg >> 24) > 0) {
		window->DrawList->AddRectFilled(bb.Min, bb.Max, bg, rounding, cornerFlags);
	}
	// Border
	if ((borderColor >> 24) > 0 && borderThickness > 0.0f) {
		window->DrawList->AddRect(bb.Min, bb.Max, borderColor, rounding, cornerFlags, borderThickness);
	}

	// Text position
	ImVec2 textPos;
	switch (textAlign) {
	case TextAlign::Left:
		textPos.x = bb.Min.x + g.Style.FramePadding.x + 10;
		break;
	case TextAlign::Center:
		textPos.x = bb.Min.x + (btnSize.x - textSize.x) * 0.5f;
		break;
	case TextAlign::Right:
		textPos.x = bb.Max.x - textSize.x - g.Style.FramePadding.x;
		break;
	}
	textPos.y = bb.Min.y + (btnSize.y - textSize.y) * 0.5f;

	// Text color and render
	ImU32 col = held ? ImGui::GetColorU32(ImGuiCol_TextDisabled)
		: hovered ? ImGui::GetColorU32(ImGuiCol_Text)
		: textColor;

	window->DrawList->AddText(font, useFontSize, textPos, col, label);

	return pressed;
}

void Label(
	const char* label,
	float fontSize = 0.0f,                   // NEW: font size (0 = default)
	TextAlign textAlign= TextAlign::Left,
	ImU32 textColor = 0xFFFFFFFF,
	ImU32 bgColor = 0,
	float rounding = 0,
	ImDrawFlags cornerFlags = ImDrawFlags_RoundCornersAll,
	ImU32 borderColor = 0,
	float borderThickness = 1.0f,
	ImFont* fontOverride = nullptr           // NEW: optional custom font
)
{
	const ImVec2 size = ImGui::CalcTextSize(label);

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems) return;

	ImGuiContext& g = *GImGui;
	ImFont* font = fontOverride ? fontOverride : g.Font;
	float useFontSize = (fontSize > 0.0f) ? fontSize : 13;

	// Calculate text size with custom font and size
	ImVec2 textSize = font->CalcTextSizeA(useFontSize, FLT_MAX, 0.0f, label);

	ImVec2 btnSize = ImGui::CalcItemSize(
		size,
		textSize.x + g.Style.FramePadding.x * 2,
		textSize.y + g.Style.FramePadding.y * 2
	);

	ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + btnSize.x, window->DC.CursorPos.y + btnSize.y));
	ImGui::ItemSize(bb);
	if (!ImGui::ItemAdd(bb, window->GetID(label))) return;

	// Background
	ImU32 bg = bgColor;

	if ((bg >> 24) > 0) {
		window->DrawList->AddRectFilled(bb.Min, bb.Max, bg, rounding, cornerFlags);
	}
	// Border
	if ((borderColor >> 24) > 0 && borderThickness > 0.0f) {
		window->DrawList->AddRect(bb.Min, bb.Max, borderColor, rounding, cornerFlags, borderThickness);
	}

	// Text position
	ImVec2 textPos;
	switch (textAlign) {
	case TextAlign::Left:
		textPos.x = bb.Min.x + g.Style.FramePadding.x + 10;
		break;
	case TextAlign::Center:
		textPos.x = bb.Min.x + (btnSize.x - textSize.x) * 0.5f;
		break;
	case TextAlign::Right:
		textPos.x = bb.Max.x - textSize.x - g.Style.FramePadding.x;
		break;
	}
	textPos.y = bb.Min.y + (btnSize.y - textSize.y) * 0.5f;

	// Text color and render
	ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);

	window->DrawList->AddText(font, useFontSize, textPos, col, label);

	return;
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
//const ImVec4 WindowsBgColor = ImVec4(34.0f / 255.0f, 39.0f / 255.0f, 44.0f / 255.0f, 1.0f);
const ImVec4 WindowsBgColor = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);


static bool SearchBar(std::string& text) 
{
	// Style tweaks for rounded edges
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);            // full round
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));     // extra padding
	ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(30, 30, 30, 255)); // dark background
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(70, 70, 70, 255));  // subtle border
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

	// Optional: set width
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

	static char buffer[128] = "";
	strcpy_s(buffer, sizeof(buffer), text.c_str());

	const bool active = ImGui::InputTextWithHint("##Search", "Vox Name...", buffer, IM_ARRAYSIZE(buffer));

	if (active) 
	{
		text.resize(128);
		strcpy_s(text.data(), sizeof(buffer), buffer);
	}

	// Restore style
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(2);

	return active;
}
void ToolBar() 
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	bool open = true;
	ImGui::SetNextWindowSize({ ImGui::GetIO().DisplaySize.x - 10, toolBarHeight }, ImGuiCond_Always);
	ImGui::SetNextWindowPos({ 5, 6 }, ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(30, 30, 30, 0));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, WindowsBgColor);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::Begin("Toolbar", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
	const f32 elementsHeight = 25.0f;
	ImGui::SetCursorPosY((toolBarHeight - elementsHeight) / 2.0f);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15);


	
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
	ImGui::PushStyleColor(ImGuiCol_WindowBg, WindowsBgColor);
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
	CornerButton("3D view", TextAlign::Center,{ buttonDownWidth, buttonDOwnHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 20, ImDrawFlags_RoundCornersLeft);
	ImGui::SameLine();
	CornerButton("Atlas view", TextAlign::Center, { buttonDownWidth, buttonDOwnHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 30, ImDrawFlags_RoundCornersRight);
	ImGui::PopStyleVar(2);
	ImGui::End();
	ImGui::PopStyleVar(4);
	ImGui::PopStyleColor(2);
}

void RoundedChild(const char* id, std::function<void()> content, ImVec2 size, float rounding, ImU32 bgColor, ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground, ImU32 borderColor = 0, float borderThickness = 1.0f)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImDrawList* drawList = window->DrawList;
	ImVec2 pos = ImGui::GetCursorScreenPos();

	// Draw rounded background
	 auto backgroundPos = pos;
	 const float offset = 3;
	backgroundPos.y -= offset;
	drawList->AddRectFilled(backgroundPos, ImVec2(backgroundPos.x + size.x, backgroundPos.y + size.y+ offset), bgColor, rounding);

	// Optional border
	if ((borderColor >> 24) > 0 && borderThickness > 0.0f) {
		drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), borderColor, rounding, 0, borderThickness);
	}

	// Push style to disable ImGui child background and border
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));  // transparent
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, rounding);     // will have no effect directly
	ImGui::BeginChild(id, size, false, flags);
	ImGui::Dummy({ 0, 4 });
	content();
	
	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}


std::string searchBar = "";
void VoxToProcessView::UpdateGUI()
{
	ImGuiStyle& style = ImGui::GetStyle();

	// Remove scrollbar roundness
	style.ScrollbarRounding = 0.0f;

	// Make scrollbar thinner
	style.ScrollbarSize = 4.0f;  // default is 16.0f

	// Remove background color
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);  // Fully transparent

	// Optional: also remove grab roundness and change grab color
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(1, 1, 1, 0.7f);  // White
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1, 1, 1, 0.5f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1, 1, 1, 1.0f);


	ToolBar();
	ViewportWindow();
	// Sidebar region (no frame)
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	bool open = true;
	f32 posX = std::clamp(ImGui::GetIO().DisplaySize.x - 260.0f, 10.0f, ImGui::GetIO().DisplaySize.x);

	ImGui::SetNextWindowPos(ImVec2(posX, toolBarHeight + windowsSpacingY * 2), ImGuiCond_Always);
	const f32 height = ImGui::GetIO().DisplaySize.y - 70;//ImGui::GetIO().DisplaySize.y - toolBarHeight - windowsSpacingY - 13;

	ImGui::SetNextWindowSize(ImVec2(std::min(ImGui::GetIO().DisplaySize.x - posX - 5, ImGui::GetIO().DisplaySize.x - 13), height), ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(30, 30, 30, 255));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, WindowsBgColor);

	ImGui::Begin("Sidebar", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

	TitleLabel("Voxels Bag");
	if (SearchBar(searchBar)) 
	{
		
	}


	ImGui::Dummy(ImVec2(0, 10));

	const float footerHeight = 120.0f;   // enough for your two buttons + padding

	// 3) Begin a child that will scroll:
	//    width = full, height = window height minus footerHeight
	ImVec2 winSize = ImGui::GetWindowSize();
	ImVec2 winPos = ImGui::GetCursorScreenPos();
	ImVec2 childSize = ImVec2(ImGui::GetContentRegionAvail().x, winSize.y - footerHeight);
	float rounding = 10.0f;
	ImU32 bgColor = IM_COL32(30, 30, 30, 255);

	RoundedChild("##ScrollingList", []()
		{
			// Scroll only this list
			for (int i = 0; i < 30; ++i)
			{
				bool isSelected = (i == currentSelection);


				//RoundedProgressButton(std::string("Button " + std::to_string(i)).c_str(), {ImGui::GetContentRegionAvail().x, 30}, 0.2f, IM_COL32(65,105,255,255), IM_COL32(255, 2, 255, 255), IM_COL32(255, 255, 255, 255) );
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 3));
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 0));

				Label(std::string("Vox " + std::to_string(i)).c_str());
				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetWindowSize().x - 50);
				CornerButton(std::string("D " + std::to_string(i)).c_str(), TextAlign::Center, { 25, 25 }, IM_COL32(255, 05, 55, 255), IM_COL32(255, 255, 255, 255), 10, ImDrawFlags_RoundCornersAll);

				ImGui::PopStyleVar(2);

				//ImGui::Text(std::string("Vox " + std::to_string(i)).c_str());


					// Handle click
				if (isSelected && currentSelection != i) {
					prevSelection = currentSelection;
					currentSelection = i;
					// your on‐select logic…
				}
			}
		}, childSize, rounding, IM_COL32(25, 25, 25, 255));


	// Botton buttons THese should not scroll
	
	/*std::vector<std::string> items = { "A", "B", "C" };
	std::vector<const char*> item_ptrs;
	for (auto& s : items) item_ptrs.push_back(s.c_str());
	static int current_item = 0;
	ImGui::Combo("Dynamic", &current_item, item_ptrs.data(), item_ptrs.size());*/
	f32 spacing = 1;
	f32 buttonDownWidth = 25;
	f32 buttonDOwnHeight = 25;
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0 - buttonDownWidth);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(spacing, 0));
	CornerButton("Ex", TextAlign::Center, { buttonDownWidth, buttonDOwnHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 10, ImDrawFlags_RoundCornersLeft);
	ImGui::SameLine();
	CornerButton("Op", TextAlign::Center, { buttonDownWidth, buttonDOwnHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 10, ImDrawFlags_RoundCornersRight);
	ImGui::PopStyleVar(2);


	ImGui::SameLine();
	f32 buttonUpWidth = 25;
	f32 buttonUpHeight = 25;
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 0));
	CornerButton("+", TextAlign::Center, { buttonUpWidth, buttonUpHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 20, ImDrawFlags_RoundCornersAll);
	ImGui::SameLine();
	CornerButton("++", TextAlign::Center, { buttonUpWidth, buttonUpHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 30, ImDrawFlags_RoundCornersAll);
	ImGui::SameLine();
	CornerButton("-", TextAlign::Center, { buttonUpWidth, buttonUpHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 30, ImDrawFlags_RoundCornersAll);
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