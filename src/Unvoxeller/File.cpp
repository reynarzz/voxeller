#include <Unvoxeller/File.h>

namespace Unvoxeller
{
#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <unistd.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <limits.h>
#endif

    std::string File::GetExecutableFullPath()
    {
    #if defined(_WIN32)
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        return std::string(path);
    #elif defined(__APPLE__)
        char path[1024];
        uint32_t size = sizeof(path);
        if (_NSGetExecutablePath(path, &size) == 0)
            return std::string(path);
        else
            return std::string(); // Fallback: buffer too small
    #elif defined(__linux__)
        char path[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
        return std::string(path, (count > 0) ? count : 0);
    #else
        return std::string(); // Unsupported platform
    #endif
    }

    std::string File::GetExecutableDir() 
    {
        std::string fullPath = GetExecutableFullPath();
        size_t pos = fullPath.find_last_of("/\\");
        return (pos != std::string::npos) ? fullPath.substr(0, pos) : "";
    }

    std::string File::GetExecutableName()
    {
        std::string fullPath = GetExecutableFullPath();
        size_t pos = fullPath.find_last_of("/\\")+1;
        return (pos != std::string::npos) ? fullPath.substr(pos) : ""; 
    }
};