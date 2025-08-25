#pragma once
#include <Unvoxeller/Mesher/VoxelLikeMesher.h>

namespace Unvoxeller
{
    std::vector<FaceRect> VoxelLikeMesher::CreateFaces(const vox_model& model, const vox_size& size, int modelIndex)
{
	const int X = size.x, Y = size.y, Z = size.z;
	std::vector<FaceRect> faces;
	faces.reserve(1024);

	// shorthand test + fetch color-index
	auto getColor = [&](int x, int y, int z) -> int
     {
		if (x < 0 || x >= X || y < 0 || y >= Y || z < 0 || z >= Z) 
        {
           return -1;
        }

		return model.voxel_3dGrid[z][y][x];
	};

	// one 2D sweep over a “mask” of size U×V at depth w
	auto sweep = [&](Orientation orient,
		int dimU, int dimV, int dimW,
		auto mapUVtoXYZ_face,   // (u,v,w)->(x,y,z) of the face‐voxel
		auto mapUVtoXYZ_adj,    // (u,v,w)->(x,y,z) of the neighbor
		auto getPlaneConstXYZ)  // (u,v,w)->plane coordinate
		{
			std::vector<int>  mask(dimU * dimV), visited(dimU * dimV);
			for (int w = 0; w < dimW; ++w)
             {
				// build mask[u,v] = color idx if face exists, else -1
				for (int v = 0; v < dimV; ++v) 
                {
					for (int u = 0; u < dimU; ++u) 
                    {
						auto [fx, fy, fz] = mapUVtoXYZ_face(u, v, w);
						auto [ax, ay, az] = mapUVtoXYZ_adj(u, v, w);
						int  cFace = getColor(fx, fy, fz);
						int  cAdj = getColor(ax, ay, az);
						mask[v * dimU + u] = (cFace >= 0 && cAdj < 0) ? cFace : -1;
						visited[v * dimU + u] = 0;
					}
				}

				// greedy‐merge equal‐color runs
				for (int v = 0; v < dimV; ++v) 
                {
					for (int u = 0; u < dimU; ++u) 
                    {
						int idx0 = v * dimU + u;
						int color = mask[idx0];
						if (color < 0 || visited[idx0]) continue;

						// expand width (u→u+wU) while same color & unvisited
						int wU = 1;
						while (u + wU < dimU)
                        {
							int idx1 = v * dimU + (u + wU);
							if (mask[idx1] != color || visited[idx1]) break;
							++wU;
						}

						// expand height (v→v+wV) as long as each row matches
						int wV = 1;
						bool ok;
						while (v + wV < dimV) 
                        {
							ok = true;
							for (int k = 0; k < wU; ++k) 
                            {
								int idx2 = (v + wV) * dimU + (u + k);
								if (mask[idx2] != color || visited[idx2])
                                {
									ok = false; break;
								}
							}
                            
							if (!ok) break;
							++wV;
						}

						// mark visited
						for (int dv = 0; dv < wV; ++dv)
							for (int du = 0; du < wU; ++du)
								visited[(v + dv) * dimU + (u + du)] = 1;

						// emit one quad (FaceRect)
						FaceRect f;
						f.orientation = orient;
						f.constantCoord = getPlaneConstXYZ(u, v, w);
						f.uMin = u;  f.uMax = u + wU;
						f.vMin = v;  f.vMax = v + wV;
						f.w = wU;    f.h = wV;
						f.colorIndex = color;
						f.modelIndex = modelIndex;
						faces.push_back(f);
					}
				}
			}
		};

	// +X faces: at voxel (x,y,z) looking +X, UV=(z,y)
	sweep(Orientation::PosX,
		/*U=Z*/Z, /*V=Y*/Y, /*W=X*/X,
		/*face*/ [](int z, int y, int x) { return std::tuple{ x,y,z }; },
		/*adj */ [](int z, int y, int x) { return std::tuple{ x + 1,y,z }; },
		/*plane*/ [](int z, int y, int x) { return x + 1; }
	);

	// -X faces: UV=(z,y)
	sweep(Orientation::NegX, Z, Y, X,
		[](int z, int y, int x) { return std::tuple{ x,y,z };      },
		[](int z, int y, int x) { return std::tuple{ x - 1,y,z };    },
		[](int z, int y, int x) { return x; }
	);

	// +Y: UV=(x,z)
	sweep(Orientation::PosY, X, Z, Y,
		[](int x, int z, int y) { return std::tuple{ x,y,z };    },
		[](int x, int z, int y) { return std::tuple{ x,y + 1,z };  },
		[](int x, int z, int y) { return y + 1; }
	);

	// -Y: UV=(x,z)
	sweep(Orientation::NegY, X, Z, Y,
		[](int x, int z, int y) { return std::tuple{ x,y,z };    },
		[](int x, int z, int y) { return std::tuple{ x,y - 1,z };  },
		[](int x, int z, int y) { return y; }
	);

	// +Z: UV=(x,y)
	sweep(Orientation::PosZ, X, Y, Z,
		[](int x, int y, int z) { return std::tuple{ x,y,z };    },
		[](int x, int y, int z) { return std::tuple{ x,y,z + 1 };  },
		[](int x, int y, int z) { return z + 1; }
	);

	// -Z: UV=(x,y)
	sweep(Orientation::NegZ, X, Y, Z,
		[](int x, int y, int z) { return std::tuple{ x,y,z };    },
		[](int x, int y, int z) { return std::tuple{ x,y,z - 1 };  },
		[](int x, int y, int z) { return z; }
	);

	return faces;
}

}