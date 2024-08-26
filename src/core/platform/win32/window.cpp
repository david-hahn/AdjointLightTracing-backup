#include <tamashii/core/platform/window.hpp>
#include <tamashii/core/common/input.hpp>
#include <windowsx.h>
#include <shellapi.h>
#include <imgui.h>
#include <imgui_impl_win32.h>

#include <filesystem>
#include <optional>

T_USE_NAMESPACE

std::unordered_map<HWND, Window*> Window::mWindowList;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Window* window = Window::getWindow(hWindow);
	if (!window) return DefWindowProc(hWindow, uMsg, wParam, lParam);
	bool imguiReadsMouse = false;
	bool imguiReadsKeyboard = false;
	static std::optional<std::lock_guard<std::mutex>> lock;
	{
		
		if (ImGui_ImplWin32_WndProcHandler(hWindow, uMsg, wParam, lParam))
			return true;
	}
	if (ImGui::GetCurrentContext()) {
		const ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureMouse) imguiReadsMouse = true;
		if (io.WantCaptureKeyboard) imguiReadsKeyboard = true;

		
		if(uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP) imguiReadsMouse = false;
	}

	if (window && window->isMouseGrabbed()) {
		const glm::ivec2 p = window->getCenter();
		SetCursorPos(p.x, p.y);
	}
	switch (uMsg)
	{
	case WM_CREATE:
		return 0;
	case WM_GETMINMAXINFO:	
		{
			const auto lpMMI = reinterpret_cast<LPMINMAXINFO>(lParam);
			lpMMI->ptMinTrackSize.x = 500;
			lpMMI->ptMinTrackSize.y = 500;
		}
		return 0;
	case WM_PAINT:
		PAINTSTRUCT ps;
		BeginPaint(hWindow, &ps);
		
		EndPaint(hWindow, &ps);
		return 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_TIMER:
		
		
		return 0;
	case WM_SYSCOMMAND: 
	{
		const UINT sc = GET_SC_WPARAM(wParam);
		if (sc == SC_MINIMIZE) {
			if (!lock.has_value()) lock.emplace(window->resizeMutex());
			window->isMinimized(true);
		}
		else if (sc == SC_SIZE) {
			if (!lock.has_value()) lock.emplace(window->resizeMutex());
			window->isResizing(true);
		}
		else if (sc == SC_MOVE) {
			window->isMoving(true);
		}
		else if (sc == SC_MAXIMIZE) {
			window->isMaximized(true);
		}
		return DefWindowProc(hWindow, uMsg, wParam, lParam);
	}
	case WM_EXITSIZEMOVE:
		if (window->isResizing()) lock.reset();
		window->isResizing(false);
		window->isMoving(false);
		return 0;
	case WM_SIZE:
	{
		const UINT width = LOWORD(lParam);
		const UINT height = HIWORD(lParam);
		if (wParam == SIZE_RESTORED) {
			window->setSize({ width, height });
			window->isMaximized(false);
			if (window->isMinimized()) {
				lock.reset();
				window->isMinimized(false);
			}
		}
		else if (wParam == SIZE_MAXIMIZED) {
			window->setSize({ width, height });
			if (window->isMinimized()) {
				lock.reset();
				window->isMinimized(false);
			}
		}
		return 0;
	}
	case WM_MOVE:
	{
		int x = static_cast<int>(static_cast<short>(LOWORD(lParam))) - 8;
		int y = static_cast<int>(static_cast<short>(HIWORD(lParam))) - 31;
		window->setPosition({ x, y });
		return 0;
	}
	case WM_DROPFILES:
		{
			const int nFiles = DragQueryFile(reinterpret_cast<HDROP>(wParam), 0xFFFFFFFF, nullptr, 0);
			if (nFiles > 1) spdlog::warn("Please drop only one file.");
			else {
				char szFile[FILENAME_MAX] = "";
				DragQueryFile(reinterpret_cast<HDROP>(wParam), 0, szFile, FILENAME_MAX);

				const std::filesystem::path path(szFile);
				std::string ext = path.extension().string();
				std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
				if (ext == ".gltf" || ext == ".glb" || ext == ".pbrt" || ext == ".bsp") {
					EventSystem::queueEvent(EventType::ACTION, Input::A_OPEN_SCENE, 0, 0, 0, path.string());
				}
				else if (ext == ".ies" || ext == ".ldt") {
					EventSystem::queueEvent(EventType::ACTION, Input::A_ADD_LIGHT, 0, 0, 0, path.string());
				}
				else if (ext == ".ply" || ext == ".obj") {
					EventSystem::queueEvent(EventType::ACTION, Input::A_ADD_MODEL, 0, 0, 0, path.string());
				} else spdlog::warn("...format not supported");
			}
			DragFinish(reinterpret_cast<HDROP>(wParam));
		}
		return 0;
	case WM_CLOSE:
		EventSystem::queueEvent(EventType::ACTION, Input::A_EXIT);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	
	case WM_KEYDOWN:
		{
			const auto key = static_cast<Input>(MapVirtualKey(wParam, MAPVK_VK_TO_VSC));
			EventSystem::queueEvent(EventType::KEY, key, true, 0, 0);
		}
		return 0;
	case WM_KEYUP:
		{
			const auto key = static_cast<Input>(MapVirtualKey(wParam, MAPVK_VK_TO_VSC));
			EventSystem::queueEvent(EventType::KEY, key, false, 0, 0);
		}
		return 0;
	
	case WM_LBUTTONDOWN:
		if(imguiReadsMouse || GET_X_LPARAM(lParam) < 0 || GET_Y_LPARAM(lParam) < 0) break;
		EventSystem::queueEvent(EventType::KEY, Input::MOUSE_LEFT, true, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
		if(imguiReadsMouse || GET_X_LPARAM(lParam) < 0 || GET_Y_LPARAM(lParam) < 0) break;
		EventSystem::queueEvent(EventType::KEY, Input::MOUSE_LEFT, false, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MBUTTONDOWN:
		if (imguiReadsMouse || GET_X_LPARAM(lParam) < 0 || GET_Y_LPARAM(lParam) < 0) break;
		EventSystem::queueEvent(EventType::KEY, Input::MOUSE_WHEEL, true, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MBUTTONUP:
		if (imguiReadsMouse || GET_X_LPARAM(lParam) < 0 || GET_Y_LPARAM(lParam) < 0) break;
		EventSystem::queueEvent(EventType::KEY, Input::MOUSE_WHEEL, false, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_RBUTTONDOWN:
		if (imguiReadsMouse || GET_X_LPARAM(lParam) < 0 || GET_Y_LPARAM(lParam) < 0) break;
		EventSystem::queueEvent(EventType::KEY, Input::MOUSE_RIGHT, true, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_RBUTTONUP:
		if (imguiReadsMouse || GET_X_LPARAM(lParam) < 0 || GET_Y_LPARAM(lParam) < 0) break;
		EventSystem::queueEvent(EventType::KEY, Input::MOUSE_RIGHT, false, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEWHEEL: 
		if (imguiReadsMouse) break;
		{
			const int fwKeys = GET_KEYSTATE_WPARAM(wParam);
			const int zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			EventSystem::queueEvent(EventType::MOUSE_WHEEL_DELTA, Input::NONE, 0, 0, static_cast<float>(zDelta), "");
		}
		return 0;
	case WM_MOUSEMOVE:
		EventSystem::queueEvent(EventType::MOUSE_ABSOLUTE, Input::NONE, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		{
			static int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			EventSystem::queueEvent(EventType::MOUSE_DELTA, Input::NONE, 0, static_cast<float>(GET_X_LPARAM(lParam) - x), static_cast<float>(y - GET_Y_LPARAM(lParam)));
			x = GET_X_LPARAM(lParam);
			y = GET_Y_LPARAM(lParam);
			
			TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, window->getWindowHandle(), 0 };
			TrackMouseEvent(&tme);
		}
		return 0;
	case WM_MOUSELEAVE:
		EventSystem::queueEvent(EventType::MOUSE_LEAVE, Input::NONE, 0, 0, 0);
		EventSystem::queueEvent(EventType::KEY, Input::MOUSE_LEFT, false);
        EventSystem::queueEvent(EventType::KEY, Input::MOUSE_WHEEL, false);
        EventSystem::queueEvent(EventType::KEY, Input::MOUSE_RIGHT, false);
		return 0;
	case WM_INPUT:
	
	{	
		if (imguiReadsMouse) return 0;
		UINT dwSize = sizeof(RAWINPUT);
		static BYTE lpb[sizeof(RAWINPUT)];
		GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
		const auto raw = reinterpret_cast<RAWINPUT*>(lpb);
		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			/*if (((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE)) {
				int zx = 1;
			}
			if (((raw->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) == RI_MOUSE_LEFT_BUTTON_DOWN)) {
				EventSystem::queueEvent(tEventType::KEY, tInput::MOUSE_LEFT, true, raw->data.mouse.lLastX, raw->data.mouse.lLastY);
			}*/
			const int xPosRelative = raw->data.mouse.lLastX;
			const int yPosRelative = raw->data.mouse.lLastY;
			int zPosRelative = 0;
			if (raw->data.mouse.usButtonFlags == RI_MOUSE_WHEEL) {
				zPosRelative = static_cast<short>(raw->data.mouse.usButtonData) / WHEEL_DELTA;
			}
			EventSystem::queueEvent(EventType::MOUSE_DELTA, Input::NONE, 0, static_cast<float>(xPosRelative), static_cast<float>(yPosRelative));
			EventSystem::queueEvent(EventType::MOUSE_WHEEL_DELTA, Input::NONE, 0, 0, static_cast<float>(zPosRelative));
		}
		else if (raw->header.dwType == RIM_TYPEKEYBOARD) {
			if (raw->data.keyboard.Flags == RI_KEY_MAKE) EventSystem::queueEvent(EventType::KEY, static_cast<Input>(raw->data.keyboard.MakeCode), true, 0, 0);
			else if (raw->data.keyboard.Flags == RI_KEY_BREAK) EventSystem::queueEvent(EventType::KEY, static_cast<Input>(raw->data.keyboard.MakeCode), false, 0, 0);
		}
		return 0;
	}
	default:
		return DefWindowProc(hWindow, uMsg, wParam, lParam);
	}
	return 0;
}

Window::~Window() = default;

bool Window::init(const char* aName, const glm::ivec2 aSize, const glm::ivec2 aPosition)
{
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = mHInstance = nullptr;
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = aName;
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

	ATOM reg = RegisterClassEx(&wc);
	DWORD err = GetLastError();
	if (!reg && !(err == ERROR_CLASS_ALREADY_EXISTS)) {
		spdlog::error("Failed to register extended window class: ");
	}

	constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW;

	RECT winRect;
	winRect.left = 0;
	winRect.right = aSize.x;
	winRect.top = 0;
	winRect.bottom = aSize.y;
	AdjustWindowRectEx(&winRect, windowStyle, FALSE, wc.style);

	
	RECT rect;
	GetClientRect(GetDesktopWindow(), &rect);
	mPosition.x = aPosition.x == 0 ? (rect.right / 3) : aPosition.x;
	mPosition.y = aPosition.y == 0 ? (rect.bottom / 14) : aPosition.y;

	int w = winRect.right - winRect.left;
	int h = winRect.bottom - winRect.top;
	mHWnd = CreateWindowEx(
		wc.style,
		aName,
		aName,
		windowStyle,
		mPosition.x, mPosition.y,
		w, h,
		nullptr,
		nullptr,
		mHInstance,
		nullptr
	);
	if (!mHWnd)
	{
		spdlog::error("Could not create window");
	}
	RECT clientArea;
	GetClientRect(mHWnd, &clientArea);
	mSize = { clientArea.right, clientArea.bottom };
	mWindowList.insert(std::pair(mHWnd, this));

	spdlog::info("Created window of size ({}x{}) at ({},{})", w, h, mPosition.x, mPosition.y);
	DragAcceptFiles(mHWnd, true);
	SetForegroundWindow(mHWnd);
	SetFocus(mHWnd);

	return true;
}

bool Window::initWithOpenGlContex(const char* aName, glm::ivec2 aSize, glm::ivec2 aPosition)
{
	init(aName, aSize, aPosition);

	HDC mHdc = GetDC(mHWnd);
	PIXELFORMATDESCRIPTOR pfd = { sizeof(pfd), 1 };
	if (mHdc) {
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_SUPPORT_COMPOSITION | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cAlphaBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;
		const auto formatIndex = ::ChoosePixelFormat(mHdc, &pfd);
		if (!formatIndex) {
			spdlog::error("Could not attach context");
			return false;
		}

		if (!::SetPixelFormat(mHdc, formatIndex, &pfd)) {
			spdlog::error("Could not attach context");
			return false;
		}
	}

	const auto activeFormatIndex = ::GetPixelFormat(mHdc);
	if (!activeFormatIndex) {
		spdlog::error("Could not attach context");
		return false;
	}

	if (!::DescribePixelFormat(mHdc, activeFormatIndex, sizeof pfd, &pfd)) {
		spdlog::error("Could not attach context");
		return false;
	}

	if ((pfd.dwFlags & PFD_SUPPORT_OPENGL) != PFD_SUPPORT_OPENGL) {
		spdlog::error("Could not attach context");
		return false;
	}
	return false;
}

bool Window::destroy()
{
	ungrabMouse();
	mWindowList.erase(mHWnd);

	DragAcceptFiles(mHWnd, false);
	ShowWindow(mHWnd, SW_HIDE);
	DestroyWindow(mHWnd);
	mHWnd = nullptr;
	return true;
}

bool Window::isWindow() const
{
	return IsWindow(mHWnd);
}

void Window::showWindow(const Show aShow)
{
	if(aShow == Show::no) ShowWindow(mHWnd, SW_HIDE);
	else if (aShow == Show::yes) ShowWindow(mHWnd, SW_SHOW);
	else if (aShow == Show::max) {
		isMaximized(true);
		ShowWindow(mHWnd, SW_SHOWMAXIMIZED);
	}
}

void Window::grabMouse()
{
	if (mMouseGrabbed) return;
	RAWINPUTDEVICE rid[2];

	rid[0].usUsagePage = 0x01;
	rid[0].usUsage = 0x02;
	rid[0].dwFlags = RIDEV_NOLEGACY;   
	rid[0].hwndTarget = mHWnd;

	rid[1].usUsagePage = 0x01;
	rid[1].usUsage = 0x06;
	rid[1].dwFlags = RIDEV_NOLEGACY;   
	rid[1].hwndTarget = mHWnd;

	if (RegisterRawInputDevices(rid, 2, sizeof(rid[0])) == FALSE) {
		
	}
	mMouseGrabbed = true;
	ShowCursor(false);
	
}

void Window::ungrabMouse()
{
	if (!mMouseGrabbed) return;
	ShowCursor(true);
	RAWINPUTDEVICE rid[2];

	rid[0].usUsagePage = 0x01;
	rid[0].usUsage = 0x02;
	rid[0].dwFlags = RIDEV_REMOVE;   
	rid[0].hwndTarget = nullptr;

	rid[1].usUsagePage = 0x01;
	rid[1].usUsage = 0x06;
	rid[1].dwFlags = RIDEV_REMOVE;   
	rid[1].hwndTarget = nullptr;
	if (RegisterRawInputDevices(rid, 2, sizeof(rid[0])) == FALSE) {
		
	}
	mMouseGrabbed = false;
}

glm::vec2 Window::getCenter() const
{
	RECT r;
	GetClientRect(mHWnd, &r);
	MapWindowPoints(mHWnd, GetParent(mHWnd), reinterpret_cast<LPPOINT>(&r), 2);
	const int width = r.right - r.left;
	const int height = r.bottom - r.top;
	return { r.left + width / 2, r.top + height / 2 };
}

void Window::setSize(const glm::ivec2 aSize)
{
	mSize = aSize;
}

glm::ivec2 Window::getSize() const
{
	return mSize;
}

HWND Window::getWindowHandle()
{
	return isWindow() ? mHWnd : nullptr;
}

HINSTANCE Window::getInstanceHandle()
{
	return isWindow() ? mHInstance : nullptr;
}

Window* Window::getWindow(HWND const aHandle)
{
	const auto search = mWindowList.find(aHandle);
	if (search != mWindowList.end()) return search->second;
	return nullptr;
}

void Window::readEvents() {
	if (!isWindow()) return;
	MSG msg;
	while (PeekMessage(&msg, mHWnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
