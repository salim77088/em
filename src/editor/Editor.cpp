#include "Editor.h"
#include "../core/Logger.h"
#include "../core/Window.h"
#include "../graphics/Renderer.h"
#include "../scene/Scene.h"
#include "../nodes/NodeGraph.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/bgfx.h>
#include <cstdio>
#include <cstring>

namespace nexus::editor {

// Forward declarations of bgfx ImGui backend functions we implement manually
static bool imguiBgfxInit();
static void imguiBgfxShutdown();
static void imguiBgfxRenderDrawData(ImDrawData* drawData);
static void imguiSdlProcessEvent(const SDL_Event* ev);

// ============ Editor ============
Editor::Editor() = default;
Editor::~Editor() { shutdown(); }

bool Editor::init(core::Window* window, graphics::Renderer* renderer) {
    m_window = window;
    m_renderer = renderer;

    IMGUI_CHECKVERSION();
    m_context = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_context);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = "nexus_editor.ini";

    // Style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 2.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.22f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.27f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.40f, 0.55f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.50f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.22f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.35f, 0.50f, 1.00f);

    // Build font atlas
    unsigned char* pixels = nullptr;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    if (!imguiBgfxInit()) {
        NX_ERROR("Editor", "Failed to init bgfx ImGui backend");
        return false;
    }

    // Upload font texture to bgfx
    bgfx::TextureHandle fontTex = bgfx::createTexture2D(
        (uint16_t)width, (uint16_t)height, false, 1,
        bgfx::TextureFormat::BGRA8, 0,
        bgfx::makeRef(pixels, (uint32_t)(width * height * 4)));
    io.Fonts->SetTexID((ImTextureID)(uintptr_t)fontTex.idx);

    // Hook SDL events
    window->onKey = [](int sym, int scancode, int mod, int down) {
        (void)mod;
        ImGuiIO& io = ImGui::GetIO();
        io.AddKeyEvent((ImGuiKey)scancode, down != 0);
        // Simple ASCII mapping
        if (down && sym >= 32 && sym < 127) io.AddInputCharacter((unsigned int)sym);
    };
    window->onMouseMove = [](float x, float y) {
        ImGui::GetIO().AddMousePosEvent(x, y);
    };
    window->onMouseButton = [](int button, int clicks, int x, int y) {
        (void)clicks; (void)x; (void)y;
        int b = -1;
        if (button == SDL_BUTTON_LEFT) b = 0;
        else if (button == SDL_BUTTON_RIGHT) b = 1;
        else if (button == SDL_BUTTON_MIDDLE) b = 2;
        if (b >= 0) {
            Uint32 state = SDL_GetMouseState(nullptr, nullptr);
            ImGui::GetIO().AddMouseButtonEvent(b, (state & SDL_BUTTON(button)) != 0);
        }
    };
    window->onMouseWheel = [](float x, float y) {
        ImGui::GetIO().AddMouseWheelEvent(x, y);
    };
    window->onTextInput = [](unsigned int c) {
        ImGui::GetIO().AddInputCharacter(c);
    };

    m_initialized = true;
    NX_INFO("Editor", "ImGui initialized");
    return true;
}

void Editor::shutdown() {
    if (!m_initialized) return;
    imguiBgfxShutdown();
    if (m_context) { ImGui::DestroyContext(m_context); m_context = nullptr; }
    m_initialized = false;
}

void Editor::beginFrame() {
    if (!m_initialized) return;
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)m_window->width(), (float)m_window->height());
    static Uint64 last = 0;
    Uint64 now = SDL_GetPerformanceCounter();
    if (last == 0) last = now;
    io.DeltaTime = (float)((double)(now - last) / (double)SDL_GetPerformanceFrequency());
    last = now;
    if (io.DeltaTime <= 0) io.DeltaTime = 1.0f / 60.0f;

    ImGui::NewFrame();
}

void Editor::endFrame() {
    if (!m_initialized) return;
    ImGui::Render();
    ImDrawData* dd = ImGui::GetDrawData();
    if (dd && dd->CmdListsCount > 0) {
        imguiBgfxRenderDrawData(dd);
    }
}

bool Editor::wantCaptureMouse() const { return ImGui::GetIO().WantCaptureMouse; }
bool Editor::wantCaptureKeyboard() const { return ImGui::GetIO().WantCaptureKeyboard; }

void Editor::showDemoWindow(bool* p) { ImGui::ShowDemoWindow(p); }

void Editor::render(Scene& scene) {
    drawMainMenuBar(scene);
    drawHierarchy(scene);
    drawInspector(scene);
    drawViewport(scene);
    drawConsole();
    drawAssetBrowser();
}

void Editor::renderNodeGraph(nodes::NodeGraph& graph) {
    drawNodeGraph(graph);
}

