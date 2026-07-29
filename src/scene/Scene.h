#pragma once
#include <entt/entt.hpp>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "math/Math.h"
#include "../graphics/Renderer.h"

namespace nexus::scene {

struct Tag {
    std::string name = "Entity";
    std::string tag;
};

struct Transform {
    math::vec3 position{0.0f};
    math::vec3 rotation{0.0f};  // Euler degrees
    math::vec3 scale{1.0f};

    math::mat4 matrix() const {
        math::mat4 m = math::identity();
        m = math::translate(m, position);
        m = math::rotate(m, glm::radians(rotation.x), {1,0,0});
        m = math::rotate(m, glm::radians(rotation.y), {0,1,0});
        m = math::rotate(m, glm::radians(rotation.z), {0,0,1});
        m = math::scale(m, scale);
        return m;
    }
};

struct MeshRenderer {
    graphics::MeshRef mesh;
    bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
    uint64_t state = BGFX_STATE_DEFAULT;
    uint32_t tint = 0xFFFFFFFF;
    graphics::TextureRef texture;
};

struct Light {
    enum class Type { Directional, Point, Spot };
    Type type = Type::Directional;
    math::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
};

struct Camera3D {
    graphics::Camera camera;
    bool primary = true;
};

class Scene {
public:
    Scene();
    ~Scene();

    entt::entity createEntity(const std::string& name = "Entity");
    void destroyEntity(entt::entity e);

    template<typename T, typename... Args>
    T& addComponent(entt::entity e, Args&&... args) {
        return m_registry.emplace<T>(e, std::forward<Args>(args)...);
    }

    template<typename T>
    T& getComponent(entt::entity e) { return m_registry.get<T>(e); }

    template<typename T>
    bool hasComponent(entt::entity e) const { return m_registry.all_of<T>(e); }

    template<typename T>
    void removeComponent(entt::entity e) { m_registry.remove<T>(e); }

    entt::registry& registry() { return m_registry; }
    const entt::registry& registry() const { return m_registry; }

    void clear();
    bool empty() const { return m_registry.storage<entt::entity>().empty(); }
    size_t size() const { return m_registry.storage<entt::entity>().in_use(); }

    bool saveToJson(const std::string& path) const;
    bool loadFromJson(const std::string& path);

    const std::string& name() const { return m_name; }
    void setName(const std::string& n) { m_name = n; }

private:
    entt::registry m_registry;
    std::string m_name = "Scene";
};

using SceneRef = std::shared_ptr<Scene>;

} // namespace nexus::scene
