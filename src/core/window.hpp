#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <Windows.h>

class Window {
    bool shouldClose;
    bool wasResized;

    int width;
    int height;

    HINSTANCE hInst;
    HWND hWnd;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    Window();
    ~Window();

    bool Initialize(int width, int height, const wchar_t *title);
    void ProcessMessages();
    bool ShouldClose();
    HWND GetHandle() const;

    bool WasResized() const;
    void ClearResizeFlag();
    int GetWidth() const;
    int GetHeight() const;
};

#endif