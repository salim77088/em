#include "Renderer.h"
#include "../core/Logger.h"
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <vector>
#include <cmath>

namespace nexus::graphics {

// ============ Mesh ============
Mesh::Mesh() = default;
Mesh::~Mesh() { destroy(); }

bgfx::VertexLayout Mesh::pcLayout() {
    bgfx::VertexLayout l;
    l.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
    return l;
}

bgfx::VertexLayout Mesh::pnuLayout() {
    bgfx::VertexLayout l;
    l.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    return l;
}

bool Mesh::upload(const bgfx::VertexLayout& layout,
                  const void* vertices, uint32_t vertexCount,
                  const void* indices, uint32_t indexCount,
                  bool index32) {
    destroy();
    if (!vertices || vertexCount == 0) return false;
    m_vertexCount = vertexCount;
    const bgfx::Memory* vmem = bgfx::makeRef(vertices, layout.getStride() * vertexCount);
    m_vbh = bgfx::createVertexBuffer(vmem, layout);
    if (!bgfx::isValid(m_vbh)) return false;

    if (indices && indexCount > 0) {
        m_indexed = true;
        m_indexCount = indexCount;
        size_t stride = index32 ? 4 : 2;
        const bgfx::Memory* imem = bgfx::makeRef(indices, stride * indexCount);
        m_ibh = bgfx::createIndexBuffer(imem, index32 ? BGFX_BUFFER_INDEX32 : 0);
    }
    return true;
}

void Mesh::destroy() {
    if (bgfx::isValid(m_vbh)) bgfx::destroy(m_vbh);
    if (bgfx::isValid(m_ibh)) bgfx::destroy(m_ibh);
    m_vbh.idx = bgfx::kInvalidHandle;
    m_ibh.idx = bgfx::kInvalidHandle;
    m_indexed = false;
    m_indexCount = 0;
    m_vertexCount = 0;
}

void Mesh::submit(bgfx::ProgramHandle program, uint64_t state) const {
    if (!bgfx::isValid(m_vbh)) return;
    bgfx::setState(state);
    bgfx::setVertexBuffer(0, m_vbh);
    if (m_indexed && bgfx::isValid(m_ibh)) {
        bgfx::setIndexBuffer(m_ibh);
    }
    bgfx::submit(0, program);
}

// ============ Shader ============
Shader::Shader() = default;
Shader::~Shader() { destroy(); }

bool Shader::loadFromMemory(const void* vsData, uint32_t vsSize, const void* fsData, uint32_t fsSize) {
    destroy();
    if (!vsData || !fsData || vsSize == 0 || fsSize == 0) return false;
    const bgfx::Memory* vsm = bgfx::makeRef(vsData, vsSize);
    const bgfx::Memory* fsm = bgfx::makeRef(fsData, fsSize);
    m_vs = bgfx::createShader(vsm);
    m_fs = bgfx::createShader(fsm);
    if (!bgfx::isValid(m_vs) || !bgfx::isValid(m_fs)) {
        NX_ERROR("Shader", "Failed to create shaders");
        return false;
    }
    bgfx::setName(m_vs, "vs");
    bgfx::setName(m_fs, "fs");
    m_program = bgfx::createProgram(m_vs, m_fs, true);
    return bgfx::isValid(m_program);
}

void Shader::destroy() {
    if (bgfx::isValid(m_program)) bgfx::destroy(m_program);
    if (bgfx::isValid(m_vs)) bgfx::destroy(m_vs);
    if (bgfx::isValid(m_fs)) bgfx::destroy(m_fs);
    m_program.idx = bgfx::kInvalidHandle;
    m_vs.idx = bgfx::kInvalidHandle;
    m_fs.idx = bgfx::kInvalidHandle;
}

// ============ Texture ============
Texture::Texture() = default;
Texture::~Texture() { destroy(); }

bool Texture::loadFromMemory(const void* data, uint32_t size, uint32_t flags) {
    destroy();
    if (!data || size == 0) return false;
    bimg::ImageContainer* img = bimg::imageParse(nullptr, data, size);
    if (!img) {
        NX_ERROR("Texture", "Failed to parse image");
        return false;
    }
    const bgfx::Memory* mem = bgfx::makeRef(img->m_data, img->m_size);
    m_handle = bgfx::createTexture2D(
        uint16_t(img->m_width), uint16_t(img->m_height),
        img->m_numMips > 1, 1,
        bgfx::TextureFormat::Enum(img->m_format),
        flags, mem);
    m_width = (uint16_t)img->m_width;
    m_height = (uint16_t)img->m_height;
    bimg::imageFree(img);
    return bgfx::isValid(m_handle);
}

bool Texture::create(uint16_t w, uint16_t h, bgfx::TextureFormat::Enum format, uint64_t flags, const void* data, uint32_t size) {
    destroy();
    const bgfx::Memory* mem = nullptr;
    if (data && size > 0) mem = bgfx::makeRef(data, size);
    m_handle = bgfx::createTexture2D(w, h, false, 1, format, flags, mem);
    m_width = w;
    m_height = h;
    return bgfx::isValid(m_handle);
}

void Texture::destroy() {
    if (bgfx::isValid(m_handle)) bgfx::destroy(m_handle);
    m_handle.idx = bgfx::kInvalidHandle;
    m_width = m_height = 0;
}

// ============ Renderer ============
Renderer::Renderer() = default;
Renderer::~Renderer() { shutdown(); }

bool Renderer::init(void* nativeWindowHandle, int width, int height) {
    if (m_initialized) return true;
    if (!nativeWindowHandle) {
        NX_ERROR("Renderer", "Null native window handle");
        return false;
    }

    SDL_SysWMinfo* wmi = (SDL_SysWMinfo*)nativeWindowHandle;

    memset(&m_pd, 0, sizeof(m_pd));
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    #if SDL_VIDEO_DRIVER_WAYLAND
    if (wmi->subsystem == SDL_SYSWM_WAYLAND) {
        m_pd.ndt = wmi->info.wl.display;
        m_pd.nwh = (void*)(uintptr_t)wmi->info.wl.surface;
        m_pd.type = bgfx::NativeWindowType::Wayland;
    } else
    #endif
    {
        m_pd.ndt = wmi->info.x11.display;
        m_pd.nwh = (void*)(uintptr_t)wmi->info.x11.window;
        m_pd.type = bgfx::NativeWindowType::X11;
    }
#elif BX_PLATFORM_OSX
    m_pd.nwh = wmi->info.cocoa.window;
    m_pd.ndt = nullptr;
    m_pd.type = bgfx::NativeWindowType::Cocoa;
#elif BX_PLATFORM_WINDOWS
    m_pd.nwh = wmi->info.win.window;
    m_pd.ndt = nullptr;
    m_pd.type = bgfx::NativeWindowType::Default;
#else
    m_pd.nwh = nativeWindowHandle;
#endif

    bgfx::setPlatformData(m_pd);

    bgfx::Init init;
    init.type = bgfx::RendererType::Count; // auto-select
    init.resolution.reset = BGFX_RESET_VSYNC;
    init.resolution.width = width;
    init.resolution.height = height;
    init.platformData = m_pd;

    if (!bgfx::init(init)) {
        NX_ERROR("Renderer", "bgfx::init failed");
        return false;
    }
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x1a1a2eff, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));

    m_width = width;
    m_height = height;
    m_initialized = true;
    NX_INFO("Renderer", "bgfx initialized %dx%d, renderer: %s", width, height, bgfx::getRendererName(bgfx::getRendererType()));
    return true;
}

