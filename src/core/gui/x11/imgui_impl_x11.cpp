
















#include <tamashii/core/gui/x11/imgui_impl_x11.h>
#include <stdio.h>
#include "imgui.h"
#include <stdlib.h>
#include <time.h>
#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xfixes.h>
#include <xcb/xcb_cursor.h>

#define explicit c_explicit
#include <xcb/xkb.h>
#undef explicit

#ifdef HAS_VULKAN
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>
#include "imgui_impl_vulkan.h"
#endif








enum X11_Atom_
{
    X11_Atom_Clipboard,
    X11_Atom_Targets,
    X11_Atom_Dear_Imgui_Selection,
    X11_Atom_WM_Delete_Window,
    X11_Atom_WM_Protocols,
    X11_Atom_Net_Workarea,
    X11_Atom_Net_Supported,
    X11_Atom_Net_WM_Window_Type,
    X11_Atom_Net_WM_Window_Type_Toolbar,
    X11_Atom_Net_WM_Window_Type_Menu,
    X11_Atom_Net_WM_Window_Type_Dialog,
    X11_Atom_Motif_WM_Hints,
    X11_Atom_COUNT
};

static const char* g_X11AtomNames[X11_Atom_COUNT] = {
    "CLIPBOARD",
    "TARGETS",
    "DEAR_IMGUI_SELECTION",
    "WM_DELETE_WINDOW",
    "WM_PROTOCOLS",
    "_NET_WORKAREA",
    "_NET_SUPPORTED",
    "_NET_WM_WINDOW_TYPE",
    "_NET_WM_WINDOW_TYPE_TOOLBAR",
    "_NET_WM_WINDOW_TYPE_MENU",
    "_NET_WM_WINDOW_TYPE_DIALOG",
    "_MOTIF_WM_HINTS"
};

enum Known_Target_
{
    Known_Target_UTF8_String,
    Known_Target_Compound_text,
    Known_Target_Text,
    Known_Target_String,
    Known_Target_Text_Plain_UTF8,
    Known_Target_Text_Plain,
    Known_Target_COUNT
};

static const char* g_KnownTargetNames[Known_Target_COUNT] = {
    "UTF8_STRING",
    "COMPOUND_TEXT",
    "TEXT",
    "STRING",
    "text/plain;charset=utf-8",
    "text/plain"
};

struct ImGuiViewportDataX11
{
    
    ImVec2 Pos;
    ImVec2 Size;
    ImGuiViewportFlags Flags;

    ImGuiViewportDataX11() { Pos = ImVec2(0, 0); Size = ImVec2(0, 0); Flags = 0; }
    ~ImGuiViewportDataX11() { }
};

struct MotifHints
{
    uint32_t   Flags;
    uint32_t   Functions;
    uint32_t   Decorations;
    int32_t    InputMode;
    uint32_t   Status;
};


static xcb_connection_t*      g_Connection;
static xcb_connection_t*      g_ClipboardConnection;
static xcb_window_t           g_MainWindow;
static xcb_key_symbols_t*     g_KeySyms;
static xcb_cursor_context_t*  g_CursorContext;
static xcb_screen_t*          g_Screen;
static xcb_window_t           g_ClipboardHandler;
static xcb_atom_t             g_X11Atoms[X11_Atom_COUNT];
static xcb_atom_t             g_KnownTargetAtoms[Known_Target_COUNT];

static timespec               g_LastTime;
static timespec               g_CurrentTime;

static bool                   g_HideXCursor = false;
static ImGuiMouseCursor       g_CurrentCursor = ImGuiMouseCursor_Arrow;
static char*                  g_CurrentClipboardData;

static const char* g_CursorMap[ImGuiMouseCursor_COUNT] = {
    "arrow",               
    "xterm",               
    "fleur",               
    "sb_v_double_arrow",   
    "sb_h_double_arrow",   
    "bottom_left_corner",  
    "bottom_right_corner", 
    "hand1",               
    "circle"               
};


