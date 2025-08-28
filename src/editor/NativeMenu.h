#pragma once
#include <functional>
#include <string>

// Forward-declare to avoid leaking GLFW headers
struct GLFWwindow;

class NativeMenu
{
public:
    // Call once after creating the GLFW window (main thread).
    static void Init(GLFWwindow* window);

    // "A/B/C" creates submenus A->B and an item "C" under B (or A if single segment).
    static void Add(const std::string& path, std::function<void()> callback);

    // Removes either a leaf menu item or a submenu (recursively removes all descendants).
    static void DestroyMenu(const std::string& path);

    // Optional cleanup before destroying the window/process.
    static void Shutdown(GLFWwindow* window);
};
