#include "NodeGraph.h"
#include "../core/Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace nexus::nodes {

int NodeGraph::addNode(const std::string& title, const std::string& category, float x, float y) {
    Node n;
    n.id = m_nextId++;
    n.title = title;
    n.category = category;
    n.position = {x, y};
    m_nodes.push_back(n);
    return n.id;
}

int NodeGraph::addPin(int nodeId, const std::string& name, PinKind kind, const std::string& type) {
    Node* n = findNode(nodeId);
    if (!n) return -1;
    Pin p;
    p.id = m_nextId++;
    p.name = name;
    p.kind = kind;
    p.type = type;
    if (kind == PinKind::Input) n->inputs.push_back(p);
    else n->outputs.push_back(p);
    return p.id;
}

int NodeGraph::addLink(int fromPin, int toPin) {
    if (!findPin(fromPin) || !findPin(toPin)) return -1;
    Link l;
    l.id = m_nextId++;
    l.fromPin = fromPin;
    l.toPin = toPin;
    m_links.push_back(l);
    return l.id;
}

void NodeGraph::removeNode(int id) {
    m_nodes.erase(std::remove_if(m_nodes.begin(), m_nodes.end(),
        [id](const Node& n) { return n.id == id; }), m_nodes.end());
    m_links.erase(std::remove_if(m_links.begin(), m_links.end(),
        [id, this](const Link& l) {
            auto* p1 = findPin(l.fromPin);
            auto* p2 = findPin(l.toPin);
            return !p1 || !p2;
        }), m_links.end());
}

void NodeGraph::removeLink(int id) {
    m_links.erase(std::remove_if(m_links.begin(), m_links.end(),
        [id](const Link& l) { return l.id == id; }), m_links.end());
}

Node* NodeGraph::findNode(int id) {
    for (auto& n : m_nodes) if (n.id == id) return &n;
    return nullptr;
}

Pin* NodeGraph::findPin(int id) {
    for (auto& n : m_nodes) {
        for (auto& p : n.inputs) if (p.id == id) return &p;
        for (auto& p : n.outputs) if (p.id == id) return &p;
    }
    return nullptr;
}

Link* NodeGraph::findLink(int id) {
    for (auto& l : m_links) if (l.id == id) return &l;
    return nullptr;
}

void NodeGraph::clear() { m_nodes.clear(); m_links.clear(); }

bool NodeGraph::saveToJson(const std::string& path) const {
    using json = nlohmann::json;
    json j;
    j["nodes"] = json::array();
    for (const auto& n : m_nodes) {
        json nj;
        nj["id"] = n.id;
        nj["title"] = n.title;
        nj["category"] = n.category;
        nj["x"] = n.position.x;
        nj["y"] = n.position.y;
        nj["inputs"] = json::array();
        for (const auto& p : n.inputs) nj["inputs"].push_back({{"id", p.id}, {"name", p.name}, {"type", p.type}});
        nj["outputs"] = json::array();
        for (const auto& p : n.outputs) nj["outputs"].push_back({{"id", p.id}, {"name", p.name}, {"type", p.type}});
        j["nodes"].push_back(nj);
    }
    j["links"] = json::array();
    for (const auto& l : m_links) {
        j["links"].push_back({{"id", l.id}, {"from", l.fromPin}, {"to", l.toPin}});
    }
    std::ofstream f(path);
    if (!f) return false;
    f << j.dump(2);
    return true;
}

bool NodeGraph::loadFromJson(const std::string& path) {
    using json = nlohmann::json;
    std::ifstream f(path);
    if (!f) return false;
    json j;
    try { f >> j; } catch (...) { return false; }
    clear();
    if (j.contains("nodes")) {
        for (auto& nj : j["nodes"]) {
            Node n;
            n.id = nj.value("id", 0);
            n.title = nj.value("title", "");
            n.category = nj.value("category", "");
            n.position.x = nj.value("x", 0.0f);
            n.position.y = nj.value("y", 0.0f);
            if (nj.contains("inputs")) for (auto& pj : nj["inputs"]) {
                Pin p; p.id = pj.value("id", 0); p.name = pj.value("name", ""); p.type = pj.value("type", ""); p.kind = PinKind::Input;
                n.inputs.push_back(p);
            }
            if (nj.contains("outputs")) for (auto& pj : nj["outputs"]) {
                Pin p; p.id = pj.value("id", 0); p.name = pj.value("name", ""); p.type = pj.value("type", ""); p.kind = PinKind::Output;
                n.outputs.push_back(p);
            }
            m_nodes.push_back(n);
            m_nextId = std::max(m_nextId, n.id + 1);
        }
    }
    if (j.contains("links")) {
        for (auto& lj : j["links"]) {
            Link l; l.id = lj.value("id", 0); l.fromPin = lj.value("from", 0); l.toPin = lj.value("to", 0);
            m_links.push_back(l);
            m_nextId = std::max(m_nextId, l.id + 1);
        }
    }
    return true;
}

void NodeGraph::registerNodeType(const std::string& title, ExecuteFn fn) {
    m_executors[title] = fn;
}

void NodeGraph::executeAll() {
    std::vector<NodeValue> empty;
    for (const auto& n : m_nodes) {
        auto it = m_executors.find(n.title);
        if (it != m_executors.end()) it->second(n, empty);
    }
}

} // namespace nexus::nodes
