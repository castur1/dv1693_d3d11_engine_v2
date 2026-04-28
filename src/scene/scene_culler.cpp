#include "scene_culler.hpp"
#include "scene/component.hpp"
#include "core/logging.hpp"
#include "rendering/render_view.hpp"
#include "debugging/debug.hpp"
#include "scene/entity.hpp"
#include "debugging/debug_draw.hpp"

#include <algorithm>

BoundingBox Octree::GetChildBounds(const BoundingBox &parent, int index) {
    XMFLOAT3 extents = {
        parent.Extents.x * 0.5f,
        parent.Extents.y * 0.5f,
        parent.Extents.z * 0.5f
    };
    XMFLOAT3 centre = {
        parent.Center.x + ((index & 1) ? extents.x : -extents.x),
        parent.Center.y + ((index & 2) ? extents.y : -extents.y),
        parent.Center.z + ((index & 4) ? extents.z : -extents.z)
    };

    return BoundingBox(centre, extents);
}

void Octree::Insert(Node *node, Component *component, const BoundingBox &componentBounds, int depth) {
    if (!node)
        return;

    if (!node->IsLeaf()) {
        for (auto &child : node->children)
            if (child && child->bounds.Intersects(componentBounds))
                this->Insert(child.get(), component, componentBounds, depth + 1);
        
        return;
    }

    node->components.push_back(component);

    if (depth >= MAX_DEPTH || node->components.size() <= SPLIT_THRESHOLD)
        return;

    for (int i = 0; i < node->children.size(); ++i)
        node->children[i] = std::make_unique<Node>(Node{this->GetChildBounds(node->bounds, i)});

    std::vector<Component *> toRedistribute = std::move(node->components);

    for (Component *comp : toRedistribute) {
        BoundingBox compBounds;
        if (!comp->GetWorldBounds(compBounds))
            continue;

        for (auto &child : node->children)
            if (child && child->bounds.Intersects(compBounds))
                this->Insert(child.get(), comp, compBounds, depth + 1);
    }
}

void Octree::CollectAll(const Node *node, std::vector<Component *> &outComponents) const {
    if (!node)
        return;

    if (node->IsLeaf()) {
        for (Component *component : node->components)
            if (component->GetOwner()->IsActive() && component->isActive)
                outComponents.push_back(component);

        return;
    }

    for (auto &child : node->children)
        this->CollectAll(child.get(), outComponents);
}

void Octree::Query(const Node *node, const BoundingFrustum &frustum, std::vector<Component *> &outComponents) const {
    if (!node)
        return;

    ContainmentType contains = frustum.Contains(node->bounds);

    if (contains == DISJOINT)
        return;

    if (contains == CONTAINS) {
        this->CollectAll(node, outComponents);
        return;
    }

    if (node->IsLeaf()) {
        for (Component *component : node->components) {
            if (!component->GetOwner()->IsActive())
                continue;

            BoundingBox bounds;
            component->GetWorldBounds(bounds);

            if (frustum.Contains(bounds) != DISJOINT)
                outComponents.push_back(component);
        }

        return;
    }

    for (auto &child : node->children)
        this->Query(child.get(), frustum, outComponents);
}

void Octree::Build(const std::vector<std::pair<Component *, BoundingBox>> &items, const BoundingBox &sceneBounds) {
    this->root = std::make_unique<Node>(Node{sceneBounds});

    for (auto &[component, bounds] : items)
        if (this->root->bounds.Intersects(bounds))
            this->Insert(this->root.get(), component, bounds, 0);

    LogInfo("Octree built: %zu static renderables\n", items.size());
}

void Octree::Query(const BoundingFrustum &frustum, std::vector<Component *> &outComponents) const {
    this->Query(this->root.get(), frustum, outComponents);
}

void Octree::QueryAll(std::vector<Component *> &outComponents) const {
    this->CollectAll(this->root.get(), outComponents);
}

void Octree::Clear() {
    this->root.reset();
}

int Octree::Count() const {
    std::vector<Component *> components;
    this->CollectAll(this->root.get(), components);
    std::sort(components.begin(), components.end());
    components.erase(std::unique(components.begin(), components.end()), components.end());
    return components.size();
}

