#include "AssetManager.h"
#include "../core/Logger.h"
#include <fstream>
#include <sstream>
#include <stb_image.h>

namespace nexus::assets {

std::string AssetManager::loadFileText(const std::string& path) {
    std::ifstream f(m_root + "/" + path);
    if (!f) { NX_WARN("Assets", "Cannot open text file: %s/%s", m_root.c_str(), path.c_str()); return ""; }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::vector<uint8_t> AssetManager::loadFileBinary(const std::string& path) {
    std::ifstream f(m_root + "/" + path, std::ios::binary | std::ios::ate);
    if (!f) { NX_WARN("Assets", "Cannot open bin file: %s/%s", m_root.c_str(), path.c_str()); return {}; }
    auto size = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> data((size_t)size);
    f.read((char*)data.data(), size);
    return data;
}

graphics::TextureRef AssetManager::loadTexture(const std::string& path) {
    auto it = m_textures.find(path);
    if (it != m_textures.end()) return it->second;
    auto bytes = loadFileBinary(path);
    if (bytes.empty()) return nullptr;

    int w = 0, h = 0, channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(bytes.data(), (int)bytes.size(), &w, &h, &channels, 4);
    if (!pixels) { NX_ERROR("Assets", "stbi_load failed: %s", path.c_str()); return nullptr; }

    auto tex = std::make_shared<graphics::Texture>();
    if (!tex->create(uint16_t(w), uint16_t(h), bgfx::TextureFormat::RGBA8,
                      BGFX_TEXTURE_NONE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
                      pixels, w * h * 4)) {
        NX_ERROR("Assets", "Texture create failed: %s", path.c_str());
        stbi_image_free(pixels);
        return nullptr;
    }
    stbi_image_free(pixels);
    m_textures[path] = tex;
    return tex;
}

graphics::MeshRef AssetManager::loadMesh(const std::string& path) {
    auto it = m_meshes.find(path);
    if (it != m_meshes.end()) return it->second;
    // Basic OBJ loader — supports positions and faces (triangulated)
    auto text = loadFileText(path);
    if (text.empty()) return nullptr;

    auto mesh = std::make_shared<graphics::Mesh>();
    auto layout = graphics::Mesh::pnuLayout();
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> uvs;
    struct V { float x,y,z, nx,ny,nz, u,v; };
    std::vector<V> verts;
    std::vector<uint16_t> idx;

    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        std::istringstream ls(line);
        std::string tag; ls >> tag;
        if (tag == "v") {
            float x, y, z; ls >> x >> y >> z;
            positions.push_back(x); positions.push_back(y); positions.push_back(z);
        } else if (tag == "vn") {
            float x, y, z; ls >> x >> y >> z;
            normals.push_back(x); normals.push_back(y); normals.push_back(z);
        } else if (tag == "vt") {
            float u, v; ls >> u >> v;
            uvs.push_back(u); uvs.push_back(v);
        } else if (tag == "f") {
            int faces[3] = {0,0,0};
            for (int i = 0; i < 3; ++i) {
                std::string tok; ls >> tok;
                int vi = 0, ti = 0, ni = 0;
                size_t p1 = tok.find('/');
                if (p1 == std::string::npos) vi = std::stoi(tok);
                else {
                    vi = std::stoi(tok.substr(0, p1));
                    size_t p2 = tok.find('/', p1 + 1);
                    if (p2 != std::string::npos) {
                        if (p2 > p1 + 1) ti = std::stoi(tok.substr(p1 + 1, p2 - p1 - 1));
                        if (tok.size() > p2 + 1) ni = std::stoi(tok.substr(p2 + 1));
                    } else if (p1 + 1 < tok.size()) {
                        ti = std::stoi(tok.substr(p1 + 1));
                    }
                }
                if (vi < 0) vi = (int)positions.size()/3 + vi + 1;
                if (ti < 0) ti = (int)uvs.size()/2 + ti + 1;
                if (ni < 0) ni = (int)normals.size()/3 + ni + 1;
                V v{};
                if (vi > 0) {
                    v.x = positions[(vi-1)*3 + 0];
                    v.y = positions[(vi-1)*3 + 1];
                    v.z = positions[(vi-1)*3 + 2];
                }
                if (ni > 0) {
                    v.nx = normals[(ni-1)*3 + 0];
                    v.ny = normals[(ni-1)*3 + 1];
                    v.nz = normals[(ni-1)*3 + 2];
                }
                if (ti > 0) {
                    v.u = uvs[(ti-1)*2 + 0];
                    v.v = uvs[(ti-1)*2 + 1];
                }
                verts.push_back(v);
                faces[i] = (int)verts.size() - 1;
            }
            idx.push_back((uint16_t)faces[0]);
            idx.push_back((uint16_t)faces[1]);
            idx.push_back((uint16_t)faces[2]);
        }
    }
    mesh->upload(layout, verts.data(), (uint32_t)verts.size(), idx.data(), (uint32_t)idx.size(), false);
    m_meshes[path] = mesh;
    return mesh;
}

} // namespace nexus::assets
