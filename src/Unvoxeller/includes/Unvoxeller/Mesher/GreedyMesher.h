#pragma once
#include "MesherBase.h"

namespace Unvoxeller
{
	class GreedyMesher : public MesherBase
	{
	public:
		std::vector<FaceRect> CreateFaces(const vox_model& model, const vox_size& size, s32 modelIndex) override;
	protected:
	};
}