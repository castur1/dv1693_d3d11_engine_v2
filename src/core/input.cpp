#include <Windows.h>

#include "input.hpp"

Input::Key_state Input::keyStates[0xFF] = {};

int Input::mouseX = 0;
int Input::mouseY = 0;
int Input::prevMouseX = 0;
int Input::prevMouseY = 0;
int Input::mouseDeltaX = 0;
int Input::mouseDeltaY = 0;

bool Input::isMouseCaptured = false;
HWND Input::capturedWindow = nullptr;

void Input::Update() {
    for (int i = 0; i < 0xFF; ++i) {
        keyStates[i].wasDown = keyStates[i].isDown;
        keyStates[i].isDown = GetAsyncKeyState(i) & 0x8000;
    }

    prevMouseX = mouseX;
    prevMouseY = mouseY;

    if (isMouseCaptured && capturedWindow) {
        RECT clientRect;
        GetClientRect(capturedWindow, &clientRect);

        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        POINT centrePoint = {width / 2, height / 2};
        ClientToScreen(capturedWindow, &centrePoint);

        POINT cursorPosition;
        GetCursorPos(&cursorPosition);

        mouseDeltaX = cursorPosition.x - centrePoint.x;
        mouseDeltaY = cursorPosition.y - centrePoint.y;

        SetCursorPos(centrePoint.x, centrePoint.y);

        mouseX = centrePoint.x;
        mouseY = centrePoint.y;
    }
    else {
        POINT cursorPosition;
        if (GetCursorPos(&cursorPosition) && capturedWindow) {
            ScreenToClient(capturedWindow, &cursorPosition);
            mouseX = cursorPosition.x;
            mouseY = cursorPosition.y;

            mouseDeltaX = mouseX - prevMouseX;
            mouseDeltaY = mouseY - prevMouseY;
        }
        else {
            mouseDeltaX = 0;
            mouseDeltaY = 0;
        }
    }
}

bool Input::IsKeyDown(int key) {
    return keyStates[key].isDown;
}

bool Input::IsKeyPressed(int key) {
    return keyStates[key].isDown && !keyStates[key].wasDown;
}

bool Input::IsKeyReleased(int key) {
    return !keyStates[key].isDown && keyStates[key].wasDown;
}

int Input::GetMouseX() {
    return mouseX;
}

int Input::GetMouseY() {
    return mouseY;
}

int Input::GetMouseDeltaX() {
    return mouseDeltaX;
}

int Input::GetMouseDeltaY() {
    return mouseDeltaY;
}

void Input::CaptureMouse(HWND hWnd) {
    if (isMouseCaptured)
        return;

    isMouseCaptured = true;
    capturedWindow = hWnd;

    ShowCursor(false);

    RECT clientRect;
    GetClientRect(capturedWindow, &clientRect);

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    POINT centrePoint = {width / 2, height / 2};
    ClientToScreen(capturedWindow, &centrePoint);

    SetCursorPos(centrePoint.x, centrePoint.y);

    mouseDeltaX = 0;
    mouseDeltaY = 0;
}

void Input::ReleaseMouse() {
    if (!isMouseCaptured)
        return;

    isMouseCaptured = false;
    capturedWindow = nullptr;

    ShowCursor(true);

    mouseDeltaX = 0;
    mouseDeltaY = 0;
}

bool Input::IsMouseCaptured() {
    return isMouseCaptured;
}
