#include "Cursor.h"
#include <Windows.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

static HWND    m_hwnd;
static HCURSOR m_arrow;
static HCURSOR m_ibeam;
static HCURSOR m_cross;
static HCURSOR m_hand;
static HCURSOR m_hresize;
static HCURSOR m_vresize;
static HCURSOR m_noDrop;

void Cursor::Initialize(void* glfwWindow)
{
	m_hwnd = glfwGetWin32Window(static_cast<GLFWwindow*>(glfwWindow));

	// Load standard cursors
	m_arrow = LoadCursor(NULL, IDC_ARROW);
	m_ibeam = LoadCursor(NULL, IDC_IBEAM);
	m_cross = LoadCursor(NULL, IDC_CROSS);
	m_hand = LoadCursor(NULL, IDC_HAND);
	m_hresize = LoadCursor(NULL, IDC_SIZEWE);
	m_vresize = LoadCursor(NULL, IDC_SIZENS);
	m_noDrop = LoadCursor(NULL, IDC_NO);
}

void Cursor::SetMode(CursorMode mode)
{
	// Ensure cursor clip is reset by default
	RECT clipRect;
	bool doClip = false;

	switch (mode)
	{
	case CursorMode::Normal:
		ShowCursor(TRUE);
		SetCursor(m_arrow);
		break;

	case CursorMode::Hidden:
		ShowCursor(FALSE);
		break;

	case CursorMode::Disabled:
		ShowCursor(FALSE);
		// Clip to client area
		{
			RECT rc;
			GetClientRect(m_hwnd, &rc);
			POINT ul = { rc.left, rc.top };
			POINT lr = { rc.right, rc.bottom };
			ClientToScreen(m_hwnd, &ul);
			ClientToScreen(m_hwnd, &lr);
			clipRect.left = ul.x;
			clipRect.top = ul.y;
			clipRect.right = lr.x;
			clipRect.bottom = lr.y;
			doClip = true;
		}
		break;

	case CursorMode::Arrow:
		ShowCursor(TRUE);
		SetCursor(m_arrow);
		break;

	case CursorMode::IBeam:
		ShowCursor(TRUE);
		SetCursor(m_ibeam);
		break;

	case CursorMode::Crosshair:
		ShowCursor(TRUE);
		SetCursor(m_cross);
		break;

	case CursorMode::Hand:
		ShowCursor(TRUE);
		SetCursor(m_hand);
		break;

	case CursorMode::HResize:
		ShowCursor(TRUE);
		SetCursor(m_hresize);
		break;

	case CursorMode::VResize:
		ShowCursor(TRUE);
		SetCursor(m_vresize);
		break;

	case CursorMode::NoDrop:
		ShowCursor(TRUE);
		SetCursor(m_noDrop);
		break;
	}

	// Apply clipping or release
	if (doClip) {
		ClipCursor(&clipRect);
	}
	else {
		ClipCursor(NULL);
	}
}
