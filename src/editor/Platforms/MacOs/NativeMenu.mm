// native_menu_mac.mm
#import <Cocoa/Cocoa.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "NativeMenu.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>

using Callback = std::function<void()>;

// ---- Forward (file-local) bridge so Obj-C never references anonymous-namespace names directly.
static void NM_InvokeCallback(int id);

// ---- Obj-C class MUST be at global scope; use a unique name and selector to avoid collisions.
@interface NMFileScopedMenuTarget : NSObject
@end

@implementation NMFileScopedMenuTarget
- (void)nm_onMenu:(id)sender
{
    NSNumber* key = [sender representedObject];
    if (!key) return;
    NM_InvokeCallback(key.intValue);
}
@end

// ======================= C++ file-scoped state & helpers =======================
namespace
{
    NSWindow* g_NsWindow = nil;
    int g_NextId = 30000;

    // Maps (all file-local due to anonymous namespace)
    std::unordered_map<std::string, NSMenu*>     g_MenuByPath;  // "File" -> NSMenu*, "File/Recent" -> NSMenu*
    std::unordered_map<std::string, NSMenuItem*> g_ItemByPath;  // "File/Open" -> NSMenuItem*
    std::unordered_map<int, Callback>            g_CbById;      // ID -> callback

    NMFileScopedMenuTarget* g_Target = nil;

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

    NSMenu* EnsureMenu(const std::string& path)
    {
        if (auto it = g_MenuByPath.find(path); it != g_MenuByPath.end())
            return it->second;

        if (!NSApp) [NSApplication sharedApplication];

        // Ensure there is a main menubar with a basic App menu
        if (!NSApp.mainMenu)
        {
            NSMenu* menubar = [[NSMenu alloc] init];
            [NSApp setMainMenu:menubar];

            NSMenuItem* appItem = [[NSMenuItem alloc] init];
            [menubar addItem:appItem];
            NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@""];
            NSMenuItem* quitItem = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                                              action:@selector(terminate:)
                                                       keyEquivalent:@"q"];
            [appMenu addItem:quitItem];
            [appItem setSubmenu:appMenu];
        }

        std::string parent = ParentPath(path);
        std::string label  = LastSeg(path);

        if (parent.empty())
        {
            // Create a new top-level menu
            NSMenuItem* top = [[NSMenuItem alloc] init];
            [top setTitle:[NSString stringWithUTF8String:label.c_str()]];
            NSMenu* sub = [[NSMenu alloc] initWithTitle:[NSString stringWithUTF8String:label.c_str()]];
            [top setSubmenu:sub];
            [NSApp.mainMenu addItem:top];
            g_MenuByPath[path] = sub;
            return sub;
        }
        else
        {
            NSMenu* p = EnsureMenu(parent);

            // Try to find existing submenu item with this title
            for (NSMenuItem* mi in p.itemArray)
            {
                if ([[mi title] isEqualToString:[NSString stringWithUTF8String:label.c_str()]])
                {
                    if (mi.submenu)
                    {
                        g_MenuByPath[path] = mi.submenu;
                        return mi.submenu;
                    }
                }
            }
            // Create new submenu under parent
            NSMenuItem* container = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:label.c_str()]
                                                               action:nil
                                                        keyEquivalent:@""];
            NSMenu* sub = [[NSMenu alloc] initWithTitle:[NSString stringWithUTF8String:label.c_str()]];
            [container setSubmenu:sub];
            [p addItem:container];
            g_MenuByPath[path] = sub;
            return sub;
        }
    }

    // Remove all maps with a given prefix
    void EraseDescendants(const std::string& prefix)
    {
        const std::string pref = prefix + "/";

        for (auto it = g_MenuByPath.begin(); it != g_MenuByPath.end(); )
        {
            if (it->first == prefix || it->first.rfind(pref, 0) == 0)
                it = g_MenuByPath.erase(it);
            else
                ++it;
        }

        for (auto it = g_ItemByPath.begin(); it != g_ItemByPath.end(); )
        {
            if (it->first == prefix || it->first.rfind(pref, 0) == 0)
            {
                NSMenuItem* item = it->second;
                NSNumber* key = [item representedObject];
                if (key) g_CbById.erase(key.intValue);
                it = g_ItemByPath.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    NSMenuItem* FindChildItemByTitle(NSMenu* parent, const std::string& title)
    {
        NSString* t = [NSString stringWithUTF8String:title.c_str()];
        for (NSMenuItem* mi in parent.itemArray)
        {
            if ([[mi title] isEqualToString:t]) return mi;
        }
        return nil;
    }
} // anonymous namespace

// ---- File-local bridge implementation (can access anonymous-namespace state)
static void NM_InvokeCallback(int id)
{
    auto it = g_CbById.find(id);
    if (it != g_CbById.end())
        it->second();
}

// --- Purge default Cocoa/GLFW menus (file-local) ---

// Remove the top-level NSMenuItem hosting a given submenu
static void NM_RemoveTopLevelMenuForSubmenu(NSMenu* submenu)
{
    if (!submenu) return;
    NSMenu* menubar = NSApp.mainMenu;
    if (!menubar) return;

    // Copy the array to avoid mutation during enumeration
    NSArray<NSMenuItem*>* items = [[menubar itemArray] copy];
    for (NSMenuItem* mi in items)
    {
        if (mi.submenu == submenu)
        {
            [menubar removeItem:mi];
            break;
        }
    }
}

// Remove common default menus created by GLFW/Cocoa.
// keepAppMenu=true keeps the first "App" menu (recommended).
static void NM_ClearDefaultMenus(BOOL keepAppMenu)
{
    if (!NSApp) [NSApplication sharedApplication];

    // If GLFW created a menubar, ensure we have it.
    if (!NSApp.mainMenu)
    {
        NSMenu* menubar = [[NSMenu alloc] init];
        [NSApp setMainMenu:menubar];
    }

    // Optionally remove the App menu (the very first item).
    if (!keepAppMenu)
    {
        NSMenu* menubar = NSApp.mainMenu;
        if (menubar.numberOfItems > 0)
        {
            // Usually item 0 is the App menu
            [menubar removeItemAtIndex:0];
        }
    }

    // Remove default "Window" menu if present
    NSMenu* windowsMenu = [NSApp windowsMenu];
    if (windowsMenu)
    {
        NM_RemoveTopLevelMenuForSubmenu(windowsMenu);
        [NSApp setWindowsMenu:nil];
    }

    // Remove default "Help" menu if present
    NSMenu* helpMenu = [NSApp helpMenu];
    if (helpMenu)
    {
        NM_RemoveTopLevelMenuForSubmenu(helpMenu);
        [NSApp setHelpMenu:nil];
    }

    // Remove default "Services" menu if present (usually under App menu; also can be top-level in some setups)
    NSMenu* servicesMenu = [NSApp servicesMenu];
    if (servicesMenu)
    {
        // Try remove as a top-level first (rare)
        NM_RemoveTopLevelMenuForSubmenu(servicesMenu);

        // Also remove from the App menu if it exists there
        NSMenu* menubar = NSApp.mainMenu;
        if (menubar.numberOfItems > 0)
        {
            NSMenuItem* appItem = [menubar itemAtIndex:0];
            if (appItem.submenu == nil)
            {
                // Skip
            }
            else
            {
                NSMenu* appMenu = appItem.submenu;
                // Find and remove "Services" item by submenu identity
                NSArray<NSMenuItem*>* appItems = [[appMenu itemArray] copy];
                for (NSMenuItem* mi in appItems)
                {
                    if (mi.submenu == servicesMenu)
                    {
                        [appMenu removeItem:mi];
                        break;
                    }
                }
            }
        }

        [NSApp setServicesMenu:nil];
    }

    // Finally, remove any **empty** top-level menus that might be left over.
    NSMenu* menubar = NSApp.mainMenu;
    NSArray<NSMenuItem*>* items = [[menubar itemArray] copy];
    for (NSMenuItem* mi in items)
    {
        if (mi.submenu && mi.submenu.numberOfItems == 0)
        {
            [menubar removeItem:mi];
        }
    }
}


// ======================= Public NativeMenu methods =======================
void NativeMenu::Init(GLFWwindow* window)
{
    g_NsWindow = glfwGetCocoaWindow(window);
    if (!NSApp) [NSApplication sharedApplication];
    if (!g_Target) g_Target = [NMFileScopedMenuTarget new];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];

    NM_ClearDefaultMenus(YES);
}

void NativeMenu::Add(const std::string& path, std::function<void()> callback)
{
    if (!g_Target) g_Target = [NMFileScopedMenuTarget new];

    auto parts = Split(path);
    if (parts.empty()) return;

    // Build submenu chain (all but last)
    std::string prefix;
    for (size_t i = 0; i + 1 < parts.size(); ++i)
    {
        prefix = prefix.empty() ? parts[i] : (prefix + "/" + parts[i]);
        (void)EnsureMenu(prefix);
    }

    NSMenu* parent = (parts.size() == 1) ? EnsureMenu(parts[0]) : g_MenuByPath[prefix];

    int id = g_NextId++;
    g_CbById[id] = std::move(callback);

    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:parts.back().c_str()]
                                                  action:@selector(nm_onMenu:)
                                           keyEquivalent:@""];
    [item setTarget:g_Target];
    [item setRepresentedObject:@(id)];
    [parent addItem:item];

    g_ItemByPath[path] = item;
    (void)g_NsWindow; // reserved for optional toolbar usage
}

