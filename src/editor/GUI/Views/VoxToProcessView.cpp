#include "VoxToProcessView.h"
#include <imgui/imgui.h>
#include <vector>
#include <Voxeller/Types.h>
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
	float fontSize = 0.0f,
	ImFont* fontOverride = nullptr
) {
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems) return false;

	ImGuiContext& g = *GImGui;
	ImFont* font = fontOverride ? fontOverride : g.Font;
	float useFontSize = (fontSize > 0.0f) ? fontSize : 13.0f;

	// Split label into display and ID parts ("Display##ID")
	const char* display_end = ImGui::FindRenderedTextEnd(label);
	const char* id_separator = strstr(label, "##");
	const char* display_label = label;
	const char* display_label_end = id_separator ? id_separator : display_end;

	// Calculate text size using display substring
	ImVec2 textSize = font->CalcTextSizeA(useFontSize, FLT_MAX, 0.0f, display_label, display_label_end);
	ImVec2 btnSize = ImGui::CalcItemSize(
		size,
		textSize.x + g.Style.FramePadding.x * 2,
		textSize.y + g.Style.FramePadding.y * 2
	);

	ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + btnSize.x, window->DC.CursorPos.y + btnSize.y));
	ImGui::ItemSize(bb);
	ImGuiID id = window->GetID(label);
	if (!ImGui::ItemAdd(bb, id)) return false;

	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

	// Animated hover transition
	static std::unordered_map<ImGuiID, float> hoverLerp;
	float& t = hoverLerp[id];
	float delta = ImGui::GetIO().DeltaTime;
	const float fadeSpeed = 6.0f;
	t = hovered ? ImMin(1.0f, t + delta * fadeSpeed) : ImMax(0.0f, t - delta * fadeSpeed);

	auto LerpColor = [](ImU32 a, ImU32 b, float t) {
		ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
		ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
		ImVec4 cc = ImLerp(ca, cb, t);
		return ImGui::ColorConvertFloat4ToU32(cc);
		};

	ImU32 bg = held
		? ImGui::GetColorU32(ImGuiCol_ButtonActive)
		: LerpColor(bgColor, ImGui::GetColorU32(ImGuiCol_ButtonHovered), t);

	if ((bg >> 24) > 0) {
		window->DrawList->AddRectFilled(bb.Min, bb.Max, bg, rounding, cornerFlags);
	}
	if ((borderColor >> 24) > 0 && borderThickness > 0.0f) {
		window->DrawList->AddRect(bb.Min, bb.Max, borderColor, rounding, cornerFlags, borderThickness);
	}

	// Text alignment
	ImVec2 textPos;
	switch (textAlign) {
	case TextAlign::Left:
		textPos.x = bb.Min.x + g.Style.FramePadding.x + 10.0f;
		break;
	case TextAlign::Center:
		textPos.x = bb.Min.x + (btnSize.x - textSize.x) * 0.5f;
		break;
	case TextAlign::Right:
		textPos.x = bb.Max.x - textSize.x - g.Style.FramePadding.x;
		break;
	}
	textPos.y = bb.Min.y + (btnSize.y - textSize.y) * 0.5f;

	// Smooth text color with opacity on press
	ImVec4 textCol = ImGui::ColorConvertU32ToFloat4(textColor);
	if (held) textCol.w *= 0.7f;
	else if (hovered) textCol = ImGui::GetStyleColorVec4(ImGuiCol_Text);
	ImU32 col = ImGui::ColorConvertFloat4ToU32(textCol);

	// Render only the display text portion
	window->DrawList->AddText(font, useFontSize, textPos, col, display_label, display_label_end);

	return pressed;
}


