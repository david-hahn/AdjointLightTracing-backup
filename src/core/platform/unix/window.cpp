#include <tamashii/core/platform/window.hpp>
#include <tamashii/core/common/input.hpp>

#if defined( VK_USE_PLATFORM_WAYLAND_KHR )
#elif defined( VK_USE_PLATFORM_XCB_KHR )
#include <xcb/xcb.h>
#include <xcb/xfixes.h>
#include <tamashii/core/gui/x11/imgui_impl_x11.h>
#endif

T_USE_NAMESPACE

std::unordered_map<xcb_window_t, Window*> Window::mWindowList;

#define XCB_KEYCODE_TO_SCANCODE(code) (code-8)
#define XCB_EVENT_RESPONSE_TYPE_MASK (0x7f)


void handleEvent(xcb_generic_event_t *event)
{
    bool imgui_reads_mouse = false;
    bool imgui_reads_keyboard = false;
    {
        ImGui_ImplX11_ProcessEvent(event);
    }
    if (ImGui::GetCurrentContext()) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) imgui_reads_mouse = true;
        if (io.WantCaptureKeyboard) imgui_reads_keyboard = true;
        
        if(((event->response_type & XCB_EVENT_RESPONSE_TYPE_MASK) == XCB_BUTTON_PRESS && ((xcb_button_press_event_t *)event)->detail == 2)||
                ((event->response_type & XCB_EVENT_RESPONSE_TYPE_MASK) == XCB_BUTTON_RELEASE && ((xcb_button_release_event_t *)event)->detail == 2)) {
            imgui_reads_mouse = false;
        }
    }

    switch (event->response_type & XCB_EVENT_RESPONSE_TYPE_MASK)
    {
    case XCB_CLIENT_MESSAGE:
    {
        xcb_client_message_event_t *client_m = (xcb_client_message_event_t *)event;
        if ((*(xcb_client_message_event_t*)event).data.data32[0] == Window::getWindow(&client_m->window)->getCloseAtom()->atom) {
            EventSystem::queueEvent(EventType::ACTION, Input::A_EXIT);
        }
        break;
    }
    case XCB_DESTROY_NOTIFY:
        EventSystem::queueEvent(EventType::ACTION, Input::A_EXIT);
    break;
    case XCB_MOTION_NOTIFY:
    {
        if(imgui_reads_mouse) break;
        xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;

        Window *w = Window::getWindow(&motion->event);
        if(w->isMouseGrabbed()){
            glm::vec2 c = w->getCenter();
            int relative_x = motion->event_x - c.x;
            int relative_y = motion->event_y - c.y;
            EventSystem::queueEvent(EventType::MOUSE_DELTA, Input::NONE, 0, relative_x, relative_y);
        } else { 
            static int x = motion->event_x, y = motion->event_y;
            EventSystem::queueEvent(EventType::MOUSE_DELTA, Input::NONE, 0, motion->event_x - x, y - motion->event_y);
            if(motion->event_x >= 0 && motion->event_y >=0) {
                x = motion->event_x;
                y = motion->event_y;
                EventSystem::queueEvent(EventType::MOUSE_ABSOLUTE, Input::NONE, 0, x, y);
            }
        }
        break;
    }
    break;
    case XCB_LEAVE_NOTIFY:
        EventSystem::queueEvent(EventType::MOUSE_LEAVE, Input::NONE);
    break;
    case XCB_BUTTON_PRESS:
    {
        xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
        if(imgui_reads_mouse) break;
        switch(press->detail){
        case 1: EventSystem::queueEvent(EventType::KEY, Input::MOUSE_LEFT, true, press->event_x, press->event_y); break;
        case 2: EventSystem::queueEvent(EventType::KEY, Input::MOUSE_WHEEL, true, press->event_x, press->event_y); break;
        case 3: EventSystem::queueEvent(EventType::KEY, Input::MOUSE_RIGHT, true, press->event_x, press->event_y); break;
        case 4: EventSystem::queueEvent(EventType::MOUSE_WHEEL_DELTA, Input::NONE, 0, 0, 1); break;   
        case 5: EventSystem::queueEvent(EventType::MOUSE_WHEEL_DELTA, Input::NONE, 0, 0, -1); break;  
        }
        break;
    }
    break;
    case XCB_BUTTON_RELEASE:
    {
        xcb_button_release_event_t *press = (xcb_button_release_event_t *)event;
        if(imgui_reads_mouse) break;
        switch(press->detail){
        case 1: EventSystem::queueEvent(EventType::KEY, Input::MOUSE_LEFT, false, press->event_x, press->event_y); break;
        case 2: EventSystem::queueEvent(EventType::KEY, Input::MOUSE_WHEEL, false, press->event_x, press->event_y); break;
        case 3: EventSystem::queueEvent(EventType::KEY, Input::MOUSE_RIGHT, false, press->event_x, press->event_y); break;
        }
        break;
    }
    break;
    case XCB_KEY_PRESS:
    {
        const xcb_key_press_event_t *keyEvent = (const xcb_key_press_event_t *)event;
        Input key = (Input)XCB_KEYCODE_TO_SCANCODE(keyEvent->detail);
        EventSystem::queueEvent(EventType::KEY, key, true, 0, 0);
    }
    break;
    case XCB_KEY_RELEASE:
    {
        const xcb_key_release_event_t *keyEvent = (const xcb_key_release_event_t *)event;
        Input key = (Input)XCB_KEYCODE_TO_SCANCODE(keyEvent->detail);
        EventSystem::queueEvent(EventType::KEY, key, false, 0, 0);
    }
    break;
    case XCB_CONFIGURE_NOTIFY:
    {
        xcb_configure_notify_event_t *notifyEvent = (xcb_configure_notify_event_t *)event;
        Window *window = Window::getWindow(&notifyEvent->window);
        const std::lock_guard lock(window->resizeMutex());
        window->setSize({notifyEvent->width, notifyEvent->height});
        window->setPosition({notifyEvent->x, notifyEvent->y});
    }
    break;
    }
}


