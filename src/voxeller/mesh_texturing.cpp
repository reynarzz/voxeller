
#ifndef VOXELLER_API_EXPORT
#define VOXELLER_API_EXPORT
#endif

#include <Voxeller/mesh_texturing.h>
#include <fstream>
#define STB_IMAGE_STATIC

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <iostream>


void mesh_texturizer::export_pallete_png(const char* path, const std::vector<Voxeller::color>& pallete)
{
	if (!pallete.empty())
	{
		constexpr int strideBytes = 0;

		int length{};
		/*if (flipY)
		{
			stbi_flip_vertically_on_write(1);
		}*/
		unsigned char* image = stbi_write_png_to_mem(reinterpret_cast<const unsigned char*>(pallete.data()), strideBytes, 255, 1, 4, &length);

		// TODO: make this platform independant
		std::ofstream imageStream(path, std::ios::out | std::ios::binary);

		imageStream.write(reinterpret_cast<const char*>(image), length);
		imageStream.close();
	}
	else
	{
		std::cout << "Color pallete is empty";
	}
}
