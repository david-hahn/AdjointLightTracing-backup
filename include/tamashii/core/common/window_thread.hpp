#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/platform/window.hpp>
#include <mutex>
#include <thread>

T_BEGIN_NAMESPACE
class WindowThread
{
public:
					WindowThread();
					~WindowThread();
	void			spawn(const std::function<void(Window&)>& aFunction);
	void			frame();
	void			run();
	bool			running() const;
	void			shutdown();
	std::mutex&		mutex();
	Window*			window();

	bool			isWindow();
	bool			isMinimized();
	bool			isMaximized();
	void			showWindow(bool aShow);
	void            setSize(glm::ivec2 aSize);
	glm::ivec2      getSize();
	glm::ivec2      getPosition();
	void			grabMouse();
	void			ungrabMouse();
	bool			isMouseGrabbed();
private:
	Window			mWindow;
	std::thread		mThread;
	std::mutex		mMutex;

	bool			mShowWindow;
	bool			mHideWindow;
	bool			mGrabMouse;
	bool			mUngrabMouse;
	bool			mShutdown;
};
T_END_NAMESPACE
