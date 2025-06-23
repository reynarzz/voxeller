#pragma once
#include <vector>
#include <Unvoxeller/VoxelTypes.h>
#include <Unvoxeller/FaceRect.h>

namespace Unvoxeller
{
	class MesherBase
	{
	public:
		virtual std::vector<FaceRect> CreateFaces(const vox_model& model, const vox_size& size, s32 modelIndex) = 0;
	protected:
	};
}