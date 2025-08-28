#pragma once
#include <functional>
#include <string>

// Forward declaration to avoid leaking GLFW into dependents
struct GLFWwindow;

class NativeMenu
{
public:
    // Initialize after creating the GLFW window (main thread).
    static void Init(GLFWwindow* window);

    // Add a menu item at "A/B/C" -> creates submenus A->B and an item "C".
    static void Add(const std::string& path, std::function<void()> callback);

    // Add a menu item, optionally checkable (toggle=true). Click auto-toggles, then calls callback.
    static void Add(const std::string& path, std::function<void()> callback, bool toggle);

    // Programmatically set a checkable item's check state (no-op if item isn't checkable).
    static void Toggle(const std::string& path, bool checked);

    // Enable/disable a menu item or submenu holder (grays it out when disabled).
    static void Enable(const std::string& path, bool enabled);

    // Remove a single item or an entire submenu tree at "path".
    static void DestroyMenu(const std::string& path);

    // Optional cleanup before destroying the window/process.
    static void Shutdown(GLFWwindow* window);
};
