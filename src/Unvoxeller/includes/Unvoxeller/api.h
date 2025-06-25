#pragma once

#if defined(_WIN32)
	#ifdef UNVOXELLER_API_EXPORT
		#define UNVOXELLER_API __declspec(dllexport)
		#define VXAPI extern "C" __declspec(dllexport)
	#elif defined(UNVOXELLER_LIB)
		#define UNVOXELLER_API __declspec(dllimport)
		#define VXAPI extern "C" __declspec(dllimport)
	#else
		#define UNVOXELLER_API
		#define VXAPI
	#endif
#else
	#define UNVOXELLER_API
	#define VXAPI
#endif