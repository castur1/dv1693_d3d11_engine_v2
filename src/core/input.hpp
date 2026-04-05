#ifndef INPUT_HPP
#define INPUT_HPP

#include <Windows.h>

class Input {
    struct Key_state {
        bool isDown;
        bool wasDown;
    };

    static Key_state keyStates[0xFF];

    static int mouseX;
    static int mouseY;
    static int prevMouseX;
    static int prevMouseY;
    static int mouseDeltaX;
    static int mouseDeltaY;

    static bool isMouseCaptured;
    static HWND capturedWindow;

public:
    static void Update();
    static void Clear();

    static bool IsKeyDown(int key);
    static bool IsKeyPressed(int key);
    static bool IsKeyReleased(int key);

    static int GetMouseX();
    static int GetMouseY();
    static int GetMouseDeltaX();
    static int GetMouseDeltaY();

    static void CaptureMouse(HWND hWnd);
    static void ReleaseMouse();
    static bool IsMouseCaptured();
};

#endif