#include <Unvoxeller/Mesher/GreedyMesher.h>

namespace Unvoxeller
{
    std::vector<FaceRect> GreedyMesher::CreateFaces(
	const vox_model& model,
	const vox_size& size, s32 modelIndex)
{
	const int X = size.x;
	const int Y = size.y;
	const int Z = size.z;
	std::vector<FaceRect> faces;
	faces.reserve(1000);

	// Quick occupancy test
	auto isFilled = [&](int x, int y, int z) 
    {
		if (x < 0 || x >= X || y < 0 || y >= Y || z < 0 || z >= Z)
        {
            return false;
        }

		return model.voxel_3dGrid[z][y][x] >= 0;
	};

	// Helper lambda to do one 2D‐greedy pass:
	auto sweep = [&](char orient,
		int dimU, int dimV, int dimW,
		auto getFilled,
		auto getPlaneConst)
		{
			// dimU,dimV = extents of the mask; dimW = sweep axis length
			std::vector<bool> mask(dimU * dimV), visited(dimU * dimV);
			for (int w = 0; w < dimW; ++w) 
            {
				// clear mask+visited
				std::fill(mask.begin(), mask.end(), false);
				std::fill(visited.begin(), visited.end(), false);

				// build mask[u,v] = true if face at (u,v,w)
				for (int v = 0; v < dimV; ++v) 
                {
					for (int u = 0; u < dimU; ++u) 
                    {
						if (getFilled(u, v, w))
							mask[v * dimU + u] = true;
					}
				}

				// improved greedy‐merge rects in mask
				for (int v = 0; v < dimV; ++v) 
                {
					for (int u = 0; u < dimU; ++u) 
                    {
						int idx = v * dimU + u;
						if (!mask[idx] || visited[idx]) continue;

						// 1) Compute run‐lengths for each row starting at (u,v)
						std::vector<int> rowWidths;
						for (int dv = 0; dv < dimV - v; ++dv) 
                        {
							int run = 0;
							int base = (v + dv) * dimU + u;
							while (u + run < dimU
								&& mask[base + run]
								&& !visited[base + run]) {
								++run;
							}
							if (run == 0) break;
							rowWidths.push_back(run);
						}

						// 2) Pick height h that maximizes area = h * min(widths[0..h))
						int bestArea = 0, bestW = 0, bestH = 0;
						for (int h = 1; h <= (int)rowWidths.size(); ++h)
                         {
							int wMin = *std::min_element(rowWidths.begin(),
								rowWidths.begin() + h);
							int area = wMin * h;
							if (area > bestArea) 
                            {
								bestArea = area;
								bestW = wMin;
								bestH = h;
							}
						}

						// 3) Mark visited
						for (int dv = 0; dv < bestH; ++dv)
                        {
							for (int du = 0; du < bestW; ++du)
                            {
								visited[(v + dv) * dimU + (u + du)] = true;
							}
						}

						// 4) Record a FaceRect
						FaceRect f;
						f.orientation = orient;
						f.constantCoord = getPlaneConst(u, v, w);
						f.uMin = u;            f.uMax = u + bestW;
						f.vMin = v;            f.vMax = v + bestH;
						f.w = bestW;        f.h = bestH;
						f.colorIndex = 0;      // unused for merging
						f.modelIndex = modelIndex;
						faces.push_back(f);
					}
				}
			}
		};

	// +X ('X'): sweep w=x in [0..X-1], UV=(z,y)
	sweep('X',
		/*dimU=*/Z, /*dimV=*/Y, /*dimW=*/X,
		[&](int z, int y, int x) {
			return isFilled(x, y, z)
				&& (x == X - 1 || !isFilled(x + 1, y, z));
		},
		[&](int z, int y, int x) {
			return x + 1; // plane at x+1
		});

	// -X ('x'): sweep w=x, UV=(z,y)
	sweep('x',
		Z, Y, X,
		[&](int z, int y, int x) {
			return isFilled(x, y, z)
				&& (x == 0 || !isFilled(x - 1, y, z));
		},
		[&](int z, int y, int x) {
			return x;   // plane at x
		});

	// +Y ('Y'): sweep w=y, UV=(x,z)
	sweep('Y',
		X, Z, Y,
		[&](int x, int z, int y) {
			return isFilled(x, y, z)
				&& (y == Y - 1 || !isFilled(x, y + 1, z));
		},
		[&](int x, int z, int y) {
			return y + 1;
		});

	// -Y ('y'): sweep w=y, UV=(x,z)
	sweep('y',
		X, Z, Y,
		[&](int x, int z, int y) {
			return isFilled(x, y, z)
				&& (y == 0 || !isFilled(x, y - 1, z));
		},
		[&](int x, int z, int y) {
			return y;
		});

	// +Z ('Z'): sweep w=z, UV=(x,y)
	sweep('Z',
		X, Y, Z,
		[&](int x, int y, int z) {
			return isFilled(x, y, z)
				&& (z == Z - 1 || !isFilled(x, y, z + 1));
		},
		[&](int x, int y, int z) {
			return z + 1;
		});

	// -Z ('z'): sweep w=z, UV=(x,y)
	sweep('z',
		X, Y, Z,
		[&](int x, int y, int z)
        {
			return isFilled(x, y, z)
				&& (z == 0 || !isFilled(x, y, z - 1));
		},
		[&](int x, int y, int z) 
        {
			return z;
		});

	return faces;
}

}