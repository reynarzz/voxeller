// log.h
#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <voxeller/api.h>

namespace VoxellerApp
{
    // Forward declared loggers
    VOXELLER_API extern std::shared_ptr<spdlog::logger> core;
    VOXELLER_API extern std::shared_ptr<spdlog::logger> editor;

    VOXELLER_API void init();  // Call this once during startup
}

// Logging macros
#ifdef NDEBUG  // Release build
    #define LOG_CORE_INFO(...)
    #define LOG_CORE_WARN(...)
    #define LOG_CORE_ERROR(...)
    #define LOG_EDITOR_INFO(...)
    #define LOG_EDITOR_WARN(...)
    #define LOG_EDITOR_ERROR(...)
#else
    #define LOG_CORE_INFO(...)     VoxellerApp::core->info(__VA_ARGS__)
    #define LOG_CORE_WARN(...)     VoxellerApp::core->warn(__VA_ARGS__)
    #define LOG_CORE_ERROR(...)    VoxellerApp::core->error(__VA_ARGS__)

    #define LOG_EDITOR_INFO(...)   VoxellerApp::editor->info(__VA_ARGS__)
    #define LOG_EDITOR_WARN(...)   VoxellerApp::editor->warn(__VA_ARGS__)
    #define LOG_EDITOR_ERROR(...)  VoxellerApp::editor->error(__VA_ARGS__)
#endif
