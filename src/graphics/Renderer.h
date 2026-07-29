#pragma once
#include <bgfx/bgfx.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "math/Math.h"

namespace nexus::graphics {

struct VertexPosColor {
    float x, y, z;
    uint32_t rgba;
};

struct VertexPosNormalUV {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

struct MeshData {
    std::vector<uint8_t> vertices;
    std::vector<uint8_t> indices;
    bgfx::VertexLayout layout;
    uint32_t indexCount = 0;
    bool indexed = false;
};

class Mesh {
public:
    Mesh();
    ~Mesh();

    bool upload(const bgfx::VertexLayout& layout,
                const void* vertices, uint32_t vertexCount,
                const void* indices, uint32_t indexCount,
                bool index32 = false);
    void destroy();

    void submit(bgfx::ProgramHandle program, uint64_t state = BGFX_STATE_DEFAULT) const;

    bool valid() const { return m_vbh.idx != bgfx::kInvalidHandle; }
    bgfx::VertexBufferHandle vbo() const { return m_vbh; }
    bgfx::IndexBufferHandle ibo() const { return m_ibh; }
    bool indexed() const { return m_indexed; }
    uint32_t indexCount() const { return m_indexCount; }
    uint32_t vertexCount() const { return m_vertexCount; }

    static bgfx::VertexLayout pcLayout();
    static bgfx::VertexLayout pnuLayout();

private:
    bgfx::VertexBufferHandle m_vbh = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle m_ibh = BGFX_INVALID_HANDLE;
    bool m_indexed = false;
    uint32_t m_indexCount = 0;
    uint32_t m_vertexCount = 0;
};
using MeshRef = std::shared_ptr<Mesh>;

class Shader {
public:
    Shader();
    ~Shader();
    bool loadFromMemory(const void* vsData, uint32_t vsSize, const void* fsData, uint32_t fsSize);
    void destroy();
    bgfx::ProgramHandle program() const { return m_program; }
    bool valid() const { return m_program.idx != bgfx::kInvalidHandle; }
private:
    bgfx::ShaderHandle m_vs = BGFX_INVALID_HANDLE;
    bgfx::ShaderHandle m_fs = BGFX_INVALID_HANDLE;
    bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
};

class Texture {
public:
    Texture();
    ~Texture();
    bool loadFromMemory(const void* data, uint32_t size, uint32_t flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
    bool create(uint16_t width, uint16_t height, bgfx::TextureFormat::Enum format, uint64_t flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP, const void* data = nullptr, uint32_t size = 0);
    void destroy();
    bgfx::TextureHandle handle() const { return m_handle; }
    bool valid() const { return m_handle.idx != bgfx::kInvalidHandle; }
    uint16_t width() const { return m_width; }
    uint16_t height() const { return m_height; }
private:
    bgfx::TextureHandle m_handle = BGFX_INVALID_HANDLE;
    uint16_t m_width = 0;
    uint16_t m_height = 0;
};
using TextureRef = std::shared_ptr<Texture>;

struct Camera {
    math::vec3 position = {0.0f, 2.0f, -5.0f};
    math::vec3 target = {0.0f, 0.0f, 0.0f};
    math::vec3 up = {0.0f, 1.0f, 0.0f};
    float fov = 60.0f;
    float nearZ = 0.1f;
    float farZ = 1000.0f;

    math::mat4 view() const { return math::lookAt(position, target, up); }
    math::mat4 projection(float aspect) const {
        return math::perspective(glm::radians(fov), aspect, nearZ, farZ);
    }
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init(void* nativeWindowHandle, int width, int height);
    void shutdown();

    void beginFrame();
    void endFrame();
    void resize(int width, int height);

    void setViewClear(uint8_t view, uint32_t rgba = 0x1a1a2eff, float depth = 1.0f, uint8_t stencil = 0);
    void setViewRect(uint8_t view, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void setViewTransform(uint8_t view, const math::mat4& viewM, const math::mat4& projM);

    bgfx::ProgramHandle loadEmbeddedShader(const char* name,
                                            const void* vsData, uint32_t vsSize,
                                            const void* fsData, uint32_t fsSize);

    void drawMesh(uint8_t view, const Mesh& mesh, bgfx::ProgramHandle program,
                  const math::mat4& model, uint64_t state = BGFX_STATE_DEFAULT);

    bool initialized() const { return m_initialized; }
    int width() const { return m_width; }
    int height() const { return m_height; }

private:
    bool m_initialized = false;
    int m_width = 0;
    int m_height = 0;
    bgfx::PlatformData m_pd{};
};

MeshRef createBoxMesh(float size = 1.0f);
MeshRef createSphereMesh(float radius = 0.5f, uint32_t segments = 16, uint32_t rings = 16);
MeshRef createPlaneMesh(float size = 5.0f);
MeshRef createQuadMesh(float size = 1.0f);

} // namespace nexus::graphics
