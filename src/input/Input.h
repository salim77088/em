#pragma once
#include <SDL.h>
#include <cstdint>
#include <unordered_map>
#include <bitset>

namespace nexus::input {

class Input {
public:
    static Input& get() {
        static Input instance;
        return instance;
    }

    void init() {
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    }

    void update() {
        m_prevMouseButtons = m_mouseButtons;
        m_prevKeys = m_keys;
        m_mouseDeltaX = m_mouseX - m_lastMouseX;
        m_mouseDeltaY = m_mouseY - m_lastMouseY;
        m_lastMouseX = m_mouseX;
        m_lastMouseY = m_mouseY;
    }

    bool keyDown(int scancode) const { return m_keys[scancode]; }
    bool keyPressed(int scancode) const { return m_keys[scancode] && !m_prevKeys[scancode]; }
    bool keyReleased(int scancode) const { return !m_keys[scancode] && m_prevKeys[scancode]; }

    bool mouseDown(int button) const { return m_mouseButtons[button]; }
    bool mousePressed(int button) const { return m_mouseButtons[button] && !m_prevMouseButtons[button]; }
    bool mouseReleased(int button) const { return !m_mouseButtons[button] && m_prevMouseButtons[button]; }

    float mouseX() const { return m_mouseX; }
    float mouseY() const { return m_mouseY; }
    float mouseDeltaX() const { return m_mouseDeltaX; }
    float mouseDeltaY() const { return m_mouseDeltaY; }

    void setKey(int scancode, bool down) { if (scancode >= 0 && scancode < 512) m_keys[scancode] = down; }
    void setMouse(int button, bool down) { if (button >= 0 && button < 8) m_mouseButtons[button] = down; }
    void setMousePos(float x, float y) { m_mouseX = x; m_mouseY = y; }

private:
    Input() = default;
    std::bitset<512> m_keys;
    std::bitset<512> m_prevKeys;
    std::bitset<8> m_mouseButtons;
    std::bitset<8> m_prevMouseButtons;
    float m_mouseX = 0, m_mouseY = 0;
    float m_lastMouseX = 0, m_lastMouseY = 0;
    float m_mouseDeltaX = 0, m_mouseDeltaY = 0;
};

} // namespace nexus::input
