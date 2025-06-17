#pragma once

// C linkage macro
// UNVOXELLER_API / VXAPI handling
#if defined(_WIN32)

  // When building the DLL (inside Unvoxeller project)
#ifdef UNVOXELLER_API_EXPORT
#define UNVOXELLER_API __declspec(dllexport)
#define VXAPI extern "C" __declspec(dllexport)

// When consuming the DLL (outside)
#elif defined(UNVOXELLER_LIB)
#define UNVOXELLER_API __declspec(dllimport)
#define VXAPI extern "C" __declspec(dllimport)

// If neither defined — fallback (still valid)
#else
#define UNVOXELLER_API
#define VXAPI
#endif

#else
  // Non-Windows platforms (Linux/macOS)
#define UNVOXELLER_API
#define VXAPI
#endif