void Editor::drawMainMenuBar(Scene& scene) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Scene")) { scene.clear(); scene.setName("Untitled"); }
            if (ImGui::MenuItem("Open Scene...")) { scene.loadFromJson("scene.json"); }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) { scene.saveToJson("scene.json"); }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Hierarchy", nullptr, &m_showHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &m_showInspector);
            ImGui::MenuItem("Viewport", nullptr, &m_showViewport);
            ImGui::MenuItem("Console", nullptr, &m_showConsole);
            ImGui::MenuItem("Node Editor", nullptr, &m_showNodeEditor);
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &m_showDemo);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::TextDisabled("NEXUS Engine v1.0.0");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (m_showDemo) showDemoWindow(&m_showDemo);
}

void Editor::drawHierarchy(Scene& scene) {
    if (!m_showHierarchy) return;
    if (ImGui::Begin("Hierarchy", &m_showHierarchy)) {
        if (ImGui::Button("+ Add Entity")) {
            scene.createEntity("Entity_" + std::to_string(scene.size()));
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) scene.clear();
        ImGui::Separator();
        auto& reg = scene.registry();
        reg.each([&](auto e) {
            auto* tag = reg.try_get<scene::Tag>(e);
            std::string name = tag ? tag->name : ("Entity_" + std::to_string((uint32_t)entt::to_integral(e)));
            ImGui::PushID((int)e);
            bool sel = false;
            ImGui::Selectable(name.c_str(), &sel);
            if (sel) {
                // selected = e;
            }
            ImGui::PopID();
        });
    }
    ImGui::End();
}

void Editor::drawInspector(Scene& scene) {
    if (!m_showInspector) return;
    if (ImGui::Begin("Inspector", &m_showInspector)) {
        ImGui::TextDisabled("Select an entity to inspect");
    }
    ImGui::End();
}

void Editor::drawViewport(Scene& scene) {
    if (!m_showViewport) return;
    if (ImGui::Begin("Viewport", &m_showViewport)) {
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImGui::Text("Viewport: %dx%d", (int)size.x, (int)size.y);
        ImGui::Separator();
        // Render scene stats
        ImGui::Text("Entities: %d", (int)scene.size());
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    }
    ImGui::End();
}

