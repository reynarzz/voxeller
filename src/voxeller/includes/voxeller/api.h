#pragma once

#define VOXELLER_API_EXPORT

#if defined (_WIN32)
   #ifndef EXPORT
     #define EXPORT extern "C"
   #endif
   #if defined(VOXELLER_API_EXPORT)
     #define VOXELLER_API __declspec(dllexport)
     #define VXAPI extern "C" __declspec(dllexport)
   #elif defined(VOXELLER_LIB)
     #define VOXELLER_API __declspec(dllimport)
     #define VXAPI __declspec(dllimport)
   #endif
#else
    #define EXPORT
    #define VOXELLER_API
    #define VXAPI
#endif
