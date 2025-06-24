#include <Unvoxeller/TextureGenerators/AtlasTextureGen.h>
#include <Unvoxeller/Log/Log.h>


namespace Unvoxeller
{

std::shared_ptr<TextureData> AtlasTextureGenerator::GetTexture(std::vector<FaceRect>& faces, const std::vector<color>& palette,
                                                      		   const std::vector<vox_model>& models, const bool texturesPOT)
{
	s32 atlasDim = 16;
	s32 usedW = 0;
	s32 usedH = 0;

	if (texturesPOT)
	{
		// Start from 16 and double
		while (true)
		{
			if (PackFacesIntoAtlas(atlasDim, faces))
			{
				break;
			}
			atlasDim *= 2;
			if (atlasDim > 4096)
			{ // safety break
				LOG_ERROR("Could not pack texture atlas up to 4096 for frame ");// << frameIndex << "\n";
				break;
			}
		}

		usedW = atlasDim;
		usedH = atlasDim;
	}
	else
	{
		// Start with 16 and grow by 16 steps or double as needed (non-POT allowed)
		while (true)
		{
			if (PackFacesIntoAtlas(atlasDim, faces))
			{
				break;
			}

			atlasDim += 16;

			if (atlasDim > 4096)
			{
				LOG_ERROR("Could not pack texture atlas up to 4096 for frame ");
				break;
			}
		}
		// Shrink to actual used size
		// We can compute used width and height from faces placement
		for (auto& fr : faces)
		{
			usedW = std::max(usedW, fr.atlasX + fr.w + 2);
			usedH = std::max(usedH, fr.atlasY + fr.h + 2);
		}

		atlasDim = std::max(usedW, usedH);
	}

	// Create image
	auto textureData = std::make_shared<TextureData>();
	textureData->Width = usedW;
	textureData->Height = usedH;

	LOG_INFO("Texture size: ({0}, {1})", textureData->Width, textureData->Height);

	GenerateAtlasImage(atlasDim, atlasDim, faces, models, palette, textureData->Buffer);

	return textureData;
}


