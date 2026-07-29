#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "../graphics/Renderer.h"

namespace nexus::assets {

class AssetManager {
public:
    static AssetManager& get() {
        static AssetManager instance;
        return instance;
    }

    void setRoot(const std::string& root) { m_root = root; }
    const std::string& root() const { return m_root; }

    graphics::TextureRef loadTexture(const std::string& path);
    graphics::MeshRef loadMesh(const std::string& path);

    std::string loadFileText(const std::string& path);
    std::vector<uint8_t> loadFileBinary(const std::string& path);

    void clear() { m_textures.clear(); m_meshes.clear(); }

private:
    AssetManager() = default;
    std::string m_root = "assets";
    std::unordered_map<std::string, graphics::TextureRef> m_textures;
    std::unordered_map<std::string, graphics::MeshRef> m_meshes;
};

} // namespace nexus::assets
