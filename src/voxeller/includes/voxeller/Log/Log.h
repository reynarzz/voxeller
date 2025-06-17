// log.h
#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <Voxeller/api.h>

namespace VoxellerApp
{
	// Forward declared loggers
	VOXELLER_API extern std::shared_ptr<spdlog::logger> core;
	VOXELLER_API extern std::shared_ptr<spdlog::logger> app;

	VOXELLER_API void init();  // Call this once during startup
}

#ifdef NDEBUG  // Release build
#define LOG_INFO(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)
#else
	#ifdef VOXELLER_API_EXPORT
		#define LOG_INFO(...)     VoxellerApp::core->info(__VA_ARGS__)
		#define LOG_WARN(...)     VoxellerApp::core->warn(__VA_ARGS__)
		#define LOG_ERROR(...)    VoxellerApp::core->error(__VA_ARGS__)
	#else
		#define LOG_INFO(...)     VoxellerApp::app->info(__VA_ARGS__)
		#define LOG_WARN(...)     VoxellerApp::app->warn(__VA_ARGS__)
		#define LOG_ERROR(...)    VoxellerApp::app->error(__VA_ARGS__)
	#endif
#endif