#pragma once
#include <tamashii/public.hpp>

#if defined( _WIN32 )
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#elif defined(__APPLE__)
@class NSView;
#elif defined(__linux__)

#if defined( VK_USE_PLATFORM_WAYLAND_KHR )
#elif defined( VK_USE_PLATFORM_XCB_KHR )
#include <xcb/xcb.h>
#endif

#endif

T_BEGIN_NAMESPACE
class Window {
public:
	enum class Show { no, yes, max };

						Window() : mResizing(false), mMoving(false), mMinimized(false), mMaximized(false), mMouseGrabbed(false), mSize({}), mPosition({}) {}
						~Window();

	bool				init(const char* aName, glm::ivec2 aSize, glm::ivec2 aPosition);
	bool				initWithOpenGlContex(const char* aName, glm::ivec2 aSize, glm::ivec2 aPosition);
	bool				destroy();
	bool				isWindow() const;
    void				showWindow(Show aShow);

	bool				isResizing() const { return mResizing; }
	bool				isMoving() const { return mMoving; }
	bool				isMinimized() const { return mMinimized; }
	bool				isMaximized() const { return mMaximized; }
	bool				isMouseGrabbed() const { return mMouseGrabbed; }
	void				isResizing(const bool aResizing) { mResizing = aResizing; }
	void				isMoving(const bool aMoving) { mMoving = aMoving; }
	void				isMinimized(const bool aMinimized) { mMinimized = aMinimized; }
	void				isMaximized(const bool aMaximize) { mMaximized = aMaximize; }

	void				grabMouse();
	void				ungrabMouse();

    glm::vec2			getCenter() const;
    void                setSize(glm::ivec2 aSize);
    glm::ivec2          getSize() const;
	void                setPosition(const glm::ivec2 aPosition) { mPosition = aPosition; }
	glm::ivec2          getPosition() const { return mPosition; }
	std::mutex&			resizeMutex() { return mResizeMutex; }

/*
**			- Window Handle -	- Instance Handle -
** win:			HWND,				HINSTANCE
** macOS:		NSView,				nullptr
** wayland:		wl_surface,			wl_display
** xcb:			xcb_window_t,		xcb_connection_t
** X11:			Window,				Display
*/
#if defined(_WIN32)
#define WINDOW_HANDLE HWND
#define INSTANCE_HANDLE HINSTANCE
#elif defined(__APPLE__)
#define WINDOW_HANDLE NSView*
#define INSTANCE_HANDLE void*
#elif defined(__linux__)
#if defined( VK_USE_PLATFORM_WAYLAND_KHR )
#elif defined( VK_USE_PLATFORM_XCB_KHR )
#define WINDOW_HANDLE xcb_window_t*
#define INSTANCE_HANDLE xcb_connection_t*
    xcb_intern_atom_reply_t *getCloseAtom() { return mAtomWmDeleteWindow; };
#endif
#endif
    WINDOW_HANDLE		getWindowHandle();
    INSTANCE_HANDLE		getInstanceHandle();
    static Window*		getWindow(WINDOW_HANDLE aHandle);
#undef WINDOW_HANDLE
#undef INSTANCE_HANDLE

	void				readEvents();
private:
	bool				mResizing;
	bool				mMoving;
	bool				mMinimized;
	bool				mMaximized;
	bool				mMouseGrabbed;
    glm::ivec2          mSize;
    glm::ivec2          mPosition;
	std::mutex			mResizeMutex;

#if defined(_WIN32)
	HWND				mHWnd = nullptr;				
	HINSTANCE			mHInstance = nullptr;
    static std::unordered_map<HWND, Window*> mWindowList;
#elif defined(__APPLE__)
    void*               mWindow = nullptr;                
    void*               mInstance = nullptr;
    static std::unordered_map<NSView*, Window*> mWindowList;
#elif defined(__linux__)

#if defined( VK_USE_PLATFORM_WAYLAND_KHR )
#elif defined( VK_USE_PLATFORM_XCB_KHR )
    xcb_connection_t    *mConnection = nullptr;
    xcb_window_t        mWindow;
    xcb_screen_t        *mScreen = nullptr;
    xcb_intern_atom_reply_t *mAtomWmDeleteWindow;
    static std::unordered_map<xcb_window_t, Window*> mWindowList;
#endif

#endif
};
T_END_NAMESPACE