static void ImGui_ImplX11_SetClipboardText(void* user_data, const char* text);
static const char* ImGui_ImplX11_GetClipboardText(void *user_data);
static void ImGui_ImplX11_ProcessPendingSelectionEvents(bool wait_for_notify=false);
static void ImGui_ImplX11_ProcessViewportMessages();


bool    ImGui_ImplX11_Init(xcb_window_t window)
{
    g_Connection = xcb_connect(nullptr, nullptr);
    g_Screen = xcb_setup_roots_iterator(xcb_get_setup(g_Connection)).data;
    g_MainWindow = window;
    xcb_generic_error_t* x_Err = nullptr;

    
    clock_gettime(CLOCK_MONOTONIC_RAW, &g_LastTime);

    
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;      
    
    io.BackendPlatformName = "imgui_impl_x11";

    
    
    
    xcb_intern_atom_cookie_t target_atom_cookies[Known_Target_COUNT];
    for (int i = 0; i < Known_Target_COUNT; ++i)
        target_atom_cookies[i] = xcb_intern_atom(g_Connection, 0, strlen(g_KnownTargetNames[i]), g_KnownTargetNames[i]);

    for (int i = 0; i < Known_Target_COUNT; ++i)
    {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(g_Connection, target_atom_cookies[i], &x_Err);
        IM_ASSERT((reply && !x_Err) && "Error getting atom reply");
        g_KnownTargetAtoms[i] = reply->atom;
        free(reply);
    }

    
    xcb_intern_atom_cookie_t atom_cookies[X11_Atom_COUNT];
    for (int i = 0; i < X11_Atom_COUNT; ++i)
        atom_cookies[i] = xcb_intern_atom(g_Connection, 0, strlen(g_X11AtomNames[i]), g_X11AtomNames[i]);

    for (int i = 0; i < X11_Atom_COUNT; ++i)
    {
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(g_Connection, atom_cookies[i], &x_Err);
        IM_ASSERT((reply && !x_Err) && "Error getting atom reply");
        g_X11Atoms[i] = reply->atom;
        free(reply);
    }

    
    xcb_get_geometry_reply_t* resp = xcb_get_geometry_reply(g_Connection, xcb_get_geometry(g_Connection, g_MainWindow), &x_Err);
    IM_ASSERT(!x_Err && "X error querying window geometry");
    io.DisplaySize = ImVec2(resp->width, resp->height);
    free(resp);

    
    g_KeySyms = xcb_key_symbols_alloc(g_Connection);

    
    
    
    
    xcb_xkb_use_extension_cookie_t extension_cookie = xcb_xkb_use_extension(g_Connection, 1, 0);
    xcb_xkb_use_extension_reply_t* extension_reply = xcb_xkb_use_extension_reply(g_Connection, extension_cookie, &x_Err);

    if (!x_Err && extension_reply->supported)
    {
        xcb_discard_reply(g_Connection,
            xcb_xkb_per_client_flags(g_Connection,
                        XCB_XKB_ID_USE_CORE_KBD,
                        XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
                        XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT,
                        0, 0, 0).sequence);
    }
    free(extension_reply);

    
    xcb_discard_reply(g_Connection, xcb_xfixes_query_version(g_Connection, 4, 0).sequence);

    
    xcb_cursor_context_new(g_Connection, g_Screen, &g_CursorContext);

    
    
    g_ClipboardConnection = xcb_connect(nullptr, nullptr);
    g_ClipboardHandler = xcb_generate_id(g_ClipboardConnection);
    xcb_screen_t *clipboard_screen = xcb_setup_roots_iterator(xcb_get_setup(g_ClipboardConnection)).data;
    xcb_create_window(g_ClipboardConnection, XCB_COPY_FROM_PARENT, g_ClipboardHandler,
                      clipboard_screen->root, 0, 0, 1, 1,
                      0, XCB_WINDOW_CLASS_INPUT_OUTPUT, clipboard_screen->root_visual,
                      0, nullptr);
    const char* clipboard_window_title = "Dear ImGui Requestor Window";
    xcb_change_property(g_ClipboardConnection, XCB_PROP_MODE_REPLACE,
                        g_ClipboardHandler, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                        strlen(clipboard_window_title), clipboard_window_title);

    
    g_CurrentClipboardData = strdup("");

    io.GetClipboardTextFn = ImGui_ImplX11_GetClipboardText;
    io.SetClipboardTextFn = ImGui_ImplX11_SetClipboardText;

    
    
    
    
    
    
    
    io.KeyMap[ImGuiKey_Tab] = XK_Tab - 0xFF00;
    io.KeyMap[ImGuiKey_LeftArrow] = XK_Left - 0xFF00;
    io.KeyMap[ImGuiKey_RightArrow] = XK_Right - 0xFF00;
    io.KeyMap[ImGuiKey_UpArrow] = XK_Up - 0xFF00;
    io.KeyMap[ImGuiKey_DownArrow] = XK_Down - 0xFF00;
    io.KeyMap[ImGuiKey_PageUp] = XK_Page_Up - 0xFF00;
    io.KeyMap[ImGuiKey_PageDown] = XK_Page_Up - 0xFF00;
    io.KeyMap[ImGuiKey_Home] = XK_Home - 0xFF00;
    io.KeyMap[ImGuiKey_End] = XK_End - 0xFF00;
    io.KeyMap[ImGuiKey_Insert] = XK_Insert - 0xFF00;
    io.KeyMap[ImGuiKey_Delete] = XK_Delete - 0xFF00;
    io.KeyMap[ImGuiKey_Backspace] = XK_BackSpace - 0xFF00;
    io.KeyMap[ImGuiKey_Space] = XK_space;
    io.KeyMap[ImGuiKey_Enter] = XK_Return - 0xFF00;
    io.KeyMap[ImGuiKey_Escape] = XK_Escape - 0xFF00;
    io.KeyMap[ImGuiKey_KeyPadEnter] = XK_KP_Enter - 0xFF00;
    io.KeyMap[ImGuiKey_A] = XK_a;
    io.KeyMap[ImGuiKey_C] = XK_c;
    io.KeyMap[ImGuiKey_V] = XK_v;
    io.KeyMap[ImGuiKey_X] = XK_x;
    io.KeyMap[ImGuiKey_Y] = XK_y;
    io.KeyMap[ImGuiKey_Z] = XK_z;

    g_HideXCursor = io.MouseDrawCursor;


    return true;
}

