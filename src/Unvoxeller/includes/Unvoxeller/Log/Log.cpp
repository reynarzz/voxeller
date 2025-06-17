// log.cpp
#include "Log.h"

namespace VoxellerApp
{

    std::shared_ptr<spdlog::logger> core;
    std::shared_ptr<spdlog::logger> app;
    
    void init()
    {
#ifndef NDEBUG
        spdlog::set_pattern("[%T] [%n] %v");

        core = spdlog::stdout_color_mt("Core");
        app = spdlog::stdout_color_mt("Editor");

        core->set_level(spdlog::level::trace);
        app->set_level(spdlog::level::trace);
#endif
    }
}