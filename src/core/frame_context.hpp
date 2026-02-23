#ifndef FRAME_CONTEXT_HPP
#define FRAME_CONTEXT_HPP

class SceneManager;
class AssetManager;

struct Engine_context {
    SceneManager *sceneManager;
    AssetManager *assetManager;
};

struct Frame_context {
    float deltaTime;
    const Engine_context &engineContext;
};

#endif