#include <tamashii/engine/platform/window.hpp>
#include <tamashii/engine/common/input.hpp>
#include <tamashii/engine/common/common.hpp>

#include <imgui.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <imgui_impl_osx.h>

T_USE_NAMESPACE
std::unordered_map<NSView*, Window*> Window::mWindowList;

static bool handleEvent(NSEvent* event, NSView* view)
{
    Window* window = Window::getWindow(view);
    if (!window) return false;
    bool imguiReadsMouse = false;
    bool imguiReadsKeyboard = false;
    {
        if (ImGui_ImplOSX_HandleEvent(event,view))
            return true;
    }
    if (ImGui::GetCurrentContext()) {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) imguiReadsMouse = true;
        if (io.WantCaptureKeyboard) imguiReadsKeyboard = true;

        // don't block middle mouse button when hovering over imgui gui (mainly used for imguizmo)
        //if(event.type == NSEventTypeWh || uMsg == WM_MBUTTONUP) imguiReadsMouse = false;
    }
    
    NSPoint mouseLoc = event.locationInWindow;
    mouseLoc = NSMakePoint(mouseLoc.x, view.bounds.size.height - mouseLoc.y);
    switch(event.type) {
        case NSEventTypeMouseMoved:
        {
            int32_t deltaX, deltaY;
            CGGetLastMouseDelta(&deltaX, &deltaY);
            EventSystem::queueEvent(EventType::MOUSE_DELTA, Input::NONE, 0, deltaX, deltaY);
            EventSystem::queueEvent(EventType::MOUSE_ABSOLUTE, Input::NONE, 0, mouseLoc.x, mouseLoc.y);
            return true;
        }
        case NSEventTypeScrollWheel:
        {
            float mag = std::ceil(std::abs([event deltaY]));
            EventSystem::queueEvent(EventType::MOUSE_DELTA, Input::NONE, std::copysign(mag, [event deltaY]));
            return true;
        }
        case NSEventTypeLeftMouseDown:
        {
            if (imguiReadsMouse || mouseLoc.x < 0 || mouseLoc.y < 0) return false;
            EventSystem::queueEvent(EventType::KEY, Input::MOUSE_LEFT, true, mouseLoc.x, mouseLoc.y);
            return true;
        }
        case NSEventTypeLeftMouseUp:
        {
            if (imguiReadsMouse || mouseLoc.x < 0 || mouseLoc.y < 0) return false;
            EventSystem::queueEvent(EventType::KEY, Input::MOUSE_LEFT, false, mouseLoc.x, mouseLoc.y);
            return true;
        }
        case NSEventTypeRightMouseDown:
        {
            if (imguiReadsMouse || mouseLoc.x < 0 || mouseLoc.y < 0) return false;
            EventSystem::queueEvent(EventType::KEY, Input::MOUSE_RIGHT, true, mouseLoc.x, mouseLoc.y);
            return true;
        }
        case NSEventTypeRightMouseUp:
        {
            if (imguiReadsMouse || mouseLoc.x < 0 || mouseLoc.y < 0) return false;
            EventSystem::queueEvent(EventType::KEY, Input::MOUSE_RIGHT, false, mouseLoc.x, mouseLoc.y);
            return true;
        }
        case NSEventTypeOtherMouseDown:
        {
            if (imguiReadsMouse || mouseLoc.x < 0 || mouseLoc.y < 0) return false;
            EventSystem::queueEvent(EventType::KEY, Input::MOUSE_WHEEL, true, mouseLoc.x, mouseLoc.y);
            return true;
        }
        case NSEventTypeOtherMouseUp:
        {
            if (imguiReadsMouse || mouseLoc.x < 0 || mouseLoc.y < 0) return false;
            EventSystem::queueEvent(EventType::KEY, Input::MOUSE_WHEEL, false, mouseLoc.x, mouseLoc.y);
            return true;
        }
        case NSEventTypeKeyDown:
        {
            return true;
        }
        case NSEventTypeKeyUp:
        {
            return true;
        }
    }
    return false;
}

@interface WindowDelegate : NSObject <NSWindowDelegate>
@end
@implementation WindowDelegate
- (BOOL)windowShouldClose:(id)sender {
    EventSystem::queueEvent(EventType::ACTION, Input::A_EXIT);
}
- (void)windowWillMiniaturize:(NSNotification *)notification {
    
}
- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{
    //Common::getInstance().getRenderSystem()->renderSurfaceResize(frameSize.width, frameSize.height);
    return frameSize;
}
@end


Window::~Window() {
    
}

bool Window::init(const char* name, glm::ivec2 size, glm::ivec2 position)
{

    NSRect           windowRect;
        
    // Create a window of the desired size
    windowRect.origin.x = position.x;
    windowRect.origin.y = position.y;
    windowRect.size.width = size.x;
    windowRect.size.height = size.y;
    mSize = size;
    mPosition = position;
        
    
    mWindow = [[NSWindow alloc] initWithContentRect:windowRect
                                         styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable)
                                                    backing:NSBackingStoreBuffered
                                                    defer:NO];
    NSWindow *_window = (NSWindow*)mWindow;
    
    NSSize nssize;
    nssize.height = size.y;
    nssize.width = size.x;
    
    [_window setDelegate:[WindowDelegate alloc]];
    [_window setContentSize:nssize];
    [_window setCollectionBehavior:NSWindowCollectionBehaviorDefault];
    [_window setTitle: [NSString stringWithUTF8String:name]];
    [_window orderFront: nil];
    [_window setAcceptsMouseMovedEvents: YES];
    
    // Window: set Metal Layer
    NSBundle* bundle = [NSBundle bundleWithPath:@"/System/Library/Frameworks/QuartzCore.framework"];
    if (!bundle)
    {
        spdlog::error("Could not load Metal Layer");
    }
    CAMetalLayer *metalLayer = [[bundle classNamed:@"CAMetalLayer"] layer];

    [[_window contentView] setWantsLayer:YES];
    [[_window contentView] setLayer:metalLayer];
    
    const float dpi = (float)[_window backingScaleFactor];
    
    mWindowList.insert(std::pair(getWindowHandle(), this));
	return true;
}

bool Window::destroy()
{
    mWindowList.erase(getWindowHandle());
    [(NSWindow*)mWindow close];
    mWindow = nil;
	return true;
}


bool Window::isWindow() const
{
    return mWindow;
}

void Window::showWindow(bool show) const
{
}

void Window::grabMouse() {
    mMouseGrabbed = true;
}
void Window::ungrabMouse() {
    mMouseGrabbed = false;
}

glm::vec2 Window::getCenter() const
{
    return { round(mSize.x * 0.5f), round(mSize.y * 0.5f)};
}

void Window::setSize(const glm::ivec2 size)
{
    mSize = size;
}

glm::ivec2 Window::getSize() const
{
    return mSize;
}

NSView* Window::getWindowHandle()
{
    return isWindow() ? [(NSWindow*)mWindow contentView] : nullptr;
}

void* Window::getInstanceHandle()
{
    return isWindow() ? mInstance : nullptr;
}

Window* Window::getWindow(NSView* const handle)
{
    auto search = mWindowList.find(handle);
    if (search != mWindowList.end()) return search->second;
    return nullptr;
}

void Window::readEvents() {
    if (!isWindow()) return;
    NSEvent *event;
    while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:0 inMode:NSDefaultRunLoopMode dequeue:YES])){
        handleEvent(event,(NSView*)getWindowHandle());
        [NSApp sendEvent:event];
    }
}