void    ImGui_ImplX11_Shutdown()
{
    xcb_destroy_window(g_ClipboardConnection, g_ClipboardHandler);
    xcb_cursor_context_free(g_CursorContext);
    xcb_key_symbols_free(g_KeySyms);
    xcb_flush(g_Connection);
    xcb_disconnect(g_ClipboardConnection);
}

void    ImGui_ImplX11_ChangeCursor(const char* name)
{
    xcb_font_t font = xcb_generate_id(g_Connection);
    
    
    xcb_cursor_t cursor = xcb_cursor_load_cursor(g_CursorContext, name);
    IM_ASSERT(cursor && "X cursor not found!");

    uint32_t value_list = cursor;
    xcb_change_window_attributes(g_Connection, g_MainWindow, XCB_CW_CURSOR, &value_list);
    xcb_free_cursor(g_Connection, cursor);
    xcb_close_font_checked(g_Connection, font);
}

void    ImGui_ImplX11_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (g_HideXCursor != io.MouseDrawCursor) 
    {
        g_HideXCursor = io.MouseDrawCursor;
        if (g_HideXCursor)
            xcb_xfixes_hide_cursor(g_Connection, g_MainWindow);
        else
            xcb_xfixes_show_cursor(g_Connection, g_MainWindow);
    }

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (g_CurrentCursor != imgui_cursor)
    {
        g_CurrentCursor = imgui_cursor;
        ImGui_ImplX11_ChangeCursor(g_CursorMap[g_CurrentCursor]);
    }

    xcb_flush(g_Connection);
}