    // A simple shelf-bin packer for placing rectangles (with added border) into a square atlas of given dimension.
    // Returns true and updates FaceRect atlas positions if successful, or false if not fitting.
bool AtlasTextureGenerator::PackFacesIntoAtlas(int atlasSize, std::vector<FaceRect>& rects)
{
	// Sort rectangles by height (descending) for better packing (larger first).
	std::sort(rects.begin(), rects.end(), [](const FaceRect& a, const FaceRect& b)
		{
			// compare (h+2) including border
			int ah = a.h + 2;
			int bh = b.h + 2;
			if (ah == bh) {
				// if heights equal, maybe sort by width as well
				return (a.w + 2) > (b.w + 2);
			}
			return ah > bh;
		});

	int currentX = 0;
	int currentY = 0;
	int currentRowHeight = 0;
	int usedHeight = 0;

	for (auto& face : rects) 
    {
		int rw = face.w + 2; // rect width with border
		int rh = face.h + 2; // rect height with border
		if (rw > atlasSize || rh > atlasSize) 
        {
			return false; // one rect too big to ever fit
		}
		if (currentX + rw > atlasSize) 
        {
			// start new row
			currentY += currentRowHeight;
			currentX = 0;
			currentRowHeight = 0;
		}
		if (currentY + rh > atlasSize) 
        {
			return false; // height overflow
		}
		// place this rect
		face.atlasX = currentX;
		face.atlasY = currentY;
		// update row
		currentX += rw;
		if (rh > currentRowHeight) 
        {
			currentRowHeight = rh;
		}
		usedHeight = std::max(usedHeight, currentY + currentRowHeight);
		if (usedHeight > atlasSize) 
        {
			return false;
		}
	}
	return true;
}




// Generate the texture atlas image data given the list of faces and palette colors
void AtlasTextureGenerator::GenerateAtlasImage(s32 texWidth,
	s32 texHeight,
	const std::vector<FaceRect>& faces,
	const std::vector<vox_model>& models,
	const std::vector<color>& palette,
	std::vector<unsigned char>& outImage)
{
	const int border = 1;
	outImage.assign(texWidth * texHeight * 4, 0);

	// Helper to fetch RGBA from palette given a MagicaVoxel colorIndex (1–255)
	auto getRGBA = [&](uint8_t ci) 
	{
		size_t idx = ci > 0 ? ci - 1 : 0;
		if (idx >= palette.size()) idx = palette.size() - 1;
		return palette[idx];
		};

	// Sample the colorIndex at a given (x,y,z)
	auto sampleCI = [&](int x, int y, int z, s32 modelIndex)->uint8_t 
	{
		int cell = models[modelIndex].voxel_3dGrid[z][y][x];
		if (cell < 0) return 0;
		return models[modelIndex].voxels[cell].colorIndex;
		};

	for (auto& face : faces) 
	{
		int x0 = face.atlasX;
		int y0 = face.atlasY;
		int w = face.w;
		int h = face.h;

		// 1) Fill interior (w×h) by sampling the original voxels
		for (int iy = 0; iy < h; ++iy) 
		{
			for (int ix = 0; ix < w; ++ix) 
			{
				int vx, vy, vz;
				switch (face.orientation) 
				{
				case 'X': // +X face: plane at x+1, u→Z, v→Y
					vx = face.constantCoord - 1;
					vy = face.vMin + iy;
					vz = face.uMin + ix;
					break;
				case 'x': // -X face: plane at x,  u→←Z, v→Y
					vx = face.constantCoord;
					vy = face.vMin + iy;
					vz = face.uMin + (w - 1 - ix);
					break;
				case 'Y': // +Y face: plane at y+1, u→X, v→Z
					vx = face.uMin + ix;
					vy = face.constantCoord - 1;
					vz = face.vMin + iy;
					break;
				case 'y': // -Y face: plane at y,  u→X, v→←Z
					vx = face.uMin + ix;
					vy = face.constantCoord;
					vz = face.vMin + (h - 1 - iy);
					break;
				case 'Z': // +Z face: plane at z+1, u→X, v→Y
					vx = face.uMin + ix;
					vy = face.vMin + iy;
					vz = face.constantCoord - 1;
					break;
				case 'z': // -Z face: plane at z,  u→←X, v→Y
					vx = face.uMin + (w - 1 - ix);
					vy = face.vMin + iy;
					vz = face.constantCoord;
					break;
				default:
					continue;
				}

				uint8_t ci = sampleCI(vx, vy, vz, face.modelIndex);
				auto col = getRGBA(ci);

				int px = x0 + border + ix;
				int py = y0 + border + iy;
				int idx = (py * texWidth + px) * 4;
				outImage[idx + 0] = col.r;
				outImage[idx + 1] = col.g;
				outImage[idx + 2] = col.b;
				outImage[idx + 3] = col.a;
			}
		}

		// 2) Bleed edges into the 1-pixel border on all four sides:

		// Top & bottom
		for (int ix = 0; ix < w; ++ix) 
		{
			// copy from first interior row → top border
			int srcTop = ((y0 + border + 0) * texWidth + (x0 + border + ix)) * 4;
			int dstTop = ((y0 + 0) * texWidth + (x0 + border + ix)) * 4;
			memcpy(&outImage[dstTop], &outImage[srcTop], 4);

			// copy from last interior row → bottom border
			int srcBot = ((y0 + border + h - 1) * texWidth + (x0 + border + ix)) * 4;
			int dstBot = ((y0 + border + h) * texWidth + (x0 + border + ix)) * 4;
			memcpy(&outImage[dstBot], &outImage[srcBot], 4);
		}

		// Left & right
		for (int iy = 0; iy < h; ++iy) 
		{
			// copy from first interior column → left border
			int srcL = ((y0 + border + iy) * texWidth + (x0 + border + 0)) * 4;
			int dstL = ((y0 + border + iy) * texWidth + (x0 + 0)) * 4;
			memcpy(&outImage[dstL], &outImage[srcL], 4);

			// copy from last interior column → right border
			int srcR = ((y0 + border + iy) * texWidth + (x0 + border + w - 1)) * 4;
			int dstR = ((y0 + border + iy) * texWidth + (x0 + border + w)) * 4;
			memcpy(&outImage[dstR], &outImage[srcR], 4);
		}

		// Finally fill corners by copying the appropriate interior pixel:
		// top-left corner
		memcpy(
			&outImage[((y0 + 0) * texWidth + (x0 + 0)) * 4],
			&outImage[((y0 + border) * texWidth + (x0 + border)) * 4],
			4
		);
		// top-right
		memcpy(
			&outImage[((y0 + 0) * texWidth + (x0 + border + w)) * 4],
			&outImage[((y0 + border) * texWidth + (x0 + border + w - 1)) * 4],
			4
		);
		// bottom-left
		memcpy(
			&outImage[((y0 + border + h) * texWidth + (x0 + 0)) * 4],
			&outImage[((y0 + border + h - 1) * texWidth + (x0 + border)) * 4],
			4
		);
		// bottom-right
		memcpy(
			&outImage[((y0 + border + h) * texWidth + (x0 + border + w)) * 4],
			&outImage[((y0 + border + h - 1) * texWidth + (x0 + border + w - 1)) * 4],
			4
		);
	}


	// auto flipVertical = [&](std::vector<unsigned char>& img, int w, int h) {
	// 	const int rowBytes = w * 4;
	// 	std::vector<unsigned char> tmp(rowBytes);
	// 	for (int y = 0; y < h/2; ++y) {
	// 		int topIndex = y * rowBytes;
	// 		int botIndex = (h - 1 - y) * rowBytes;
	// 		// swap the entire row
	// 		std::memcpy(tmp.data(),        &img[topIndex], rowBytes);
	// 		std::memcpy(&img[topIndex],   &img[botIndex], rowBytes);
	// 		std::memcpy(&img[botIndex],   tmp.data(),      rowBytes);
	// 	}
	// };

	// flipVertical(outImage, texWidth, texHeight);

}

}