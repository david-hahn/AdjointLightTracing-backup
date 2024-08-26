
















#pragma once
#include <imgui.h>      
#include <stdint.h>
#include <xcb/xcb.h>

IMGUI_IMPL_API bool     ImGui_ImplX11_Init(xcb_window_t window);
IMGUI_IMPL_API void     ImGui_ImplX11_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplX11_NewFrame();
IMGUI_IMPL_API bool     ImGui_ImplX11_ProcessEvent(xcb_generic_event_t* event);
