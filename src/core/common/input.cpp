#include <tamashii/core/common/input.hpp>
#include <tamashii/core/common/common.hpp>

T_USE_NAMESPACE

bool InputSystem::wasPressed(Input aInput) const
{
	if (!inBounds<Input>(Input::NONE, Input::LAST_KEY, aInput)) return false;
	return mKeys[static_cast<int>(aInput)].mPressed;
}
bool InputSystem::isDown(Input aInput) const
{
	if (!inBounds<Input>(Input::NONE, Input::LAST_KEY, aInput)) return false;
	return mKeys[static_cast<int>(aInput)].mDown;
}
bool InputSystem::wasReleased(Input aInput) const
{
	if (!inBounds<Input>(Input::NONE, Input::LAST_KEY, aInput)) return false;
	return mKeys[static_cast<int>(aInput)].mReleased;
}

glm::vec2 InputSystem::getMousePosAbsolute() const
{
	return mMousePosAbsolute;
}

glm::vec2 InputSystem::getMousePosRelative() const
{
	return mMousePosRelative;
}

glm::vec2 InputSystem::getMouseWheelRelative() const
{
	return mMouseWheelRelative;
}

void EventSystem::queueEvent(const EventType aType, const Input aInput, const int aValue, const float aX, const float aY, std::string_view aString) {
	EventSystem& es = getInstance();
	
	const std::lock_guard lock(es.mMutex);
	Event* ev = &es.mEventQue[es.mEventHead & MASK_QUED_EVENTS];

	if (es.mEventHead - es.mEventTail >= MAX_QUED_EVENTS) {
		
		
		es.mEventTail++;
	}
	es.mEventHead++;

	ev->mType = aType;
	ev->mInput = aInput;
	ev->mValue = aValue;
	ev->mX = aX;
	ev->mY = aY;
	ev->mMessage = aString;
}

void EventSystem::eventLoop()
{
	InputSystem& is = InputSystem::getInstance();
	is.mMousePosRelative = { 0, 0 };
	is.mMouseWheelRelative = { 0, 0 };
	for (auto& key : is.mKeys)
	{
		key.mPressed = key.mReleased = false;
	}

	
	while (true)
	{
		const Event ev = getEvent();
		if (ev.mType == EventType::NONE)
		{
			break;
		}
		processEvent(ev);
	}
}

void EventSystem::clearEventQueue() {
	const std::lock_guard lock(mMutex);
	mEventHead = mEventTail = 0;
}

void EventSystem::reset()
{
	const std::lock_guard lock(mMutex);
	InputSystem& is = InputSystem::getInstance();
	is.mMousePosRelative = { 0, 0 };
	is.mMouseWheelRelative = { 0, 0 };
	for (auto& key : is.mKeys)
	{
		key.reset();
	}
}

void EventSystem::setCallback(const EventType aEventType, const Input aInput, const std::function<bool(const Event&)>&
                              aCallback)
{
	const auto it = mCallbacks.find(aEventType);
	if (it == mCallbacks.end()) mCallbacks.insert({ aEventType , {} });

	const auto& it2 = mCallbacks.find(aEventType);
	it2->second.insert({ aInput, aCallback });
}

Event EventSystem::getEvent()
{
	const std::lock_guard lock(mMutex);
	
	if (mEventHead > mEventTail) {
		mEventTail++;
		return mEventQue[(mEventTail - 1) & MASK_QUED_EVENTS];
	}
	return Event{};
}

void EventSystem::processEvent(const Event& aEvent)
{
	if (!mCallbacks.empty()) {
		const auto& it = mCallbacks.find(aEvent.mType);
		if (it != mCallbacks.end())
		{
			const auto& it2 = it->second.find(aEvent.mInput);
			if ((it2 != it->second.end()) && it2->second(aEvent)) return;
		}
	}

	InputSystem& inputSystem = InputSystem::getInstance();
	if (aEvent.isKeyEvent()) {
		InputSystem::Key& key = inputSystem.mKeys[static_cast<int>(aEvent.getInput())];
		if (!key.mDown && aEvent.isKeyDown()) key.mPressed = true;
		else if (key.mDown && !aEvent.isKeyDown()) key.mReleased = true;
		key.mDown = aEvent.isKeyDown();
	}
	else if (aEvent.isMouseAbsoluteEvent()) {
		inputSystem.mMousePosAbsolute = { aEvent.getXCoord(), aEvent.getYCoord() };
	}
	else if (aEvent.isMouseRelativeEvent()) {
		inputSystem.mMousePosRelative += glm::vec2(aEvent.getXCoord(), aEvent.getYCoord());
	}
	else if (aEvent.isMouseWheelRelativeEvent()) {
		inputSystem.mMouseWheelRelative += glm::vec2(aEvent.getXCoord(), aEvent.getYCoord());
	}
	else if (aEvent.mType == EventType::MOUSE_LEAVE) {
        inputSystem.mMousePosAbsolute = { -1, -1 };
	}
	else if (aEvent.mType == EventType::ACTION) {
		Common &common = Common::getInstance();
		switch (aEvent.getInput()) {
		case Input::A_NEW_SCENE:
			common.newScene();
			break;
		case Input::A_OPEN_SCENE:
			common.openScene(aEvent.getMessage());
			break;
		case Input::A_ADD_SCENE:
			common.addScene(aEvent.getMessage());
			break;
		case Input::A_ADD_MODEL:
			common.addModel(aEvent.getMessage());
			break;
		case Input::A_ADD_LIGHT:
			common.addLight(aEvent.getMessage());
			break;
		case Input::A_EXPORT_SCENE:
			{
				const auto bitmask = static_cast<uint32_t>(aEvent.getValue());
				common.exportScene(aEvent.getMessage(), bitmask);
			}
			break;
		case Input::A_WINDOW_RESIZE:
			common.getRenderSystem()->renderSurfaceResize(static_cast<uint32_t>(aEvent.getXCoord()), static_cast<uint32_t>(aEvent.getYCoord()));
			break;
		case Input::A_RELOAD_BACKEND_IMPL:
			common.reloadBackendImplementation();
			break;
		case Input::A_CHANGE_BACKEND_IMPL:
			common.changeBackendImplementation(aEvent.getValue());
			break;
		case Input::A_CLEAR_CACHE:
			common.clearCache();
			break;
		case Input::A_TAKE_SCREENSHOT:
			common.queueScreenshot(aEvent.getMessage(), static_cast<uint32_t>(aEvent.getValue()));
			break;
		case Input::A_EXIT:
			common.queueShutdown();
			break;
		}
	}
}

