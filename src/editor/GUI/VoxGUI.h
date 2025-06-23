#pragma once
#include <imgui/imgui.h>
#include <vector>
#include <string>
#include <functional>

enum class TextAlign 
{
	Left,
	Center,
	Right
};

class VoxGUI
{
public:
    static void Label(
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
    );

    static bool ButtonProgress(
        const char* label,
        const ImVec2& size,
        float fillFraction,             // 0.0 = empty, 1.0 = full
        const ImU32 bgCol,              // background color
        const ImU32 fillCol,            // fill color
        const ImU32 textCol,
        float rounding = 30.0f);

    static bool Button(
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
    );

    static void ImageRounded(
        intptr_t* tex,
        const ImVec2& size,
        float rounding = 8.0f,
        ImDrawFlags corners = ImDrawFlags_RoundCornersAll,
        const ImVec2& uv0 = ImVec2(0, 1),
        const ImVec2& uv1 = ImVec2(1, 0),
        const ImVec4& tint_col = ImVec4(1, 1, 1, 1),
        const ImVec4& border_col = ImVec4(0, 0, 0, 0));

    static bool Checkbox(const char* id, bool* v);

    static void DonutProgressBar(const char* label,
        float fraction,        // 0.0 → 0% , 1.0 → 100%
        float radius,          // outer radius in pixels
        float thickness,       // ring width in pixels
        ImU32 bg_col,          // background ring color
        ImU32 fg_col,          // filled‐portion color
        bool show_text = true); // draw percentage in center

    static void ProgressBar(float fraction, const ImVec2& size, float rounding, ImU32 bgColor, ImU32 fillColor);
    static bool Dropdown(const char* label, int* currentIndex, const std::vector<std::string>& items,
                        float rounding = 8.0f,
                        float desiredComboWidth = 200.0f,
                        float labelToComboSpacing = 10.0f);

    static void RoundedChild(const char* id, std::function<void()> content, ImVec2 size, 
                          float rounding, ImU32 bgColor, ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground, 
                          ImU32 borderColor = 0, float borderThickness = 1.0f);

    static bool SearchBar(std::string& text);
};