void    ImGui_ImplX11_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    
    clock_gettime(CLOCK_MONOTONIC_RAW, &g_CurrentTime);
    io.DeltaTime = (g_CurrentTime.tv_sec - g_LastTime.tv_sec) + ((g_CurrentTime.tv_nsec - g_LastTime.tv_nsec) / 1000000000.0f);
    g_LastTime = g_CurrentTime;

    ImGui_ImplX11_UpdateMouseCursor();
    ImGui_ImplX11_ProcessPendingSelectionEvents();
}





bool ImGui_ImplX11_ProcessEvent(xcb_generic_event_t* event)
{
    if (ImGui::GetCurrentContext() == NULL)
        return false;

    ImGuiIO& io = ImGui::GetIO();
    switch (event->response_type & ~0x80)
    {
    case XCB_MOTION_NOTIFY:
    {
        xcb_motion_notify_event_t* e = (xcb_motion_notify_event_t*)event;
        io.MousePos = ImVec2(e->event_x, e->event_y);

        return true;
    }
    case XCB_KEY_PRESS:
    {
        xcb_key_press_event_t* e = (xcb_key_press_event_t*)event;
        
        
        
        
        uint32_t col = io.KeyShift ? 1 : 0;
        xcb_keysym_t k = xcb_key_press_lookup_keysym(g_KeySyms, e, col);

        if (k < 0xFF) 
        {
            io.KeysDown[k] = 1;
            io.AddInputCharacter(k);
        }
        else if (k >= XK_Shift_L && k <= XK_Hyper_R) 
        {
            switch(k)
            {
            case XK_Shift_L:
            case XK_Shift_R:
                io.KeyShift = true;
                break;
            case XK_Control_L:
            case XK_Control_R:
                io.KeyCtrl = true;
                break;
            case XK_Meta_L:
            case XK_Meta_R:
            case XK_Alt_L:
            case XK_Alt_R:
                io.KeyAlt = true;
                break;
            case XK_Super_L:
            case XK_Super_R:
                io.KeySuper = true;
                break;
            }
        }
        else if (k >= 0x1000100 && k <= 0x110ffff) 
            io.AddInputCharacterUTF16(k);
        else
            io.KeysDown[k - 0xFF00] = 1;

        return true;
    }
    case XCB_KEY_RELEASE:
    {
        xcb_key_press_event_t* e = (xcb_key_press_event_t*)event;
        xcb_keysym_t k = xcb_key_press_lookup_keysym(g_KeySyms, e, 0);

        if (k < 0xff)
            io.KeysDown[k] = 0;
        else if (k >= XK_Shift_L && k <= XK_Hyper_R) 
        {
            switch(k)
            {
            case XK_Shift_L:
            case XK_Shift_R:
                io.KeyShift = false;
                break;
            case XK_Control_L:
            case XK_Control_R:
                io.KeyCtrl = false;
                break;
            case XK_Meta_L:
            case XK_Meta_R:
            case XK_Alt_L:
            case XK_Alt_R:
                io.KeyAlt = false;
                break;
            case XK_Super_L:
            case XK_Super_R:
                io.KeySuper = false;
                break;
            }
        }
        else
            io.KeysDown[k - 0xFF00] = 0;

        return true;
    }
    case XCB_BUTTON_PRESS:
    {
        xcb_button_press_event_t* e = (xcb_button_press_event_t*)event;
        
        
        io.MousePos = ImVec2(e->event_x, e->event_y);

        
        if (e->detail >= 1 && e->detail <= 3)
            io.MouseDown[e->detail - 1] = true;
        else if (e->detail == 4)
            io.MouseWheel += 1.0;
        else if (e->detail == 5)
            io.MouseWheel -= 1.0;
        return true;
    }
    case XCB_BUTTON_RELEASE:
    {
        xcb_button_release_event_t* e = (xcb_button_release_event_t*)event;

        io.MousePos = ImVec2(e->event_x, e->event_y);

        if (e->detail >= 1 && e->detail <= 3)
            io.MouseDown[e->detail - 1] = false;
        return true;
    }
    case XCB_ENTER_NOTIFY:
    {
        if (g_HideXCursor)
        {
            xcb_xfixes_hide_cursor(g_Connection, g_MainWindow);
            
            xcb_flush(g_Connection);
        }
        return true;
    }
    case XCB_LEAVE_NOTIFY:
    {
        xcb_xfixes_show_cursor(g_Connection, g_MainWindow);
        xcb_flush(g_Connection);
        return true;
    }
    case XCB_CONFIGURE_NOTIFY:
    {
        xcb_configure_notify_event_t* r = (xcb_configure_notify_event_t*)event;
        ImGui::GetIO().DisplaySize = ImVec2(r->width, r->height);
        break;
    }
    }
    return false;
}

