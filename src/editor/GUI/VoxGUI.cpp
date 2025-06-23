#include "VoxGUI.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <unordered_map>

void VoxGUI::Label(
	const char* label,
	float fontSize,                   // NEW: font size (0 = default)
	TextAlign textAlign,
	ImU32 textColor,
	ImU32 bgColor,
	float rounding,
	ImDrawFlags cornerFlags,
	ImU32 borderColor,
	float borderThickness,
	ImFont* fontOverride           // NEW: optional custom font
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

bool VoxGUI::ButtonProgress(
	const char* label,
	const ImVec2& size,
	float fillFraction,             
	const ImU32 bgCol,              
	const ImU32 fillCol,            
	const ImU32 textCol,
	float rounding)
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

bool VoxGUI::Button(
	const char* label,
	TextAlign textAlign,
	const ImVec2& size,
	ImU32 bgColor,
	ImU32 textColor,
	float rounding,
	ImDrawFlags cornerFlags,
	ImU32 borderColor,
	float borderThickness,
	float fontSize,
	ImFont* fontOverride
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

	if (hovered)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}

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


void VoxGUI::ImageRounded(
	intptr_t* tex,
	const ImVec2& size,
	float rounding,
	ImDrawFlags corners,
	const ImVec2& uv0,
	const ImVec2& uv1,
	const ImVec4& tint_col,
	const ImVec4& border_col)
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


bool VoxGUI::Checkbox(const char* id, bool* v)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    if (window->SkipItems)
        return false;

    ImGuiContext& g     = *GImGui;
    ImGuiStyle&  style = g.Style;

    // Size & bounding box
    float square_sz = ImGui::GetFrameHeight();
    ImVec2 pos      = window->DC.CursorPos;
    ImVec2 bb_min   = pos;
    ImVec2 bb_max   = ImVec2(pos.x + square_sz, pos.y + square_sz);
    ImRect total_bb(bb_min, bb_max);

    // Reserve & ID
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    ImGuiID widget_id = window->GetID(id);
    if (!ImGui::ItemAdd(total_bb, widget_id))
        return false;

    // Interaction
    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, widget_id, &hovered, &held);
    if (hovered)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

    // Animation storage keys
    ImGuiStorage* store = window->DC.StateStorage;
    const ImGuiID key_on  = widget_id ^ 0xCAFEBABE;
    const ImGuiID key_off = widget_id ^ 0xDEADBEEF;
    float t_now = ImGui::GetTime();
    float t_on  = store->GetFloat(key_on,  -1.0f);
    float t_off = store->GetFloat(key_off, -1.0f);

    // Toggle and kickoff animation
    if (pressed)
    {
        bool newv = !*v;
        *v = newv;
        if (newv)
        {
            store->SetFloat(key_on,  t_now);
            store->SetFloat(key_off, -1.0f);
        }
        else
        {
            store->SetFloat(key_off, t_now);
            store->SetFloat(key_on,  -1.0f);
        }
    }

    // Draw background
    ImU32 custom_bg = IM_COL32(45, 45, 45, 255);
    window->DrawList->AddRectFilled(bb_min, bb_max, custom_bg, 10.0f);

    // Shared center & base radius
    ImVec2 center{
        (bb_min.x + bb_max.x) * 0.5f,
        (bb_min.y + bb_max.y) * 0.5f
    };
    float base_radius = square_sz * 0.3f;

    // ON animation: grow + fade-in
    if (*v && t_on > 0.0f)
    {
        float anim = ImSaturate((t_now - t_on) / 0.3f);
        float r = base_radius * anim;
        int   a = (int)(200 * anim);
        window->DrawList->AddCircleFilled(center, r, IM_COL32(255,255,255,a), 16);

        if (anim > 0.5f)
        {
            float c = (anim - 0.5f)*2.0f;
            float thickness = 2.5f * c;
            ImU32 col_check = IM_COL32(50,50,50,(int)(255 * c));
            ImVec2 p1{ center.x - square_sz * 0.15f, center.y + square_sz * 0.02f };
            ImVec2 p2{ center.x - square_sz * 0.02f, center.y + square_sz * 0.15f };
            ImVec2 p3{ center.x + square_sz * 0.20f, center.y - square_sz * 0.15f };
            ImVec2 pts[3] = { p1, p2, p3 };
            window->DrawList->AddPolyline(pts, 3, col_check, false, thickness);
        }
    }
    // OFF animation: shrink + fade-out
    else if (!*v && t_off > 0.0f)
    {
        float anim = ImSaturate((t_now - t_off) / 0.3f);
        float inv  = 1.0f - anim;
        float r    = base_radius * inv;
        int   a    = (int)(200 * inv);
        // draw shrinking circle
        if (r > 0.0f && a > 0)
            window->DrawList->AddCircleFilled(center, r, IM_COL32(255,255,255,a), 16);

        // draw fading check until halfway
        if (anim < 0.5f)
        {
            float c = (0.5f - anim)*2.0f; // 1→0 over first half
            float thickness = 2.5f * c;
            ImU32 col_check = IM_COL32(50,50,50,(int)(255 * c));
            ImVec2 p1{ center.x - square_sz * 0.15f, center.y + square_sz * 0.02f };
            ImVec2 p2{ center.x - square_sz * 0.02f, center.y + square_sz * 0.15f };
            ImVec2 p3{ center.x + square_sz * 0.20f, center.y - square_sz * 0.15f };
            ImVec2 pts[3] = { p1, p2, p3 };
            window->DrawList->AddPolyline(pts, 3, col_check, false, thickness);
        }

        // reset storage once done
        if (anim >= 1.0f)
            store->SetFloat(key_off, -1.0f);
    }
    // Static checked state (no animation keys) — draw final circle + check
    else if (*v && t_on < 0.0f)
    {
        window->DrawList->AddCircleFilled(center, base_radius, IM_COL32(255,255,255,200), 16);
        ImVec2 p1{ center.x - square_sz * 0.15f, center.y + square_sz * 0.02f };
        ImVec2 p2{ center.x - square_sz * 0.02f, center.y + square_sz * 0.15f };
        ImVec2 p3{ center.x + square_sz * 0.20f, center.y - square_sz * 0.15f };
        ImVec2 pts[3] = { p1, p2, p3 };
        window->DrawList->AddPolyline(pts, 3, IM_COL32(50,50,50,255), false, 2.5f);
    }

    return pressed;
}


