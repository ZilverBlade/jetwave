#include <cstdio>
#include <cstdlib>

#include <src/Core.hpp>
#include <src/Logging.hpp>
#include <src/MainApp.hpp>

static void AppMain() {
    devs_out_of_bounds::MainApp app;
    app.Main();
}

#if defined(DOB_PLATFORM_FAMILY_WINDOWS)

#include <Windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
#ifndef NDEBUG
    nCmdShow = TRUE;
#endif
    if (nCmdShow == TRUE) {
        AllocConsole();
        FILE* stdout_handle = freopen("CONOUT$", "w", stdout);
        FILE* stderr_handle = freopen("CONOUT$", "w", stderr);
    }
    AppMain();
    return EXIT_SUCCESS;
}

#else

int main(int argc, char** argv, char** envp) {
    AppMain();
    return EXIT_SUCCESS;
}

#endif
