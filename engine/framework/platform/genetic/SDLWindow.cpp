//
// Created by Zach Lee on 2022/9/25.
//

#include "SDLWindow.h"
#include <framework/window/IWindowEvent.h>
#include <framework/window/NativeWindow.h>
#include <framework/window/NativeWindowManager.h>

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <objc/runtime.h>
#include <objc/message.h>
#endif

namespace sky {

#ifdef __APPLE__
    static void MakeProcessForegroundApp()
    {
        // 终端启动的进程默认 activation policy 是 accessory，收不到键盘焦点。
        // 转成 regular app 并激活。用 objc runtime 以便在 .cpp 里调用。
        id app = ((id(*)(Class, SEL))objc_msgSend)(objc_getClass("NSApplication"), sel_getUid("sharedApplication"));
        if (app != nil) {
            ((void(*)(id, SEL, long))objc_msgSend)(app, sel_getUid("setActivationPolicy:"), 0 /* NSApplicationActivationPolicyRegular */);
            ((void(*)(id, SEL, BOOL))objc_msgSend)(app, sel_getUid("activateIgnoringOtherApps:"), YES);
        }
    }
#endif

    SDLWindow::~SDLWindow()
    {
        SDL_DestroyWindow(window);
    }

    bool SDLWindow::Init(const Descriptor &desc)
    {
        descriptor = desc;
        window = SDL_CreateWindow(desc.titleName.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, desc.width, desc.height,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_CAPTURE |
                                      SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS | SDL_WINDOW_VULKAN);

        if (window != nullptr) {
            winID = SDL_GetWindowID(window);
            NativeWindowManager::Get()->Register(this);
#ifdef __APPLE__
            MakeProcessForegroundApp();
            SDL_RaiseWindow(window);
#endif
        }
        return window != nullptr;
    }

    void SDLWindow::Dispatch(const SDL_WindowEvent &event) {
        switch (event.event) {
            case SDL_WINDOWEVENT_SHOWN:
            case SDL_WINDOWEVENT_RESTORED:
                break;
//            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED: {
                UpdateWindow();
                WindowResizeEvent mEvent = {};
                mEvent.winID = winID;
                mEvent.width = (uint32_t)(event.data1 * scale);
                mEvent.height = (uint32_t)(event.data2 * scale);
                Event<IWindowEvent>::BroadCast(this, &IWindowEvent::OnWindowResize, mEvent);
                break;
            }
            case SDL_WINDOWEVENT_FOCUS_GAINED:
            case SDL_WINDOWEVENT_FOCUS_LOST: {
                Event<IWindowEvent>::BroadCast(this, &IWindowEvent::OnFocusChanged, event.event == SDL_WINDOWEVENT_FOCUS_GAINED);
                break;
            }
            case SDL_WINDOWEVENT_HIDDEN:
            case SDL_WINDOWEVENT_MINIMIZED:
            case SDL_WINDOWEVENT_ENTER:
            case SDL_WINDOWEVENT_CLOSE: {
                break;
            }
        }
    }
}
