#pragma once

#include <src/Core.hpp>
#include <src/ILogOutput.hpp>

namespace devs_out_of_bounds {
/**
 * @brief Entry point wrapper.
 */
class MainApp : NoCopy, NoMove {
public:
    MainApp() {}
    ~MainApp() {}

    void Main();
};
} // namespace devs_out_of_bounds