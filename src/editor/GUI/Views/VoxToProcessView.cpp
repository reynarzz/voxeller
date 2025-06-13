#include "VoxToProcessView.h"
#include <imgui/imgui.h>
#include <vector>
#include <voxeller/Types.h>
#include <imgui/imgui_internal.h>


void VoxToProcessView::OnShowView()
{
}

std::vector<std::string> voxTest{"Vox1", "vox animated", "another vox"};
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
    ImGuiWindow*   window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    // ID & layout
    ImGuiContext&  g = *GImGui;
    const ImGuiID  id = window->GetID(label);
    ImVec2         pos = window->DC.CursorPos;
    ImVec2         sz  = ImGui::CalcItemSize(size, ImGui::CalcTextSize(label).x + g.Style.FramePadding.x*2, 
                                                     ImGui::GetTextLineHeightWithSpacing() + g.Style.FramePadding.y*2);
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
        bb.Min.x + (sz.x - textSize.x)*0.5f,
        bb.Min.y + (sz.y - textSize.y)*0.5f
    );
    draw->AddText(textPos, textCol, label);

    return clicked;
}


bool CustomCornerButton(
    const char*    label,
    const ImVec2&  size,
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
    ImVec2 btnSize  = ImGui::CalcItemSize(
        size,
        textSize.x + g.Style.FramePadding.x*2,
        textSize.y + g.Style.FramePadding.y*2
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
    ImU32 col = held    ? ImGui::GetColorU32(ImGuiCol_TextDisabled)
              : hovered ? ImGui::GetColorU32(ImGuiCol_Text)
                        : textColor;
    ImVec2 textPos = {
        bb.Min.x + (btnSize.x - textSize.x)*0.5f,
        bb.Min.y + (btnSize.y - textSize.y)*0.5f
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
void VoxToProcessView::UpdateGUI()
{
    // Sidebar region (no frame)
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 30.0f);
    bool open = true;
    ImGui::SetNextWindowSize(ImVec2(280, ImGui::GetIO().DisplaySize.y - 30), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x-290,20), ImGuiCond_Always);

ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255,255,255,255)); // whatever color you like

    ImGui::Begin("Sidebar",&open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    TitleLabel("Voxels Bag");
    ImGui::Dummy(ImVec2(0, 30));
    CustomCornerButton("Button", {70, 30},IM_COL32(255,255,255,255),IM_COL32(255,255,255,255), 20, ImDrawFlags_RoundCornersRight);

    RoundedProgressButton("Button2", {150, 30}, 0.2f, ImColor(0x4A90E2FF), IM_COL32(255, 2, 255, 255), IM_COL32(255, 255, 255, 255) );
  
    // ——— Item list ———
    for (int i = 0; i < (int)voxTest.size(); ++i)
    {
        bool isSelected = (i == currentSelection);

        

        // Handle click
        if (isSelected && currentSelection != i) {
            prevSelection = currentSelection;
            currentSelection = i;
            // your on‐select logic…
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
ImGui::PopStyleVar();
}


void VoxToProcessView::OnCloseView()
{
}