#ifndef DEBUG_DRAW_HPP
#define DEBUG_DRAW_HPP

#include <d3d11.h>

// Line/Box/Frustum
// Submit to dynamic line list that is cleared each frame (needs Begin()/NewFrame())
// Just vertex position and vertex colour

class DebugDraw {
    static ID3D11Device *device;

public:
    static void Initialize(ID3D11Device *device);
    void Shutdown();

    // CONTINUE HERE!

    static void NewFrame();

    static void Line();

    static void Box();

    static void Frustum();
};

#endif