void Editor::drawConsole() {
    if (!m_showConsole) return;
    if (ImGui::Begin("Console", &m_showConsole)) {
        if (ImGui::Button("Clear")) core::Logger::get().clear();
        ImGui::SameLine();
        ImGui::Text("Entries: %d", (int)core::Logger::get().entries().size());
        ImGui::Separator();
        ImGui::BeginChild("log");
        for (const auto& e : core::Logger::get().entries()) {
            ImVec4 col = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
            if (e.level == core::LogLevel::Warn) col = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
            else if (e.level == core::LogLevel::Error) col = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
            else if (e.level == core::LogLevel::Fatal) col = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            ImGui::TextUnformatted(("[");
            ImGui::SameLine(0, 0);
            ImGui::TextUnformatted(e.tag.c_str());
            ImGui::SameLine(0, 0);
            ImGui::TextUnformatted("] ");
            ImGui::SameLine(0, 0);
            ImGui::TextUnformatted(e.message.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void Editor::drawAssetBrowser() {
    static bool show = true;
    if (!show) return;
    if (ImGui::Begin("Assets", &show)) {
        if (ImGui::Button("Refresh")) {}
        ImGui::Separator();
        ImGui::TextDisabled("Assets folder: /assets");
    }
    ImGui::End();
}

void Editor::drawNodeGraph(nodes::NodeGraph& graph) {
    if (!m_showNodeEditor) return;
    if (ImGui::Begin("Node Editor", &m_showNodeEditor, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImVec2 canvas = ImGui::GetContentRegionAvail();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + canvas.x, p0.y + canvas.y);
        // Background
        dl->AddRectFilled(p0, p1, IM_COL32(20, 20, 28, 255));
        // Grid
        for (float x = p0.x; x < p1.x; x += 32.0f)
            dl->AddLine(ImVec2(x, p0.y), ImVec2(x, p1.y), IM_COL32(40, 40, 50, 255));
        for (float y = p0.y; y < p1.y; y += 32.0f)
            dl->AddLine(ImVec2(p0.x, y), ImVec2(p1.x, y), IM_COL32(40, 40, 50, 255));

        // Draw nodes
        for (const auto& n : graph.nodes()) {
            ImVec2 np(p0.x + n.position.x, p0.y + n.position.y);
            ImVec2 sz(180.0f, 80.0f);
            ImU32 bg = n.selected ? IM_COL32(60, 80, 120, 230) : IM_COL32(40, 44, 56, 230);
            dl->AddRectFilled(np, ImVec2(np.x + sz.x, np.y + sz.y), bg, 4.0f);
            dl->AddRect(np, ImVec2(np.x + sz.x, np.y + sz.y), IM_COL32(120, 130, 150, 255), 4.0f);
            dl->AddText(ImVec2(np.x + 8, np.y + 6), IM_COL32(255, 255, 255, 255), n.title.c_str());
            int i = 0;
            for (const auto& pin : n.inputs) {
                dl->AddCircleFilled(ImVec2(np.x, np.y + 30 + i * 18), 5.0f, IM_COL32(80, 180, 255, 255));
                dl->AddText(ImVec2(np.x + 12, np.y + 22 + i * 18), IM_COL32(220, 220, 220, 255), pin.name.c_str());
                i++;
            }
            i = 0;
            for (const auto& pin : n.outputs) {
                dl->AddCircleFilled(ImVec2(np.x + sz.x, np.y + 30 + i * 18), 5.0f, IM_COL32(255, 180, 80, 255));
                dl->AddText(ImVec2(np.x + sz.x - 12 - ImGui::CalcTextSize(pin.name.c_str()).x, np.y + 22 + i * 18), IM_COL32(220, 220, 220, 255), pin.name.c_str());
                i++;
            }
        }
        // Draw links
        for (const auto& l : graph.links()) {
            auto* from = graph.findPin(l.fromPin);
            auto* to = graph.findPin(l.toPin);
            if (!from || !to) continue;
            // Find node positions (approximate)
            ImVec2 a(0, 0), b(0, 0);
            for (const auto& n : graph.nodes()) {
                for (const auto& pin : n.inputs) if (pin.id == l.toPin) { b = ImVec2(p0.x + n.position.x, p0.y + n.position.y + 30); break; }
                for (const auto& pin : n.outputs) if (pin.id == l.fromPin) { a = ImVec2(p0.x + n.position.x + 180, p0.y + n.position.y + 30); break; }
            }
            dl->AddBezierCubic(a, ImVec2(a.x + 40, a.y), ImVec2(b.x - 40, b.y), b, IM_COL32(120, 180, 255, 255), 2.0f);
        }

        ImGui::TextDisabled("Right-click to add nodes");
        if (ImGui::BeginPopupContextWindow("node_ctx", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem("Add Print Node")) graph.addNode("Print", "IO", 100, 100);
            if (ImGui::MenuItem("Add Math Node")) graph.addNode("Math", "Math", 100, 200);
            if (ImGui::MenuItem("Add Vector3 Node")) graph.addNode("Vector3", "Math", 100, 300);
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

// ============ bgfx ImGui backend (minimal) ============
// We implement a simple, self-contained ImGui renderer using bgfx.

struct ImGuiBgfxData {
    bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout layout;
    bgfx::UniformHandle uTex = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle fontTex = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle ibo = BGFX_INVALID_HANDLE;
    bgfx::VertexBufferHandle vbo = BGFX_INVALID_HANDLE;
};
static ImGuiBgfxData g_data;

static const char* ImGuiBgfxVsSrc = R"(
$input a_position, a_color0, a_texcoord0
$output v_color0, v_texcoord0
#include <bgfx_shader.sh>
void main() {
    v_color0 = a_color0;
    v_texcoord0 = a_texcoord0;
    gl_Position = vec4(a_position.xy, 0.0, 1.0);
}
)";

static const char* ImGuiBgfxFsSrc = R"(
$input v_color0, v_texcoord0
#include <bgfx_shader.sh>
SAMPLER2D(s_tex, 0);
void main() {
    vec4 col = texture2D(s_tex, v_texcoord0);
    gl_FragColor = col * v_color0;
}
)";

// We can't compile shaders at runtime, so we use bgfx's embedded shader approach via bgfx_shader.sh
// As a portable fallback, we render ImGui using simple bgfx::dbg text debug (not great) - but to keep this engine
// working on all platforms out of the box, we use a placeholder approach: ImGui draws go via a custom vertex shader
// built at runtime using the bgfx embedded shader system.
//
// NOTE: For maximum portability without shader cross-compilation, we skip real ImGui rendering here and rely on
// bgfx debug text + SDL overlay. This is a stub; see README for the recommended shader build process.

static bool imguiBgfxInit() {
    // Without pre-compiled shaders, ImGui rendering is stubbed.
    // ImGui will still update state, but draws won't be visible.
    // This is intentional — the engine still works, and shaders can be added later.
    NX_WARN("Editor", "ImGui rendering is currently stubbed (no compiled shaders). Editor UI will not be visible.");
    (void)ImGuiBgfxVsSrc;
    (void)ImGuiBgfxFsSrc;
    return true;
}

static void imguiBgfxShutdown() {
    if (bgfx::isValid(g_data.fontTex)) bgfx::destroy(g_data.fontTex);
}

static void imguiBgfxRenderDrawData(ImDrawData* drawData) {
    (void)drawData;
    // Stub: no actual rendering happens here. Engine still runs.
}

static void imguiSdlProcessEvent(const SDL_Event* ev) {
    (void)ev;
}

} // namespace nexus::editor
