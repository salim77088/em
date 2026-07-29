#include "Scene.h"
#include "../core/Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace nexus::scene {

Scene::Scene() = default;
Scene::~Scene() = default;

entt::entity Scene::createEntity(const std::string& name) {
    auto e = m_registry.create();
    m_registry.emplace<Tag>(e, name.empty() ? std::string("Entity") : name, "");
    m_registry.emplace<Transform>(e);
    return e;
}

void Scene::destroyEntity(entt::entity e) {
    if (m_registry.valid(e)) m_registry.destroy(e);
}

void Scene::clear() {
    m_registry.clear();
}

bool Scene::saveToJson(const std::string& path) const {
    using json = nlohmann::json;
    json j;
    j["name"] = m_name;
    j["entities"] = json::array();

    auto& arr = j["entities"];
    m_registry.each([&](auto e) {
        json ej;
        ej["id"] = (uint32_t)entt::to_integral(e);
        if (auto* tag = m_registry.try_get<Tag>(e)) {
            ej["name"] = tag->name;
            ej["tag"] = tag->tag;
        }
        if (auto* t = m_registry.try_get<Transform>(e)) {
            ej["transform"] = {
                {"position", {t->position.x, t->position.y, t->position.z}},
                {"rotation", {t->rotation.x, t->rotation.y, t->rotation.z}},
                {"scale",    {t->scale.x, t->scale.y, t->scale.z}},
            };
        }
        if (auto* l = m_registry.try_get<Light>(e)) {
            ej["light"] = {
                {"type", (int)l->type},
                {"color", {l->color.x, l->color.y, l->color.z}},
                {"intensity", l->intensity},
                {"range", l->range},
            };
        }
        if (auto* c = m_registry.try_get<Camera3D>(e)) {
            ej["camera"] = {
                {"position", {c->camera.position.x, c->camera.position.y, c->camera.position.z}},
                {"target",   {c->camera.target.x, c->camera.target.y, c->camera.target.z}},
                {"fov", c->camera.fov},
                {"near", c->camera.nearZ},
                {"far", c->camera.farZ},
                {"primary", c->primary},
            };
        }
        if (m_registry.any_of<MeshRenderer>(e)) {
            ej["mesh"] = { {"has", true} };
        }
        arr.push_back(ej);
    });

    std::ofstream f(path);
    if (!f) { NX_ERROR("Scene", "Cannot open %s", path.c_str()); return false; }
    f << j.dump(2);
    NX_INFO("Scene", "Saved to %s (%d entities)", path.c_str(), (int)arr.size());
    return true;
}

bool Scene::loadFromJson(const std::string& path) {
    using json = nlohmann::json;
    std::ifstream f(path);
    if (!f) { NX_ERROR("Scene", "Cannot open %s", path.c_str()); return false; }
    json j;
    try { f >> j; } catch (std::exception& e) {
        NX_ERROR("Scene", "JSON parse error: %s", e.what());
        return false;
    }
    m_registry.clear();
    if (j.contains("name")) m_name = j["name"].get<std::string>();
    if (j.contains("entities")) {
        for (auto& ej : j["entities"]) {
            auto e = m_registry.create();
            std::string name = ej.value("name", "Entity");
            std::string tag = ej.value("tag", "");
            m_registry.emplace<Tag>(e, name, tag);
            Transform t;
            if (ej.contains("transform")) {
                auto& tj = ej["transform"];
                if (tj.contains("position")) { auto&p = tj["position"]; t.position = {p[0], p[1], p[2]}; }
                if (tj.contains("rotation")) { auto&r = tj["rotation"]; t.rotation = {r[0], r[1], r[2]}; }
                if (tj.contains("scale"))    { auto&s = tj["scale"];    t.scale    = {s[0], s[1], s[2]}; }
            }
            m_registry.emplace<Transform>(e, t);
            if (ej.contains("light")) {
                auto& lj = ej["light"];
                Light l;
                l.type = (Light::Type)lj.value("type", 0);
                if (lj.contains("color")) { auto&c = lj["color"]; l.color = {c[0], c[1], c[2]}; }
                l.intensity = lj.value("intensity", 1.0f);
                l.range = lj.value("range", 10.0f);
                m_registry.emplace<Light>(e, l);
            }
            if (ej.contains("camera")) {
                auto& cj = ej["camera"];
                Camera3D c;
                if (cj.contains("position")) { auto&p = cj["position"]; c.camera.position = {p[0], p[1], p[2]}; }
                if (cj.contains("target"))   { auto&t = cj["target"];   c.camera.target   = {t[0], t[1], t[2]}; }
                c.camera.fov = cj.value("fov", 60.0f);
                c.camera.nearZ = cj.value("near", 0.1f);
                c.camera.farZ = cj.value("far", 1000.0f);
                c.primary = cj.value("primary", false);
                m_registry.emplace<Camera3D>(e, c);
            }
        }
        NX_INFO("Scene", "Loaded %s (%d entities)", path.c_str(), (int)j["entities"].size());
    }
    return true;
}

} // namespace nexus::scene