void NativeMenu::DestroyMenu(const std::string& path)
{
    // Leaf item?
    if (auto it = g_ItemByPath.find(path); it != g_ItemByPath.end())
    {
        NSMenuItem* item = it->second;
        NSMenu* parent = item.menu;
        if (parent)
        {
            NSNumber* key = [item representedObject];
            if (key) g_CbById.erase(key.intValue);
            [parent removeItem:item];
        }
        g_ItemByPath.erase(it);
        return;
    }

    // Submenu?
    if (auto mit = g_MenuByPath.find(path); mit != g_MenuByPath.end())
    {
        NSMenu* submenu = mit->second;
        std::string parentPath = ParentPath(path);
        NSMenu* parent = parentPath.empty() ? NSApp.mainMenu : g_MenuByPath[parentPath];

        // Find the NSMenuItem that owns this submenu by title and remove it
        NSMenuItem* holder = FindChildItemByTitle(parent, LastSeg(path));
        if (holder)
        {
            // Clear maps for subtree first
            EraseDescendants(path);
            [parent removeItem:holder];
        }
        return;
    }
    // Not found: no-op
}

void NativeMenu::Shutdown(GLFWwindow* /*window*/)
{
    // Keep NSApp/mainMenu; just clear our file-local state
    g_MenuByPath.clear();
    g_ItemByPath.clear();
    g_CbById.clear();
    g_NextId = 30000;
}
