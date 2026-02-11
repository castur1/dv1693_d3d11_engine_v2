#ifndef FRAME_CONTEXT_HPP
#define FRAME_CONTEXT_HPP

class SceneManager;

struct Frame_context {
    float deltaTime;
    SceneManager *sceneManager;
};

#endif