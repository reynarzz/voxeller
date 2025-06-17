#ifdef _WIN32


#include "DropHoverEvents.h"
#include <windows.h>
#include <shellapi.h>

DropHoverEvents::DropCallback DropHoverEvents::dropCallback = nullptr;
DropHoverEvents::HoverCallback DropHoverEvents::hoverCallback = nullptr;

static WNDPROC originalWndProc = nullptr;
static HWND hwnd = nullptr;

static int lastX = -1;
static int lastY = -1;

LRESULT CALLBACK CustomWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            UINT count = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0);
            std::vector<std::string> paths;
            for (UINT i = 0; i < count; ++i) {
                char path[MAX_PATH];
                DragQueryFileA(hDrop, i, path, MAX_PATH);
                paths.emplace_back(path);
            }
            POINT pt;
            DragQueryPoint(hDrop, &pt);
            if (DropHoverEvents::dropCallback) {
                DropHoverEvents::dropCallback({ paths, pt.x, pt.y });
            }
            DragFinish(hDrop);
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (DropHoverEvents::hoverCallback) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                if (pt.x != lastX || pt.y != lastY) {
                    lastX = pt.x;
                    lastY = pt.y;
                    DropHoverEvents::hoverCallback({ pt.x, pt.y });
                }
            }
            break;
        }
    }
    return CallWindowProc(originalWndProc, hWnd, msg, wParam, lParam);
}

void DropHoverEvents::Initialize(void* nativeWindowHandle) {
    hwnd = static_cast<HWND>(nativeWindowHandle);
    DragAcceptFiles(hwnd, TRUE);
    originalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)CustomWndProc);
}

void DropHoverEvents::Shutdown() {
    if (hwnd && originalWndProc) {
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)originalWndProc);
        originalWndProc = nullptr;
        hwnd = nullptr;
    }
    lastX = -1;
    lastY = -1;
}

void DropHoverEvents::SetDropCallback(DropCallback cb) {
    dropCallback = cb;
}

void DropHoverEvents::SetHoverCallback(HoverCallback cb) {
    hoverCallback = cb;
}

#endif