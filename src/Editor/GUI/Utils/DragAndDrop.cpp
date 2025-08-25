#ifdef _WIN32

#include "DropHoverEvents.h"
#include <windows.h>
#include <shlobj_core.h>
#include <objidl.h>
#include <oleidl.h>
#include <combaseapi.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// Static callback storage
DragAndDrop::DropCallback DragAndDrop::dropCallback = nullptr;
DragAndDrop::HoverCallback DragAndDrop::hoverCallback = nullptr;

static HWND        g_hwnd = nullptr;
static IDropTarget* g_dropTarget = nullptr;
static LONG        g_refCount = 1;
static int         g_lastX = -1, g_lastY = -1;

//-----------------------------------------------------------------------------
// COM-based IDropTarget implementation
class DropTargetImpl : public IDropTarget {
public:
	// IUnknown
	HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override
	{
		if (!ppv) return E_POINTER;
		if (riid == IID_IUnknown || riid == IID_IDropTarget)
		{
			*ppv = static_cast<IDropTarget*>(this);
			AddRef();
			return S_OK;
		}
		*ppv = nullptr;
		return E_NOINTERFACE;
	}
	ULONG __stdcall AddRef() override
	{
		return InterlockedIncrement(&g_refCount);
	}
	ULONG __stdcall Release() override
	{
		LONG c = InterlockedDecrement(&g_refCount);
		if (c == 0) delete this;
		return c;
	}

	// IDropTarget
	HRESULT __stdcall DragEnter(IDataObject* /*pDataObj*/, DWORD /*key*/, POINTL pt, DWORD* effect) override
	{
		g_lastX = g_lastY = -1;
		return DragOver(0, pt, effect);
	}
	HRESULT __stdcall DragOver(DWORD /*key*/, POINTL pt, DWORD* effect) override
	{
		if (DragAndDrop::hoverCallback && (pt.x != g_lastX || pt.y != g_lastY))
		{
			g_lastX = pt.x;
			g_lastY = pt.y;
			DragAndDrop::hoverCallback({ pt.x, pt.y });
		}
		*effect = DROPEFFECT_COPY;

		return S_OK;
	}
	HRESULT __stdcall DragLeave() override
	{
		g_lastX = g_lastY = -1;
		return S_OK;
	}
	HRESULT __stdcall Drop(IDataObject* pDataObj, DWORD /*key*/, POINTL pt, DWORD* effect) override
	{
		FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		STGMEDIUM stg = {};
		if (FAILED(pDataObj->GetData(&fmt, &stg))) return E_FAIL;

		HDROP hDrop = static_cast<HDROP>(GlobalLock(stg.hGlobal));
		if (!hDrop) { ReleaseStgMedium(&stg); return E_FAIL; }

		UINT count = DragQueryFileA(hDrop, 0xFFFFFFFF, nullptr, 0);
		std::vector<std::string> paths;
		paths.reserve(count);
		char buf[MAX_PATH];
		for (UINT i = 0; i < count; ++i) {
			DragQueryFileA(hDrop, i, buf, MAX_PATH);
			paths.emplace_back(buf);
		}

		GlobalUnlock(stg.hGlobal);
		ReleaseStgMedium(&stg);

		if (DragAndDrop::dropCallback) {
			DragAndDrop::dropCallback({ paths, pt.x, pt.y });
		}
		*effect = DROPEFFECT_COPY;
		return S_OK;
	}
};

//-----------------------------------------------------------------------------
void DragAndDrop::Initialize(void* glfwWindow)
{
	g_hwnd = glfwGetWin32Window(static_cast<GLFWwindow*>(glfwWindow));
	if (!g_hwnd) return;

	// Initialize COM for OLE drag & drop
	if (FAILED(OleInitialize(nullptr))) {
		OutputDebugStringA("OleInitialize failed\n");
		return;
	}

	// Register drop target
	g_dropTarget = new DropTargetImpl();
	RegisterDragDrop(g_hwnd, g_dropTarget);
}

void DragAndDrop::Shutdown()
{
	if (g_hwnd && g_dropTarget)
	{
		RevokeDragDrop(g_hwnd);
		g_dropTarget->Release();
		g_dropTarget = nullptr;
	}
	OleUninitialize();
	g_hwnd = nullptr;
	g_lastX = g_lastY = -1;
}

void DragAndDrop::SetDropCallback(DropCallback cb)
{
	dropCallback = std::move(cb);
}

void DragAndDrop::SetHoverCallback(HoverCallback cb)
{
	hoverCallback = std::move(cb);
}

#endif // _WIN32
