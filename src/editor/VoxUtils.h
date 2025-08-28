#pragma once
#include <Unvoxeller/Unvoxeller.h>
#include <Rendering/VoxObject.h>

std::shared_ptr<VoxObject> CreateVoxObject(const std::vector<std::shared_ptr<Unvoxeller::UnvoxScene>>& scenes);