void VoxGUI::DonutProgressBar(const char* label,
	float fraction,        
	float radius,          
	float thickness,       
	ImU32 bg_col,          
	ImU32 fg_col,          
	bool show_text) 
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems) return;

	// Reserve a square for the control
	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size(radius * 2, radius * 2);
	ImGui::InvisibleButton(label, size);
	if (!ImGui::IsItemVisible()) return;

	ImDrawList* draw = ImGui::GetWindowDrawList();
	const ImVec2 center = ImVec2(pos.x + radius, pos.y + radius);

	// Mid‐line of the ring
	float mid_r = radius - thickness * 0.5f;
	const int  num_segments = 60;

	// 1) Background ring
	draw->AddCircle(center, mid_r, bg_col, num_segments, thickness);

	// 2) Foreground (progress) arc
	if (fraction > 0.0f)
	{
		float start_angle = -IM_PI * 0.5f;
		float end_angle = start_angle + fraction * 2 * IM_PI;

		draw->PathClear();
		draw->PathArcTo(center, mid_r, start_angle, end_angle, num_segments);
		draw->PathStroke(fg_col, false, thickness);
	}

	// 3) Optional centered text
	if (show_text)
	{
		char buf[16];
		sprintf(buf, "%d%%", int(fraction * 100 + 0.5f));
		ImVec2 ts = ImGui::CalcTextSize(buf);
		draw->AddText(
			ImVec2(center.x - ts.x * 0.5f,
				center.y - ts.y * 0.5f),
			ImGui::GetColorU32(ImGuiCol_Text),
			buf
		);
	}
}

void VoxGUI::ProgressBar(float fraction, const ImVec2& size, float rounding, ImU32 bgColor, ImU32 fillColor)
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

bool VoxGUI::Dropdown(const char* label, int* currentIndex, const std::vector<std::string>& items,
	float rounding,
	float desiredComboWidth,
	float labelToComboSpacing)
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
	if (ImGui::BeginCombo("##combo", preview, ImGuiComboFlags_NoArrowButton)) 
	{
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

			if (ImGui::IsItemHovered())
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			}
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}

	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
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

void VoxGUI::RoundedChild(const char* id, std::function<void()> content, ImVec2 size, 
                          float rounding, ImU32 bgColor, ImGuiWindowFlags flags, 
                          ImU32 borderColor, float borderThickness)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImDrawList* drawList = window->DrawList;
	ImVec2 pos = ImGui::GetCursorScreenPos();

	// Draw rounded background
	auto backgroundPos = pos;
	const float offset = 0;
	backgroundPos.y -= offset;
	drawList->AddRectFilled(backgroundPos, ImVec2(backgroundPos.x + size.x, backgroundPos.y + size.y + offset), bgColor, rounding);

	// Optional border
	if ((borderColor >> 24) > 0 && borderThickness > 0.0f) 
    {
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

bool VoxGUI::SearchBar(std::string& text)
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

bool VoxGUI::ImageButton(
    const char*   id,
    ImTextureID   texture,
    const ImVec2& size,
    const ImVec4& tintNormal,
    const ImVec4& tintHover,
    const ImVec4& tintActive,
    ImGuiButtonFlags flags,
    const ImVec2& uv0,
    const ImVec2& uv1
)
{
    // 1) Reserve the interaction area
    bool pressed = ImGui::InvisibleButton(id, size, flags);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	}
    // 2) Get state
    bool hovered = ImGui::IsItemHovered();
    bool held    = ImGui::IsItemActive();

    // 3) Choose the tint
    ImVec4 tint = tintNormal;
    if      (held)    tint = tintActive;
    else if (hovered) tint = tintHover;

    // 4) Draw the image into that same slot
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddImage(
        texture, p0, p1, uv0, uv1,
        ImGui::ColorConvertFloat4ToU32(tint)
    );

    return pressed;
}
