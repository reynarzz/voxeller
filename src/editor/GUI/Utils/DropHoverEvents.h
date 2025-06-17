#pragma once

#include <functional>
#include <string>
#include <vector>

struct DropEvent
{
    std::vector<std::string> paths; // Dropped files
    int x; // Mouse X
    int y; // Mouse Y
};

struct HoverEvent 
{
    int x; // Mouse X
    int y; // Mouse Y
};

class DropHoverEvents 
{
public:
    using DropCallback = std::function<void(const DropEvent&)>;
    using HoverCallback = std::function<void(const HoverEvent&)>;

    static void Initialize(void* glfwWin);
    static void Shutdown();

    static void SetDropCallback(DropCallback cb);
    static void SetHoverCallback(HoverCallback cb);

    static DropCallback dropCallback;
    static HoverCallback hoverCallback;
};