void Renderer::shutdown() {
    if (!m_initialized) return;
    bgfx::frame();
    bgfx::shutdown();
    m_initialized = false;
    NX_INFO("Renderer", "bgfx shutdown");
}

void Renderer::beginFrame() {
    bgfx::touch(0);
}

void Renderer::endFrame() {
    bgfx::frame();
}

void Renderer::resize(int width, int height) {
    if (!m_initialized || (m_width == width && m_height == height)) return;
    bgfx::reset(width, height, BGFX_RESET_VSYNC);
    bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));
    m_width = width;
    m_height = height;
    NX_TRACE("Renderer", "Resized to %dx%d", width, height);
}

void Renderer::setViewClear(uint8_t view, uint32_t rgba, float depth, uint8_t stencil) {
    bgfx::setViewClear(view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, rgba, depth, stencil);
}

void Renderer::setViewRect(uint8_t view, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    bgfx::setViewRect(view, x, y, w, h);
}

void Renderer::setViewTransform(uint8_t view, const math::mat4& viewM, const math::mat4& projM) {
    bgfx::setViewTransform(view, &viewM[0][0], &projM[0][0]);
}

bgfx::ProgramHandle Renderer::loadEmbeddedShader(const char* name,
                                                  const void* vsData, uint32_t vsSize,
                                                  const void* fsData, uint32_t fsSize) {
    if (!vsData || !fsData || vsSize == 0 || fsSize == 0) return BGFX_INVALID_HANDLE;
    bgfx::ShaderHandle vs = bgfx::createShader(bgfx::makeRef(vsData, vsSize));
    bgfx::ShaderHandle fs = bgfx::createShader(bgfx::makeRef(fsData, fsSize));
    if (!bgfx::isValid(vs) || !bgfx::isValid(fs)) return BGFX_INVALID_HANDLE;
    bgfx::setName(vs, name);
    bgfx::setName(fs, name);
    bgfx::ProgramHandle prog = bgfx::createProgram(vs, fs, true);
    return prog;
}

