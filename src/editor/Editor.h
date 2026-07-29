#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "../core/Logger.h"
#include "../core/Window.h"
#include "../graphics/Renderer.h"
#include "../scene/Scene.h"
#include "../nodes/NodeGraph.h"

struct ImGuiContext;
typedef union SDL_Event SDL_Event;

namespace nexus::editor {

class Editor {
public:
    Editor();
    ~Editor();

    bool init(core::Window* window, graphics::Renderer* renderer);
    void shutdown();

    void beginFrame();
    void endFrame();

    void render(Scene& scene);
    void renderNodeGraph(nodes::NodeGraph& graph);

    bool wantCaptureMouse() const;
    bool wantCaptureKeyboard() const;

    void showDemoWindow(bool* p = nullptr);

private:
    bool m_initialized = false;
    core::Window* m_window = nullptr;
    graphics::Renderer* m_renderer = nullptr;
    ImGuiContext* m_context = nullptr;
    bool m_showDemo = false;
    bool m_showConsole = false;
    bool m_showHierarchy = true;
    bool m_showInspector = true;
    bool m_showViewport = true;
    bool m_showNodeEditor = false;

    void drawMainMenuBar(Scene& scene);
    void drawHierarchy(Scene& scene);
    void drawInspector(Scene& scene);
    void drawViewport(Scene& scene);
    void drawConsole();
    void drawNodeGraph(nodes::NodeGraph& graph);
    void drawAssetBrowser();
};

} // namespace nexus::editor