void Label(
	const char* label,
	float fontSize = 0.0f,                   // NEW: font size (0 = default)
	TextAlign textAlign = TextAlign::Left,
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
	//strcpy_s(buffer, sizeof(buffer), text.c_str());

#ifdef _WIN32
	strcpy_s(buffer, sizeof(buffer), text.c_str());
#else
	strlcpy(buffer, text.c_str(), sizeof(buffer));
#endif

	const bool active = ImGui::InputTextWithHint("##Search", "Vox Name...", buffer, IM_ARRAYSIZE(buffer));

	if (active)
	{
		text.resize(128);

#ifdef _WIN32
		strcpy_s(text.data(), sizeof(buffer), buffer);
#else
		strlcpy(buffer, text.c_str(), sizeof(buffer));
#endif

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
	CornerButton("3D view", TextAlign::Center, { buttonDownWidth, buttonDOwnHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 20, ImDrawFlags_RoundCornersLeft);
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
	drawList->AddRectFilled(backgroundPos, ImVec2(backgroundPos.x + size.x, backgroundPos.y + size.y + offset), bgColor, rounding);

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

void ProgressBar(float fraction, const ImVec2& size, float rounding, ImU32 bgColor, ImU32 fillColor)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImDrawList* drawList = window->DrawList;

	// Clamp fraction [0,1]
	fraction = ImClamp(fraction, 0.0f, 1.0f);

	// Background bar
	drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bgColor, rounding);

	// Filled portion
	float fillWidth = size.x * fraction;
	if (fillWidth > 0.0f) {
		ImVec2 fillEnd = ImVec2(pos.x + fillWidth, pos.y + size.y);
		drawList->AddRectFilled(pos, fillEnd, fillColor, rounding,
			fraction >= 1.0f ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersLeft);
	}

	// Space reservation
	ImGui::Dummy(size);
}

bool Dropdown(const char* label, int* currentIndex, const std::vector<std::string>& items,
	float rounding = 8.0f,
	float desiredComboWidth = 200.0f,
	float labelToComboSpacing = 10.0f)
{
	if (items.empty()) return false;

	// Colors
	ImU32 bgColor = IM_COL32(40, 40, 40, 255);
	ImU32 popupBgColor = IM_COL32(25, 25, 25, 240);
	ImU32 hoverColor = IM_COL32(60, 60, 60, 255);
	ImU32 activeColor = IM_COL32(80, 80, 80, 255);
	ImU32 borderColor = IM_COL32(0, 0, 0, 0);
	ImU32 arrowColor = IM_COL32(200, 200, 200, 255);

	ImGuiStyle& style = ImGui::GetStyle();
	float paddingY = style.FramePadding.y;

	// 1. Save Y cursor for alignment
	float cursorY = ImGui::GetCursorPosY();

	// 2. Draw label
	ImGui::SetCursorPosY(cursorY);
	ImGui::TextUnformatted(label);

	// 3. Compute vertical alignment
	float labelHeight = ImGui::GetTextLineHeight();
	float comboHeight = ImGui::GetFrameHeight();
	float verticalOffset = (labelHeight - comboHeight) * 0.5f;

	// 4. Place combo box with custom margin
	ImGui::SameLine(0.0f, labelToComboSpacing);
	ImGui::SetCursorPosY(cursorY + verticalOffset);

	float available = ImGui::GetContentRegionAvail().x;
	float comboWidth = ImMin(desiredComboWidth, available);
	ImGui::SetNextItemWidth(comboWidth);

	// 5. Track position
	ImVec2 buttonPos = ImGui::GetCursorScreenPos();
	float buttonHeight = ImGui::GetFrameHeight();

	// 6. Style
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
	ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, bgColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
	ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, borderColor);

	bool changed = false;

	// 7. Add text padding
	std::string paddedPreview = "  " + items[*currentIndex];
	const char* preview = paddedPreview.c_str();

	// 8. Clamp popup to visible area
	ImVec2 popupPos = ImVec2(buttonPos.x, buttonPos.y + buttonHeight - 1.0f);
	float safePopupX = popupPos.x + comboWidth;
	float maxX = ImGui::GetMainViewport()->WorkSize.x - 10.0f;

	if (safePopupX > maxX)
		popupPos.x = maxX - comboWidth;

	ImGui::SetNextWindowPos(popupPos);

	// 9. Combo
	if (ImGui::BeginCombo("##combo", preview, ImGuiComboFlags_NoArrowButton)) {
		ImGuiWindow* popup = ImGui::GetCurrentWindow();
		ImDrawList* draw = popup->DrawList;

		ImVec2 bgMin = popup->Pos;
		ImVec2 bgMax = ImVec2(bgMin.x + popup->Size.x, bgMin.y + popup->Size.y);
		draw->AddRectFilled(bgMin, bgMax, popupBgColor, rounding, ImDrawFlags_RoundCornersBottom);

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.0f);

		for (int i = 0; i < items.size(); ++i) {
			bool isSelected = (i == *currentIndex);
			if (ImGui::Selectable(items[i].c_str(), isSelected)) {
				*currentIndex = i;
				changed = true;
			}
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	// 10. Draw arrow
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	float arrowSize = buttonHeight * 0.35f;
	float arrowPadding = (buttonHeight - arrowSize) * 0.5f;

	ImVec2 arrowCenter = ImVec2(
		buttonPos.x + comboWidth - arrowPadding - arrowSize * 0.5f,
		buttonPos.y + buttonHeight * 0.5f
	);

	drawList->AddTriangleFilled(
		ImVec2(arrowCenter.x - arrowSize * 0.5f, arrowCenter.y - arrowSize * 0.3f),
		ImVec2(arrowCenter.x + arrowSize * 0.5f, arrowCenter.y - arrowSize * 0.3f),
		ImVec2(arrowCenter.x, arrowCenter.y + arrowSize * 0.4f),
		arrowColor
	);

	// 11. Cleanup
	ImGui::PopStyleColor(5);
	ImGui::PopStyleVar(2);

	return changed;
}


int selectedIndex = 0;
void ExportWin()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
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

	Dropdown("Format", &selectedIndex, options, 10, 200, 70);

	ImGui::SetCursorPosY(ImGui::GetWindowSize().y - buttonDOwnHeight - 40);
	ProgressBar(0.2f, { ImGui::GetContentRegionAvail().x / 1.7f, 7 }, 12, ImColor(20, 20, 20, 255), ImColor(0, 220, 150, 255));
	ProgressBar(0.2f, { ImGui::GetContentRegionAvail().x / 1.7f, 7 }, 12, ImColor(20, 20, 20, 255), ImColor(0, 220, 150, 255));

	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0 - buttonDownWidth / 2);
	ImGui::SetCursorPosY(ImGui::GetWindowSize().y - buttonDOwnHeight - 10);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(spacing, 0));
	CornerButton("Export", TextAlign::Center, { buttonDownWidth, buttonDOwnHeight }, IM_COL32(65, 105, 255, 255), IM_COL32(255, 255, 255, 255), 10, ImDrawFlags_RoundCornersAll);

	ImGui::PopStyleVar(2);


	ImGui::End();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}



std::string searchBar = "";
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
	// Sidebar region (no frame)
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
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
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 1));
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 0));

				Label(std::string("Vox " + std::to_string(i)).c_str());
				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetWindowSize().x - 50);
				
				CornerButton(std::string("T##D" + std::to_string(i)).c_str(), TextAlign::Center, {20, 20}, IM_COL32(255, 05, 55, 255), IM_COL32(255, 255, 255, 255), 10, ImDrawFlags_RoundCornersAll);

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

	f32 buttonUpWidth = 25;
	f32 buttonUpHeight = 25;
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 0));

	f32 spacing = 1;
	f32 buttonDownWidth = 25;
	f32 buttonDOwnHeight = 25;
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0 - buttonDownWidth);
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