#if _WIN32
// native_menu_win.cpp
#include "NativeMenu.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

namespace
{
    HWND g_hwnd = nullptr;
    HMENU g_bar = nullptr;
    WNDPROC g_prevProc = nullptr;
    int g_nextId = 30000;

    // Menus and items
    std::unordered_map<std::string, HMENU> g_menuByPath;     // "File" -> HMENU, "File/Recent" -> HMENU
    std::unordered_map<std::string, int>   g_itemIdByPath;   // "File/Open" -> ID
    std::unordered_map<int, std::function<void()>> g_cbById; // ID -> callback

    std::vector<std::string> Split(const std::string& s)
    {
        std::vector<std::string> out;
        size_t i = 0, j;
        while ((j = s.find('/', i)) != std::string::npos)
        {
            if (j > i) out.push_back(s.substr(i, j - i));
            i = j + 1;
        }
        if (i < s.size()) out.push_back(s.substr(i));
        return out;
    }

    std::string ParentPath(const std::string& path)
    {
        auto pos = path.find_last_of('/');
        return (pos == std::string::npos) ? std::string() : path.substr(0, pos);
    }

    std::string LastSeg(const std::string& path)
    {
        auto pos = path.find_last_of('/');
        return (pos == std::string::npos) ? path : path.substr(pos + 1);
    }

    HMENU EnsureMenu(const std::string& path)
    {
        auto it = g_menuByPath.find(path);
        if (it != g_menuByPath.end()) return it->second;

        const std::string parent = ParentPath(path);
        const std::string label  = LastSeg(path);

        HMENU parentMenu = parent.empty() ? g_bar : EnsureMenu(parent);
        HMENU sub = CreatePopupMenu();
        AppendMenuA(parentMenu, MF_POPUP, (UINT_PTR)sub, label.c_str());
        g_menuByPath[path] = sub;
        return sub;
    }

    int FindSubmenuIndex(HMENU parent, HMENU target)
    {
        int cnt = GetMenuItemCount(parent);
        for (int i = 0; i < cnt; ++i)
        {
            HMENU sm = GetSubMenu(parent, i);
            if (sm == target) return i;
        }
        return -1;
    }

    // Remove all maps with a given prefix ("Window/View" removes "Window/View" and its descendants)
    void EraseDescendants(const std::string& prefix)
    {
        const std::string pref = prefix + "/";

        for (auto it = g_menuByPath.begin(); it != g_menuByPath.end(); )
        {
            if (it->first == prefix || it->first.rfind(pref, 0) == 0)
            {
                it = g_menuByPath.erase(it);
            }
            else ++it;
        }

        for (auto it = g_itemIdByPath.begin(); it != g_itemIdByPath.end(); )
        {
            if (it->first == prefix || it->first.rfind(pref, 0) == 0)
            {
                g_cbById.erase(it->second);
                it = g_itemIdByPath.erase(it);
            }
            else ++it;
        }
    }

    LRESULT CALLBACK HookWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_COMMAND)
        {
            const int id = LOWORD(wParam);
            auto it = g_cbById.find(id);
            if (it != g_cbById.end())
            {
                it->second();
                return 0;
            }
        }
        return CallWindowProc(g_prevProc, hWnd, msg, wParam, lParam);
    }
}

void NativeMenu::Init(GLFWwindow* window)
{
    g_hwnd = glfwGetWin32Window(window);
    g_bar = CreateMenu();
    SetMenu(g_hwnd, g_bar);
    g_prevProc = (WNDPROC)SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, (LONG_PTR)HookWndProc);
}

void NativeMenu::Add(const std::string& path, std::function<void()> callback)
{
    auto parts = Split(path);
    if (parts.empty()) return;

    // Build submenu chain (all but last segment)
    std::string prefix;
    for (size_t i = 0; i + 1 < parts.size(); ++i)
    {
        prefix = prefix.empty() ? parts[i] : (prefix + "/" + parts[i]);
        EnsureMenu(prefix);
    }

    HMENU parent = (parts.size() == 1) ? g_bar : g_menuByPath[prefix];

    int id = g_nextId++;
    AppendMenuA(parent, MF_STRING, id, parts.back().c_str());
    g_itemIdByPath[path] = id;
    g_cbById[id] = std::move(callback);

    DrawMenuBar(g_hwnd);
}

void NativeMenu::DestroyMenu(const std::string& path)
{
    // Leaf item?
    if (auto it = g_itemIdByPath.find(path); it != g_itemIdByPath.end())
    {
        const int id = it->second;
        const std::string parentPath = ParentPath(path);
        HMENU parent = parentPath.empty() ? g_bar : g_menuByPath[parentPath];

        // Remove the command item and its callback.
        RemoveMenu(parent, id, MF_BYCOMMAND);
        g_cbById.erase(id);
        g_itemIdByPath.erase(it);

        DrawMenuBar(g_hwnd);
        return;
    }

    // Submenu?
    auto mit = g_menuByPath.find(path);
    if (mit != g_menuByPath.end())
    {
        HMENU submenu = mit->second;
        const std::string parentPath = ParentPath(path);
        HMENU parent = parentPath.empty() ? g_bar : g_menuByPath[parentPath];

        // Delete by position (need index)
        int idx = FindSubmenuIndex(parent, submenu);
        if (idx >= 0)
        {
            // First, recursively erase descendants from our maps
            EraseDescendants(path);

            // Remove UI and destroy the submenu handle
            RemoveMenu(parent, idx, MF_BYPOSITION);
            DestroyMenu(submenu);
            DrawMenuBar(g_hwnd);
        }
        return;
    }
    // If not found, no-op.
}

void NativeMenu::Shutdown(GLFWwindow* /*window*/)
{
    if (g_hwnd && g_prevProc)
    {
        SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, (LONG_PTR)g_prevProc);
        g_prevProc = nullptr;
    }
    if (g_hwnd)
    {
        SetMenu(g_hwnd, nullptr);
        g_hwnd = nullptr;
    }
    if (g_bar)
    {
        DestroyMenu(g_bar);
        g_bar = nullptr;
    }
    g_menuByPath.clear();
    g_itemIdByPath.clear();
    g_cbById.clear();
    g_nextId = 30000;
}
#endif