void ImGui_ImplX11_SetWindowDecoration(bool disabled, xcb_window_t window) 
{
    
    
    
    MotifHints hints;

    hints.Flags = 2;
    hints.Functions = 0;
    hints.Decorations = !disabled;
    hints.InputMode = 0;
    hints.Status = 0;

    xcb_change_property(g_Connection, XCB_PROP_MODE_REPLACE, window,
                        g_X11Atoms[X11_Atom_Motif_WM_Hints], g_X11Atoms[X11_Atom_Motif_WM_Hints],
                        32, 5, &hints );
}























void ImGui_ImplX11_ProcessPendingSelectionEvents(bool wait_for_notify)
{
    xcb_generic_event_t* event = wait_for_notify? xcb_wait_for_event(g_ClipboardConnection) : xcb_poll_for_event(g_ClipboardConnection);

    bool found_notify_event = false;
    while (event)
    {
        switch (event->response_type & ~0x80)
        {
        case XCB_SELECTION_NOTIFY:
        {
            xcb_selection_notify_event_t* e = (xcb_selection_notify_event_t*)event;
            found_notify_event = true;
            if (e->property != XCB_NONE) 
            {
                
                xcb_get_property_cookie_t get_property = xcb_get_property(g_ClipboardConnection, true, g_ClipboardHandler,
                                    g_X11Atoms[X11_Atom_Dear_Imgui_Selection], XCB_GET_PROPERTY_TYPE_ANY,
                                    0, UINT32_MAX - 1);
                xcb_get_property_reply_t *property_reply = xcb_get_property_reply(g_ClipboardConnection, get_property, nullptr);
                g_CurrentClipboardData = strdup((const char*)xcb_get_property_value(property_reply));
                free(property_reply);
            }
            break;
        }
        case XCB_SELECTION_REQUEST:
        {
            
            
            
            
            
            
            
            
            xcb_selection_request_event_t* e = (xcb_selection_request_event_t *)event;

            
            
            
            
            
            
            
            
            if (e->target == g_X11Atoms[X11_Atom_Targets])
            {
                xcb_change_property(g_ClipboardConnection, XCB_PROP_MODE_REPLACE,
                                    e->requestor, e->property,
                                    XCB_ATOM_ATOM, sizeof(xcb_atom_t) * 8,
                                    sizeof(xcb_atom_t) * Known_Target_COUNT,
                                    g_KnownTargetAtoms);
            }
            else
            {
                
                bool is_known_target = false;
                for (int i = 0; i < Known_Target_COUNT; ++i)
                {
                    if (g_KnownTargetAtoms[i] == e->target)
                    {
                        is_known_target = true;
                        break;
                    }
                }
                if (is_known_target)
                    xcb_change_property(g_ClipboardConnection, XCB_PROP_MODE_REPLACE, e->requestor,
                                        e->property, e->target, 8,
                                        strlen(g_CurrentClipboardData), g_CurrentClipboardData);
            }

            
            xcb_selection_notify_event_t notify_event = {};
            notify_event.response_type = XCB_SELECTION_NOTIFY;
            notify_event.time = XCB_CURRENT_TIME;
            notify_event.requestor = e->requestor;
            notify_event.selection = e->selection;
            notify_event.target = e->target;
            notify_event.property = e->property;
            xcb_generic_error_t *err = xcb_request_check(g_ClipboardConnection, xcb_send_event(g_ClipboardConnection, false,
                        e->requestor,
                        XCB_EVENT_MASK_PROPERTY_CHANGE,
                        (const char*)&notify_event));
            IM_ASSERT(!err && "Failed sending event");
            break;
        }
        }
        free(event);
        event = (wait_for_notify && !found_notify_event)? xcb_wait_for_event(g_ClipboardConnection) : xcb_poll_for_event(g_ClipboardConnection);
    }
}

