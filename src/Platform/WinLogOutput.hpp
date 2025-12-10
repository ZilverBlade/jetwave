#pragma once
#include <src/ILogOutput.hpp>

namespace devs_out_of_bounds {
namespace win {
    class WinLogOutput : public ILogOutput {
    public:
        WinLogOutput();
        ~WinLogOutput();

        void Log(LogLevel level, const char* message) override;

    private:
    };
} // namespace win
} // namespace devs_out_of_bounds