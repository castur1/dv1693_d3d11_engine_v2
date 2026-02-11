#include "window.hpp"
#include "core/logging.hpp"

#define WINDOW_CLASS_NAME L"window_class"

Window::Window() : shouldClose(true), wasResized(false), width(0), height(0), hInst{}, hWnd{} {}

Window::~Window() {
    if (this->hWnd) DestroyWindow(this->hWnd);
    if (this->hInst) UnregisterClass(WINDOW_CLASS_NAME, this->hInst);
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Window *window{};
    if (uMsg == WM_CREATE) {
        window = (Window *)((CREATESTRUCT *)lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
    } else {
        window = (Window *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    switch (uMsg) {
        case WM_SIZE: {
            if (wParam != SIZE_MINIMIZED) {
                window->width = LOWORD(lParam);
                window->height = HIWORD(lParam);
                window->wasResized = true;
            }
            return 0;
        }
        case WM_QUIT:
        case WM_CLOSE: {
            window->shouldClose = true;
            return 0;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool Window::Initialize(int width, int height, const wchar_t *title) {
    LogInfo("Creating window...\n");

    this->hInst = GetModuleHandle(nullptr);

    WNDCLASS windowClass{};
    windowClass.lpfnWndProc = this->WindowProc;
    windowClass.hInstance = this->hInst;
    windowClass.lpszClassName = WINDOW_CLASS_NAME;
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&windowClass);

    this->hWnd = CreateWindowEx(
        0, 
        windowClass.lpszClassName, 
        title, 
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        width, height, 
        NULL, 
        NULL, 
        this->hInst, 
        this
    );

    if (!this->hWnd) {
        LogError("Failed to create window\n");
        return false;
    }

    ShowWindow(this->hWnd, SW_SHOWDEFAULT);

    this->shouldClose = false;

    return true;
}

void Window::ProcessMessages() {
    MSG message{};
    while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}

bool Window::ShouldClose() {
    return this->shouldClose;
}

HWND Window::GetHandle() const {
    return this->hWnd;
}

bool Window::WasResized() const {
    return this->wasResized;
}

void Window::ClearResizeFlag() {
    this->wasResized = false;
}

int Window::GetWidth() const {
    return this->width;
}

int Window::GetHeight() const {
    return this->height;
}