void Renderer::drawMesh(uint8_t view, const Mesh& mesh, bgfx::ProgramHandle program,
                        const math::mat4& model, uint64_t state) {
    bgfx::setTransform(&model[0][0]);
    bgfx::setState(state);
    bgfx::setVertexBuffer(0, mesh.vbo());
    if (mesh.indexed() && bgfx::isValid(mesh.ibo())) {
        bgfx::setIndexBuffer(mesh.ibo());
    }
    bgfx::submit(view, program);
}

// ============ Primitive meshes ============
MeshRef createBoxMesh(float size) {
    auto mesh = std::make_shared<Mesh>();
    auto layout = Mesh::pnuLayout();
    struct V { float x,y,z; float nx,ny,nz; float u,v; };
    float s = size * 0.5f;
    // 24 vertices (4 per face * 6 faces)
    V verts[24] = {
        // +X
        { s,-s,-s, 1,0,0, 0,0 }, { s, s,-s, 1,0,0, 1,0 }, { s, s, s, 1,0,0, 1,1 }, { s,-s, s, 1,0,0, 0,1 },
        // -X
        {-s,-s, s,-1,0,0, 0,0 }, {-s, s, s,-1,0,0, 1,0 }, {-s, s,-s,-1,0,0, 1,1 }, {-s,-s,-s,-1,0,0, 0,1 },
        // +Y
        {-s, s,-s, 0,1,0, 0,0 }, {-s, s, s, 0,1,0, 1,0 }, { s, s, s, 0,1,0, 1,1 }, { s, s,-s, 0,1,0, 0,1 },
        // -Y
        {-s,-s, s, 0,-1,0, 0,0 }, { s,-s, s, 0,-1,0, 1,0 }, { s,-s,-s, 0,-1,0, 1,1 }, {-s,-s,-s, 0,-1,0, 0,1 },
        // +Z
        { s,-s, s, 0,0,1, 0,0 }, { s, s, s, 0,0,1, 1,0 }, {-s, s, s, 0,0,1, 1,1 }, {-s,-s, s, 0,0,1, 0,1 },
        // -Z
        {-s,-s,-s, 0,0,-1, 0,0 }, {-s, s,-s, 0,0,-1, 1,0 }, { s, s,-s, 0,0,-1, 1,1 }, { s,-s,-s, 0,0,-1, 0,1 },
    };
    uint16_t idx[36];
    for (int f = 0; f < 6; ++f) {
        idx[f*6+0] = f*4 + 0; idx[f*6+1] = f*4 + 1; idx[f*6+2] = f*4 + 2;
        idx[f*6+3] = f*4 + 0; idx[f*6+4] = f*4 + 2; idx[f*6+5] = f*4 + 3;
    }
    mesh->upload(layout, verts, 24, idx, 36, false);
    return mesh;
}

MeshRef createSphereMesh(float radius, uint32_t segments, uint32_t rings) {
    auto mesh = std::make_shared<Mesh>();
    auto layout = Mesh::pnuLayout();
    struct V { float x,y,z; float nx,ny,nz; float u,v; };
    std::vector<V> verts;
    std::vector<uint16_t> idx;

    for (uint32_t r = 0; r <= rings; ++r) {
        float v = float(r) / rings;
        float phi = v * float(M_PI);
        for (uint32_t s = 0; s <= segments; ++s) {
            float u = float(s) / segments;
            float theta = u * float(M_PI) * 2.0f;
            float x = std::sin(phi) * std::cos(theta);
            float y = std::cos(phi);
            float z = std::sin(phi) * std::sin(theta);
            verts.push_back({ x*radius, y*radius, z*radius, x, y, z, u, v });
        }
    }
    for (uint32_t r = 0; r < rings; ++r) {
        for (uint32_t s = 0; s < segments; ++s) {
            uint16_t a = uint16_t(r * (segments + 1) + s);
            uint16_t b = uint16_t(a + segments + 1);
            idx.push_back(a); idx.push_back(b); idx.push_back(a + 1);
            idx.push_back(a + 1); idx.push_back(b); idx.push_back(b + 1);
        }
    }
    mesh->upload(layout, verts.data(), (uint32_t)verts.size(), idx.data(), (uint32_t)idx.size(), false);
    return mesh;
}

MeshRef createPlaneMesh(float size) {
    auto mesh = std::make_shared<Mesh>();
    auto layout = Mesh::pnuLayout();
    struct V { float x,y,z; float nx,ny,nz; float u,v; };
    float s = size * 0.5f;
    V verts[4] = {
        { -s, 0, -s, 0,1,0, 0,0 },
        {  s, 0, -s, 0,1,0, 1,0 },
        {  s, 0,  s, 0,1,0, 1,1 },
        { -s, 0,  s, 0,1,0, 0,1 },
    };
    uint16_t idx[6] = { 0,1,2, 0,2,3 };
    mesh->upload(layout, verts, 4, idx, 6, false);
    return mesh;
}

MeshRef createQuadMesh(float size) {
    return createPlaneMesh(size);
}

} // namespace nexus::graphics
