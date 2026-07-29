#pragma once
#include <memory>
#include <string>
#include <functional>
#include "../core/Window.h"
#include "../graphics/Renderer.h"
#include "../scene/Scene.h"
#include "../nodes/NodeGraph.h"
#include "../editor/Editor.h"

namespace nexus::core {

class Engine {
public:
    static Engine& get() {
        static Engine instance;
        return instance;
    }

    Engine();
    ~Engine();

    bool init(const std::string& title = "NEXUS Engine", int w = 1280, int h = 720);
    void shutdown();

    bool run();
    void stop() { m_running = false; }

    void update();
    void render();

    Window& window() { return *m_window; }
    graphics::Renderer& renderer() { return *m_renderer; }
    scene::Scene& scene() { return *m_scene; }
    editor::Editor& editor() { return *m_editor; }
    nodes::NodeGraph& nodeGraph() { return *m_nodeGraph; }

    bool isRunning() const { return m_running; }
    float deltaTime() const { return m_delta; }
    float fps() const { return m_fps; }

private:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<graphics::Renderer> m_renderer;
    std::unique_ptr<scene::Scene> m_scene;
    std::unique_ptr<editor::Editor> m_editor;
    std::unique_ptr<nodes::NodeGraph> m_nodeGraph;

    bool m_initialized = false;
    bool m_running = false;
    float m_delta = 0.016f;
    float m_fps = 60.0f;
    unsigned long long m_frameCount = 0;
};

} // namespace nexus::core
