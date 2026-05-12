#include "core/application.hpp"

#include "Windows.h"

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

// Hint to make the application use dedicated graphics cards (NIVIDA/AMD)
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 1;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    Application app;

    if (!app.Initialize())
        return -1;

    app.Run();

    return 0;
}