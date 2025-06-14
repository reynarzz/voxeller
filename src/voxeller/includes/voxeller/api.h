#pragma once

// C linkage macro
// VOXELLER_API / VXAPI handling
#if defined(_WIN32)

  // When building the DLL (inside voxeller project)
#ifdef VOXELLER_API_EXPORT
#define VOXELLER_API __declspec(dllexport)
#define VXAPI extern "C" __declspec(dllexport)

// When consuming the DLL (outside)
#elif defined(VOXELLER_LIB)
#define VOXELLER_API __declspec(dllimport)
#define VXAPI extern "C" __declspec(dllimport)

// If neither defined — fallback (still valid)
#else
#define VOXELLER_API
#define VXAPI
#endif

#else
  // Non-Windows platforms (Linux/macOS)
#define VOXELLER_API
#define VXAPI
#endif