static inline xcb_intern_atom_reply_t* intern_atom_helper(xcb_connection_t *conn, bool only_if_exists, const char *str)
{
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, only_if_exists, strlen(str), str);
    return xcb_intern_atom_reply(conn, cookie, NULL);
}

Window::~Window(){}


bool Window::init(const char* aName, const glm::ivec2 aSize, const glm::ivec2 aPosition)
{
    mSize = aSize;
    mPosition = aPosition;

    
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    
    
    
    
    mConnection = xcb_connect(NULL, &scr);
    assert( mConnection );
    if( xcb_connection_has_error(mConnection) ) {
        spdlog::error("Could not find a compatible Vulkan ICD!\n");
        exit(1);
    }
    xcb_xfixes_query_version((xcb_connection_t*)this->getInstanceHandle(), 4, 0);

    setup = xcb_get_setup(mConnection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);
    mScreen = iter.data;

    
    int32_t value_mask, value_list[32];

    mWindow = xcb_generate_id(mConnection);
    mWindowList.insert(std::pair<xcb_window_t, Window*>(mWindow, this));

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = mScreen->black_pixel;
    value_list[1] =
        XCB_EVENT_MASK_KEY_RELEASE |
        XCB_EVENT_MASK_KEY_PRESS |
        XCB_EVENT_MASK_EXPOSURE |
        XCB_EVENT_MASK_STRUCTURE_NOTIFY |
        XCB_EVENT_MASK_POINTER_MOTION |
        XCB_EVENT_MASK_LEAVE_WINDOW |
        XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE;

    xcb_create_window(mConnection,
        XCB_COPY_FROM_PARENT,
        mWindow, mScreen->root,
        0, 0, aSize.x, aSize.y, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        mScreen->root_visual,
        value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_reply_t* reply = intern_atom_helper(mConnection, true, "WM_PROTOCOLS");
    mAtomWmDeleteWindow = intern_atom_helper(mConnection, false, "WM_DELETE_WINDOW");

    xcb_change_property(mConnection, XCB_PROP_MODE_REPLACE,
        mWindow, (*reply).atom, 4, 32, 1,
        &(*mAtomWmDeleteWindow).atom);

    std::string windowTitle = aName;
    xcb_change_property(mConnection, XCB_PROP_MODE_REPLACE,
        mWindow, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
        strlen(aName), windowTitle.c_str());
    free(reply);

    /**
     * Set the WM_CLASS property to display
     * title in dash tooltip and application menu
     * on GNOME and other desktop environments
     */
    std::string wm_class;
    wm_class = wm_class.insert(0, aName);
    wm_class = wm_class.insert(strlen(aName), 1, '\0');
    wm_class = wm_class.insert(strlen(aName) + 1, aName);
    wm_class = wm_class.insert(wm_class.size(), 1, '\0');
    xcb_change_property(mConnection, XCB_PROP_MODE_REPLACE, mWindow, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8, wm_class.size() + 2, wm_class.c_str());

    xcb_set_input_focus(mConnection, XCB_INPUT_FOCUS_POINTER_ROOT, mWindow, XCB_CURRENT_TIME);
    return true;
}

bool Window::initWithOpenGlContex(const char *aName, glm::ivec2 aSize, glm::ivec2 aPosition)
{
    return false;
}

bool Window::destroy()
{
    ungrabMouse();
    mWindowList.erase(mWindow);
    xcb_destroy_window( mConnection, mWindow );
    xcb_disconnect( mConnection );
    mWindow			= 0;
    mConnection		= nullptr;
	return true;
}

bool Window::isWindow() const
{
    return mConnection;
}
void Window::showWindow(const Show aShow) {
    if(mConnection){
        if(aShow == Show::yes || aShow == Show::max) xcb_map_window(mConnection, mWindow);
        else if(aShow == Show::no)  xcb_unmap_window(mConnection, mWindow);
        xcb_flush(mConnection);
    }
};

void Window::setSize(const glm::ivec2 aSize){
    mSize = aSize;
}
glm::ivec2 Window::getSize() const {
    return mSize;
}

void Window::grabMouse()
{
    if(!isWindow()) return;
    mMouseGrabbed = true;
    xcb_grab_pointer(mConnection, 0, mWindow,
                XCB_EVENT_MASK_BUTTON_RELEASE
                | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_POINTER_MOTION,
                XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
                mWindow, XCB_NONE, XCB_CURRENT_TIME);
    xcb_xfixes_hide_cursor(mConnection, mWindow);
    xcb_flush(mConnection);
}

void Window::ungrabMouse()
{
    if(!isWindow()) return;
    mMouseGrabbed = false;
    xcb_ungrab_pointer(mConnection, XCB_CURRENT_TIME);
    xcb_xfixes_show_cursor(mConnection, mWindow);
    xcb_flush(mConnection);
}


glm::vec2 Window::getCenter() const {
    return { round(mSize.x * 0.5f), round(mSize.y * 0.5f)};
}

xcb_window_t* Window::getWindowHandle()
{
    return isWindow() ? &mWindow : nullptr;
}

xcb_connection_t* Window::getInstanceHandle()
{
    return isWindow() ? mConnection : nullptr;
}

Window* Window::getWindow(xcb_window_t* const aHandle)
{
    auto search = mWindowList.find(*aHandle);
    if (search != mWindowList.end()) return search->second;
    return nullptr;
}

void Window::readEvents() {
    if(!isWindow()) return;

    if(isMouseGrabbed()){
        glm::vec2 c;
        c = getCenter();
        xcb_warp_pointer(mConnection, XCB_NONE, mWindow, 0, 0, 0, 0, c.x, c.y);
        xcb_flush(mConnection);
    }
    xcb_generic_event_t *event;
    while ((event = xcb_poll_for_event(mConnection)))
    {
        handleEvent(event);
        free(event);
    }
}
