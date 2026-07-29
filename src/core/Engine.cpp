#include "Engine.h"
#include "Logger.h"
#include <SDL.h>
#include <chrono>

namespace nexus::core {

Engine::Engine() {
    m_window = std::make_unique<Window>();
    m_renderer = std::make_unique<graphics::Renderer>();
    m_scene = std::make_unique<scene::Scene>();
    m_editor = std::make_unique<editor::Editor>();
    m_nodeGraph = std::make_unique<nodes::NodeGraph>();
}
Engine::~Engine() { shutdown(); }

bool Engine::init(const std::string& title, int w, int h) {
    if (m_initialized) return true;

    WindowDesc desc;
    desc.title = title;
    desc.width = w;
    desc.height = h;
    if (!m_window->create(desc)) {
        NX_FATAL("Engine", "Window creation failed");
        return false;
    }

    if (!m_renderer->init(m_window->nativeHandle(), m_window->width(), m_window->height())) {
        NX_FATAL("Engine", "Renderer init failed");
        return false;
    }

    m_window->onResize = [this](int w, int h) {
        m_renderer->resize(w, h);
    };

    if (!m_editor->init(m_window.get(), m_renderer.get())) {
        NX_WARN("Engine", "Editor init had warnings");
    }

    // Setup default scene with a cube
    auto e = m_scene->createEntity("Cube");
    m_scene->addComponent<scene::MeshRenderer>(e);
    m_scene->addComponent<scene::Light>(e);

    auto cam = m_scene->createEntity("MainCamera");
    auto& cam3d = m_scene->addComponent<scene::Camera3D>(cam);
    cam3d.camera.position = {0.0f, 2.0f, -5.0f};
    cam3d.primary = true;

    m_initialized = true;
    m_running = true;
    NX_INFO("Engine", "NEXUS Engine initialized");
    return true;
}

void Engine::shutdown() {
    if (!m_initialized) return;
    m_running = false;
    m_editor->shutdown();
    m_renderer->shutdown();
    m_window->destroy();
    if (SDL_WasInit(0)) SDL_Quit();
    m_initialized = false;
    NX_INFO("Engine", "NEXUS Engine shutdown");
}

bool Engine::run() {
    if (!m_initialized || !m_running) return false;
    using clock = std::chrono::steady_clock;
    static auto last = clock::now();
    auto now = clock::now();
    m_delta = std::chrono::duration<float>(now - last).count();
    last = now;
    if (m_delta > 0.1f) m_delta = 0.016f;
    m_fps = 1.0f / m_delta;
    m_frameCount++;

    if (!m_window->pollEvents()) {
        m_running = false;
        return false;
    }
    update();
    render();
    return true;
}

void Engine::update() {
    // Update scene
    // (no per-frame logic for now besides input state)
}

void Engine::render() {
    if (m_window->isMinimized()) {
        return;
    }
    m_renderer->beginFrame();

    // Clear
    m_renderer->setViewClear(0, 0x1a1a2eff, 1.0f, 0);
    m_renderer->setViewRect(0, 0, 0, (uint16_t)m_window->width(), (uint16_t)m_window->height());

    // Find primary camera
    scene::Camera3D* primaryCam = nullptr;
    auto& reg = m_scene->registry();
    auto view = reg.view<scene::Camera3D>();
    for (auto e : view) {
        auto& c = view.get<scene::Camera3D>(e);
        if (c.primary) { primaryCam = &c; break; }
    }

    graphics::Camera fallbackCam;
    fallbackCam.position = {0.0f, 2.0f, -5.0f};
    fallbackCam.target = {0.0f, 0.0f, 0.0f};
    graphics::Camera& cam = primaryCam ? primaryCam->camera : fallbackCam;

    math::mat4 viewM = cam.view();
    math::mat4 projM = cam.projection(m_window->aspect());
    m_renderer->setViewTransform(0, viewM, projM);

    // Render meshes (no shader for now, but we touch the view)
    auto meshView = reg.view<scene::Transform, scene::MeshRenderer>();
    for (auto e : meshView) {
        auto& t = meshView.get<scene::Transform>(e);
        auto& mr = meshView.get<scene::MeshRenderer>(e);
        if (mr.mesh) {
            // bgfx::setState(BGFX_STATE_DEFAULT);
            // Without a shader program, we can't actually render the mesh yet.
            // The mesh is uploaded and ready; render logic activates once shaders are added.
            (void)t;
        }
    }

    // Render editor UI
    m_editor->beginFrame();
    m_editor->render(*m_scene);
    if (true) m_editor->renderNodeGraph(*m_nodeGraph);
    m_editor->endFrame();

    m_renderer->endFrame();
}

} // namespace nexus::core