const char* ImGui_ImplX11_GetClipboardText(void *user_data)
{
    xcb_generic_error_t *x_Err;
    xcb_atom_t current_selection_atom = g_X11Atoms[X11_Atom_Clipboard];
    
    xcb_get_selection_owner_reply_t *selection_reply = xcb_get_selection_owner_reply(g_ClipboardConnection,
                                                                                     xcb_get_selection_owner(g_ClipboardConnection, current_selection_atom),
                                                                                     &x_Err);

    IM_ASSERT(!x_Err && "Error getting selection reply");
    if (selection_reply->owner == 0) 
    {
        free(selection_reply);
        current_selection_atom = XCB_ATOM_PRIMARY;
        selection_reply = xcb_get_selection_owner_reply(g_ClipboardConnection,
                                            xcb_get_selection_owner(g_ClipboardConnection, current_selection_atom), &x_Err);
    }
    IM_ASSERT(!x_Err && "Error getting selection reply");

    
    if (selection_reply->owner != 0 || selection_reply->owner != g_ClipboardHandler)
    {
        
        x_Err = xcb_request_check(g_ClipboardConnection,
                    xcb_convert_selection_checked(g_ClipboardConnection, g_ClipboardHandler,
                                                    current_selection_atom, g_KnownTargetAtoms[0],
                                                    g_X11Atoms[X11_Atom_Dear_Imgui_Selection], XCB_CURRENT_TIME));

        IM_ASSERT(!x_Err && "Error getting convert selection");
        ImGui_ImplX11_ProcessPendingSelectionEvents(true);
    }

    free(selection_reply);
    return g_CurrentClipboardData;
}

void    ImGui_ImplX11_SetClipboardText(void* user_data, const char* text)
{
    xcb_generic_error_t *x_Err;
    g_CurrentClipboardData = (char *)realloc(g_CurrentClipboardData, strlen(text));
    strcpy(g_CurrentClipboardData, text);
    
    xcb_set_selection_owner(g_ClipboardConnection,
                            g_ClipboardHandler,
                            XCB_ATOM_PRIMARY,
                            XCB_CURRENT_TIME);
    xcb_xfixes_select_selection_input(g_ClipboardConnection,
                                      g_ClipboardHandler,
                                      XCB_ATOM_PRIMARY,
                                      XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER);
    xcb_discard_reply(g_ClipboardConnection,
        xcb_get_selection_owner_reply(g_ClipboardConnection, xcb_get_selection_owner(g_ClipboardConnection, XCB_ATOM_PRIMARY), &x_Err)->sequence);
    IM_ASSERT(!x_Err && "Error owning the primary selection");

    
    xcb_set_selection_owner(g_ClipboardConnection,
                            g_ClipboardHandler,
                            g_X11Atoms[X11_Atom_Clipboard],
                            XCB_CURRENT_TIME);
    xcb_xfixes_select_selection_input(g_ClipboardConnection,
                                      g_ClipboardHandler,
                                      g_X11Atoms[X11_Atom_Clipboard],
                                      XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER);
    xcb_discard_reply(g_ClipboardConnection,
        xcb_get_selection_owner_reply(g_ClipboardConnection, xcb_get_selection_owner(g_ClipboardConnection, g_X11Atoms[X11_Atom_Clipboard]), &x_Err)->sequence);
    IM_ASSERT(!x_Err && "Error owning the clipboard selection");

    xcb_flush(g_ClipboardConnection);
}
