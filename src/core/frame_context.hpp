#ifndef FRAME_CONTEXT_HPP
#define FRAME_CONTEXT_HPP

class SceneManager;
class AssetManager;
class Renderer;

struct Engine_context {
    Renderer *renderer = nullptr;
    SceneManager *sceneManager = nullptr;
    AssetManager *assetManager = nullptr;
};

struct Frame_context {
    float deltaTime = 0.0f;
    const Engine_context &engineContext;
};

#endif