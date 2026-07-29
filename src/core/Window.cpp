#include "Window.h"
#include "Logger.h"
#include <SDL.h>
#include <SDL_syswm.h>

namespace nexus::core {

Window::Window() = default;
Window::~Window() { destroy(); }

bool Window::create(const WindowDesc& desc) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        NX_ERROR("Window", "SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    Uint32 flags = SDL_WINDOW_SHOWN;
    if (desc.resizable) flags |= SDL_WINDOW_RESIZABLE;
    if (desc.highDpi)   flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    if (desc.fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    // bgfx takes ownership of the underlying window handle directly; we don't pass SDL_WINDOW_OPENGL here.
    m_window = SDL_CreateWindow(
        desc.title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        desc.width, desc.height,
        flags);

    if (!m_window) {
        NX_ERROR("Window", "SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    SDL_GetWindowSize(m_window, &m_width, &m_height);

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (SDL_GetWindowWMInfo(m_window, &wmi)) {
        m_nativeHandle = (void*)&wmi.info;
    }
    NX_INFO("Window", "Created window %dx%d", m_width, m_height);
    return true;
}

void Window::destroy() {
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    // Note: SDL_Quit deferred to engine shutdown
}

bool Window::pollEvents() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_QUIT:
                m_shouldClose = true;
                break;
            case SDL_WINDOWEVENT: {
                switch (ev.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        m_width = ev.window.data1;
                        m_height = ev.window.data2;
                        if (onResize) onResize(m_width, m_height);
                        break;
                    case SDL_WINDOWEVENT_MINIMIZED:
                        m_minimized = true;
                        break;
                    case SDL_WINDOWEVENT_RESTORED:
                        m_minimized = false;
                        break;
                }
                break;
            }
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                if (onKey) onKey(ev.key.keysym.sym, ev.key.keysym.scancode, ev.key.keysym.mod, ev.key.state == SDL_PRESSED ? 1 : 0);
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (onMouseButton) onMouseButton(ev.button.button, ev.button.clicks, ev.button.x, ev.button.y);
                break;
            case SDL_MOUSEMOTION:
                if (onMouseMove) onMouseMove((float)ev.motion.x, (float)ev.motion.y);
                break;
            case SDL_MOUSEWHEEL:
                if (onMouseWheel) onMouseWheel((float)ev.wheel.x, (float)ev.wheel.y);
                break;
            case SDL_TEXTINPUT:
                if (onTextInput) onTextInput((unsigned int)(unsigned char)ev.text.text[0]);
                break;
        }
    }
    return !m_shouldClose;
}

void Window::swap() {
    // bgfx handles frame swap; this is a placeholder for non-bgfx backends
}

void* Window::nativeHandle() const {
    return m_nativeHandle;
}

void Window::setTitle(const std::string& t) {
    if (m_window) SDL_SetWindowTitle(m_window, t.c_str());
}

void Window::setSize(int w, int h) {
    if (m_window) SDL_SetWindowSize(m_window, w, h);
    m_width = w; m_height = h;
}

} // namespace nexus::core
