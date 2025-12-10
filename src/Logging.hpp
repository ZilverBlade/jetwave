#pragma once
#include <fmt/format.h>
#include <src/ILogOutput.hpp>

template <typename... Args>
void LOG_DEBUG(fmt::format_string<Args...> fmt, Args&&... args) {
    if (devs_out_of_bounds::g_log_output) {
        devs_out_of_bounds::g_log_output->Log(devs_out_of_bounds::LogLevel::Debug, fmt::format(fmt, std::forward<Args>(args)...).c_str());
    }
}

template <typename... Args>
void LOG_INFO(fmt::format_string<Args...> fmt, Args&&... args) {
    if (devs_out_of_bounds::g_log_output) {
        devs_out_of_bounds::g_log_output->Log(devs_out_of_bounds::LogLevel::Info, fmt::format(fmt, std::forward<Args>(args)...).c_str());
    }
}

template <typename... Args>
void LOG_WARNING(fmt::format_string<Args...> fmt, Args&&... args) {
    if (devs_out_of_bounds::g_log_output) {
        devs_out_of_bounds::g_log_output->Log(devs_out_of_bounds::LogLevel::Warning, fmt::format(fmt, std::forward<Args>(args)...).c_str());
    }
}

template <typename... Args>
void LOG_ERROR(fmt::format_string<Args...> fmt, Args&&... args) {
    if (devs_out_of_bounds::g_log_output) {
        devs_out_of_bounds::g_log_output->Log(devs_out_of_bounds::LogLevel::Error, fmt::format(fmt, std::forward<Args>(args)...).c_str());
    }
}

template <typename... Args>
void LOG_FATAL(fmt::format_string<Args...> fmt, Args&&... args) {
    if (devs_out_of_bounds::g_log_output) {
        devs_out_of_bounds::g_log_output->Log(devs_out_of_bounds::LogLevel::Fatal, fmt::format(fmt, std::forward<Args>(args)...).c_str());
    }
}