void Octree::DebugDrawNode(const Node *node, int depth) {
    if (!node)
        return;

    float colour = (float)(MAX_DEPTH - depth * 0.6f) / MAX_DEPTH;
    DebugDraw::Box(node->bounds, {colour, colour * colour * 0.6f, colour * colour * 0.8f, 1.0f});

    if (node->IsLeaf()) {
        for (Component *component : node->components) {
            if (!component->GetOwner()->IsActive())
                continue;

            BoundingBox bounds;
            component->GetWorldBounds(bounds);

            DebugDraw::Box(bounds, {1.0f, 0.0, 0.0f, 1.0f});
        }
        return;
    }

    for (const auto &child : node->children)
        this->DebugDrawNode(child.get(), depth + 1);
}

void Octree::DebugDraw() {
    this->DebugDrawNode(this->root.get(), 0);
}

void SceneCuller::AddComponent(Component *component, bool isStatic) {
    BoundingBox bounds;
    if (!component->GetWorldBounds(bounds))
        return;

    if (isStatic) {
        this->staticRenderablesPending.push_back({component, bounds});
        this->needsRebuild = true;
        return;
    }

    this->dynamicRenderables.push_back(component);
}

void SceneCuller::RemoveComponent(Component *component) {
    this->dynamicRenderables.erase(
        std::remove(this->dynamicRenderables.begin(), this->dynamicRenderables.end(), component), 
        this->dynamicRenderables.end()
    );

    auto iter = std::remove_if(
        this->staticRenderablesPending.begin(), 
        this->staticRenderablesPending.end(), 
        [component](const auto &pair) { return pair.first == component; }
    );
    if (iter != this->staticRenderablesPending.end()) {
        this->staticRenderablesPending.erase(iter, this->staticRenderablesPending.end());
        this->needsRebuild = true;
    }
}

void SceneCuller::Build() {
    if (this->staticRenderablesPending.empty()) {
        this->needsRebuild = false;
        return;
    }

    BoundingBox sceneBounds = this->staticRenderablesPending[0].second;
    for (int i = 1; i < this->staticRenderablesPending.size(); ++i)
        BoundingBox::CreateMerged(sceneBounds, sceneBounds, this->staticRenderablesPending[i].second);

    sceneBounds.Extents.x *= 1.01f;
    sceneBounds.Extents.y *= 1.01f;
    sceneBounds.Extents.z *= 1.01f;

    this->octree.Build(this->staticRenderablesPending, sceneBounds);
    this->needsRebuild = false;
}

void SceneCuller::GatherVisibility(std::vector<Render_view> &views) const {
    for (Render_view &view : views) {
        std::vector<Component *> visible;

        if (view.skipFrustumCulling) {
            // Static
            this->octree.QueryAll(visible);

            // Dynamic
            for (Component *component : this->dynamicRenderables)
                if (component->GetOwner()->IsActive() && component->isActive)
                    visible.push_back(component);
        }
        else {
            // Static
            this->octree.Query(view.frustum, visible);

            // Dynamic
            for (Component *component : this->dynamicRenderables) {
                if (!component->GetOwner()->IsActive() || !component->isActive)
                    continue;

                BoundingBox bounds;
                if (!component->GetWorldBounds(bounds))
                    continue;

                if (view.frustum.Contains(bounds) != DISJOINT)
                    visible.push_back(component);
            }
        }

        std::sort(visible.begin(), visible.end());
        visible.erase(std::unique(visible.begin(), visible.end()), visible.end());

        for (Component *component : visible)
            component->Render(view, view.queue);

        //Debug::SetStat(
        //    "octree.culledStatic", 
        //    std::to_string(staticCount - staticVisible) + "/" + std::to_string(staticCount)
        //);

        //Debug::SetStat(
        //    "octree.culledDynamic", 
        //    std::to_string(dynamicCount - dynamicVisible) + "/" + std::to_string(dynamicCount)
        //);
    }
}

void SceneCuller::Clear() {
    this->octree.Clear();
    this->staticRenderablesPending.clear();
    this->dynamicRenderables.clear();
    this->needsRebuild = false;
}

void SceneCuller::DebugDraw() {
    this->octree.DebugDraw();
}