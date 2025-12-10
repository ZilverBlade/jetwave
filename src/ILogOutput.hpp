#pragma once

#include <src/Core.hpp>

namespace devs_out_of_bounds {
enum struct LogLevel : int { Debug, Info, Warning, Error, Fatal };
/**
 * @brief Abstract logger sink used by the application logging utility.
 *
 * Implementations receive string messages and route them to system logs,
 * consoles, or persistent storage depending on platform.
 */
struct ILogOutput {
    ILogOutput() = default;

    /**
     * @brief Emit a formatted log message.
     *
     * @param level Severity level of the message.
     * @param message UTF-8 formatted message to output.
     */
    virtual void Log(LogLevel level, const char* message) = 0;

protected:
    virtual ~ILogOutput() = default;
};
extern ILogOutput* g_log_output;
} // namespace devs_out_of_bounds