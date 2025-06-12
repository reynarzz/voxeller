// log.cpp
#include "Log.h"

namespace VoxellerApp
{
    std::shared_ptr<spdlog::logger> core;
    std::shared_ptr<spdlog::logger> editor;
    
    void init()
    {
#ifndef NDEBUG
        spdlog::set_pattern("[%T] [%n] %v");

        core = spdlog::stdout_color_mt("Voxeller");
        editor = spdlog::stdout_color_mt("Editor");

        core->set_level(spdlog::level::trace);
        editor->set_level(spdlog::level::trace);
#endif
    }
}
