//
// Created by Zach Lee on 2022/9/25.
//

#include "MacosWindow.h"
#include <SDL2/SDL_syswm.h>
#import <Cocoa/Cocoa.h>

namespace sky {

    bool MacosWindow::Init(const Descriptor &desc)
    {
        if (!SDLWindow::Init(desc)) {
            return false;
        }

        UpdateWindow();
        return winHandle != nullptr;
    }

    void MacosWindow::UpdateWindow()
    {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window, &wmInfo);
        // RHI 后端（vulkan/SwapchainImpl.mm）期望拿到 NSView*（会做 setLayer CAMetalLayer），
        // cocoa.window 是 NSWindow*，这里取它的 contentView。
        NSWindow *nsWindow = wmInfo.info.cocoa.window;
        winHandle = [nsWindow contentView];
    }

    NativeWindow *NativeWindow::Create(const Descriptor &des)
    {
        NativeWindow *window = new MacosWindow();
        if (!window->Init(des)) {
            delete window;
            window = nullptr;
        }
        return window;
    }
}
