#ifndef SCENE_CULLER_HPP
#define SCENE_CULLER_HPP

#include <DirectXCollision.h>

#include <vector>
#include <array>
#include <memory>
#include <utility>

using namespace DirectX;

class Component;
struct Render_view;
class RenderQueue;

class Octree {
    static constexpr int MAX_DEPTH       = 6;
    static constexpr int SPLIT_THRESHOLD = 4;

    struct Node {
        BoundingBox bounds{};
        std::array<std::unique_ptr<Node>, 8> children = {};
        std::vector<Component *> components; // TODO: Store component bounding box? NodeItem or smth

        bool IsLeaf() const { return this->children[0] == nullptr; }
    };

    std::unique_ptr<Node> root;

    // index: bit 0 = +x, bit 1 = +y, bit 2 = +z
    static BoundingBox GetChildBounds(const BoundingBox &parent, int index);

    void Insert(Node *node, Component *component, const BoundingBox &componentBounds, int depth);

    void CollectAll(const Node *node, std::vector<Component *> &outComponents) const;

    void Query(const Node *node, const BoundingFrustum &frustum, std::vector<Component *> &outComponents) const;

public:
    Octree() = default;
    ~Octree() = default;

    void Build(const std::vector<std::pair<Component *, BoundingBox>> &items, const BoundingBox &sceneBounds);

    void Query(const BoundingFrustum &frustum, std::vector<Component *> &outComponents) const;

    void Clear();
    int Count() const;

    bool IsBuilt() const { return this->root != nullptr; }

    // CONTINUE HERE! Add debug draw
};

class SceneCuller {
    Octree octree;

    std::vector<std::pair<Component *, BoundingBox>> staticRenderablesPending;
    std::vector<Component *> dynamicRenderables;

    bool needsRebuild = false;

public:
    SceneCuller() = default;
    ~SceneCuller() = default;

    void AddComponent(Component *component, bool isStatic);
    void RemoveComponent(Component *component);

    void Build();

    void GatherVisibility(std::vector<Render_view> &views) const; // TODO: Rename. Go through the rest of the code as well

    void Clear();

    bool NeedsRebuild() const { return this->needsRebuild; }
};

#endif