#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <variant>
#include "../math/Math.h"

namespace nexus::nodes {

enum class PinKind { Input, Output };

struct Pin {
    int id = 0;
    std::string name;
    PinKind kind = PinKind::Input;
    std::string type; // "float", "vec3", "exec", etc.
};

struct Link {
    int id = 0;
    int fromPin = 0;
    int toPin = 0;
};

struct NodeProperty {
    std::string name;
    std::string type;
    std::string value;
};

struct Node {
    int id = 0;
    std::string title;
    std::string category;
    math::vec2 position{0,0};
    std::vector<Pin> inputs;
    std::vector<Pin> outputs;
    std::vector<NodeProperty> properties;
    bool selected = false;
};

class NodeGraph {
public:
    NodeGraph() = default;

    int addNode(const std::string& title, const std::string& category, float x, float y);
    int addPin(int nodeId, const std::string& name, PinKind kind, const std::string& type = "float");
    int addLink(int fromPin, int toPin);

    void removeNode(int id);
    void removeLink(int id);

    Node* findNode(int id);
    Pin* findPin(int id);
    Link* findLink(int id);

    const std::vector<Node>& nodes() const { return m_nodes; }
    const std::vector<Link>& links() const { return m_links; }

    void clear();

    bool saveToJson(const std::string& path) const;
    bool loadFromJson(const std::string& path);

    using ExecuteFn = std::function<void(const Node&, const std::vector<class NodeValue>&)>;
    void registerNodeType(const std::string& title, ExecuteFn fn);
    void executeAll();

private:
    int m_nextId = 1;
    std::vector<Node> m_nodes;
    std::vector<Link> m_links;
    std::unordered_map<std::string, ExecuteFn> m_executors;
};

class NodeValue {
public:
    enum class Type { None, Number, String, Vec3, Bool };
    Type type = Type::None;
    double num = 0;
    std::string str;
    math::vec3 v3;
    bool b = false;
};

} // namespace nexus::nodes
