#include "WinLogOutput.hpp"
#include <Windows.h>
#include <cstdio>


namespace devs_out_of_bounds {
namespace win {
    static WinLogOutput g_win_log_output;
    WinLogOutput::WinLogOutput() {}
    WinLogOutput::~WinLogOutput() {}
    void WinLogOutput::Log(LogLevel level, const char* message) {
        const char* level_string = 0;
        switch (level) {
        case LogLevel::Debug:
            level_string = "DEBUG";
            break;
        case LogLevel::Info:
            level_string = "INFO";
            break;
        case LogLevel::Warning:
            level_string = "WARNING";
            break;
        case LogLevel::Error:
            level_string = "ERROR";
            break;
        case LogLevel::Fatal:
            level_string = "FATAL";
            break;
        }
#if defined(_DEBUG)
        OutputDebugStringA("[");
        OutputDebugStringA(level_string);
        OutputDebugStringA("] ");
        OutputDebugStringA(message);
        OutputDebugStringA("\n");
#endif
        printf("[%s] %s\n", level_string, message);
    }
} // namespace win
ILogOutput* g_log_output = &win::g_win_log_output;
} // namespace devs_out_of_bounds