#ifndef FRAME_CONTEXT_HPP
#define FRAME_CONTEXT_HPP

class SceneManager;
class AssetManager;

struct Frame_context {
    float deltaTime;
    SceneManager *sceneManager;
    AssetManager *assetManager;
};

#endif