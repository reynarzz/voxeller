#pragma once
#include <Unvoxeller/Unvoxeller.h>
#include <Rendering/VoxObject.h>

std::shared_ptr<VoxObject> CreateVoxObject(const std::vector<std::shared_ptr<Unvoxeller::UnvoxScene>>& scenes);
std::string GetFileName(const std::string& fullPath);
std::string GetFileExtension(const std::string& fullPath);