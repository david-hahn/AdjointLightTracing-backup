#include <tamashii/core/common/window_thread.hpp>
#include <atomic>
T_USE_NAMESPACE

WindowThread::WindowThread() : mShowWindow(false), mHideWindow(false), mGrabMouse(false),
                               mUngrabMouse(false), mShutdown(false) {}

WindowThread::~WindowThread() = default;

void WindowThread::spawn(const std::function<void(Window&)>& aFunction)
{
	std::atomic_bool done = false;
	mThread = std::thread([&]() {
		aFunction(mWindow);
		done.store(true);
		if(!mWindow.isWindow())
		{
			spdlog::error("Window not initialized");
			return;
		}
		run();
	});
	while(!done.load()) {}
}

void WindowThread::frame()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((1.0f / 144.0f) * 1000.0f)));
	mWindow.readEvents();
	if (mGrabMouse)
	{
		mWindow.grabMouse();
		mGrabMouse = false;
	}
	if (mUngrabMouse)
	{
		mWindow.ungrabMouse();
		mUngrabMouse = false;
	}
	if (mShowWindow)
	{
		mWindow.showWindow(Window::Show::yes);
		mShowWindow = false;
	}
	if (mHideWindow)
	{
		mWindow.showWindow(Window::Show::no);
		mHideWindow = false;
	}
	if (mShutdown)
	{
		mWindow.destroy();
		mShutdown = false;
	}
}

void WindowThread::run()
{
	bool run = true;
	while (run)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((1.0f / 144.0f) * 1000.0f)));
		mWindow.readEvents();
		if (mGrabMouse)
		{
			mWindow.grabMouse();
			mGrabMouse = false;
		}
		if (mUngrabMouse)
		{
			mWindow.ungrabMouse();
			mUngrabMouse = false;
		}
		if (mShowWindow)
		{
			mWindow.showWindow(Window::Show::yes);
			mShowWindow = false;
		}
		if (mHideWindow)
		{
			mWindow.showWindow(Window::Show::no);
			mHideWindow = false;
		}
		if (mShutdown)
		{
			mWindow.destroy();
			mShutdown = false;
			run = false;
			return;
		}
	}
}

bool WindowThread::running() const
{
	return mThread.joinable();
}

void WindowThread::shutdown()
{
	{
		const std::lock_guard lock(mMutex);
		mShutdown = true;
	}
	if(mThread.joinable()) mThread.join();
	mShutdown = false;
}

std::mutex& WindowThread::mutex()
{
	return mMutex;
}

Window* WindowThread::window()
{
	return &mWindow;
}

bool WindowThread::isWindow()
{
	const std::lock_guard lock(mMutex);
	return mWindow.isWindow();
}

bool WindowThread::isMinimized()
{
	const std::lock_guard lock(mMutex);
	return mWindow.isMinimized();
}

bool WindowThread::isMaximized()
{
	const std::lock_guard lock(mMutex);
	return mWindow.isMaximized();
}

void WindowThread::showWindow(const bool aShow)
{
	if (aShow) mShowWindow = true;
	else mHideWindow = true;
}

void WindowThread::setSize(const glm::ivec2 aSize)
{
	const std::lock_guard lock(mMutex);
	if (!mWindow.isWindow()) return;
	mWindow.setSize(aSize);
}

glm::ivec2 WindowThread::getSize()
{
	const std::lock_guard lock(mMutex);
	return mWindow.getSize();
}

glm::ivec2 WindowThread::getPosition()
{
	const std::lock_guard lock(mMutex);
	return mWindow.getPosition();
}

void WindowThread::grabMouse()
{
	mGrabMouse = true;
}

void WindowThread::ungrabMouse()
{
	mUngrabMouse = true;
}

bool WindowThread::isMouseGrabbed()
{
	const std::lock_guard lock(mMutex);
	return mWindow.isMouseGrabbed();
}
