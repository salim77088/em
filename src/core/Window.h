#pragma once
#include <string>
#include <functional>
#include <cstdint>

struct SDL_Window;
struct SDL_Cursor;
typedef union SDL_Event SDL_Event;

namespace nexus::core {

struct WindowDesc {
    std::string title = "NEXUS Engine";
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool vsync = true;
    bool resizable = true;
    bool highDpi = true;
};

class Window {
public:
    Window();
    ~Window();

    bool create(const WindowDesc& desc);
    void destroy();

    bool pollEvents();
    void swap();

    void* nativeHandle() const;
    SDL_Window* sdlWindow() const { return m_window; }

    int width() const { return m_width; }
    int height() const { return m_height; }
    float aspect() const { return m_height ? float(m_width) / float(m_height) : 1.0f; }
    bool shouldClose() const { return m_shouldClose; }
    bool isMinimized() const { return m_minimized; }

    void setTitle(const std::string& t);
    void setSize(int w, int h);

    std::function<void(int, int)> onResize;
    std::function<void(int, int, int, int)> onKey;
    std::function<void(int, int, int, int)> onMouseButton;
    std::function<void(float, float)> onMouseMove;
    std::function<void(float, float)> onMouseWheel;
    std::function<void(unsigned int)> onTextInput;

private:
    SDL_Window* m_window = nullptr;
    void* m_nativeHandle = nullptr;
    int m_width = 1280;
    int m_height = 720;
    bool m_shouldClose = false;
    bool m_minimized = false;
};

} // namespace nexus::core
