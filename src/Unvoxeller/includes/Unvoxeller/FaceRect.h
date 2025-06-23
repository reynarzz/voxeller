#pragma once
#include <Unvoxeller/Types.h>

// Structure to hold a face (rectangle) that needs to be packed into the atlas
struct FaceRect
{
	s32 w, h;        // dimensions (including border will be added around)
	s32 atlasX, atlasY; // top-left position in atlas (including border) after packing
	u8 colorIndex; // palette index (for color)
	// face orientation and extents for UV mapping
	c8 orientation; // one of: 'X','x','Y','y','Z','z' for +X, -X, +Y, -Y, +Z, -Z
	// Face extents in voxel coordinates (the *inclusive* start and end boundaries of the face in world coordinates)
	s32 uMin, uMax;  // min and max along face's U-axis (in world coords, face boundary)
	s32 vMin, vMax;  // min and max along face's V-axis
	s32 constantCoord; // the coordinate of the face plane (e.g., x or y or z value for the face)
	s32 modelIndex;
};