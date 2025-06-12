#ifndef VOXELLER_API_EXPORT
#define VOXELLER_API_EXPORT
#endif

#include <voxeller/vox_parser.h>

#include <fstream>
#include <iostream>

int vox_parser::modelIndex{};

vox_header vox_parser::read_vox_metadata(const char* path)
{
	std::ifstream voxFile(path, std::ios::in | std::ios::binary);
	vox_header header{};

	modelIndex = 0;

	if (voxFile.is_open())
	{
		char headerChar[5];
		int version{};

		voxFile.seekg(0, std::ios::beg);
		voxFile.read(headerChar, 4);
		voxFile.read(reinterpret_cast<char*>(&version), sizeof(int));
		headerChar[4] = '\0';

		header.id = std::string(headerChar);
		header.version = std::to_string(version);
	}
	else
	{
		std::cout << "invalid file path: " << path << '\n';
	}

	return header;
}

vox_header vox_parser::read_vox_metadata(const void* bytes)
{
	return {};
}

// NOTE: this only reads the basic type (exported with the lower right menu)
std::shared_ptr<vox_file> vox_parser::read_vox_file(const char* path)
{
	std::ifstream voxFile(path, std::ios::in | std::ios::binary);

	if (!voxFile.is_open())
	{
		std::cout << "invalid path file: " << path;

		return nullptr;
	}

	std::shared_ptr<vox_file> vox = std::make_shared<vox_file>();
	vox->pallete = {};
	vox->transforms = {};
	vox->shapes = {};
	vox->groups = {};
	vox->materials = {};
	vox->sizes = {};
	vox->voxModels = {};

	char id[5];

	voxFile.read(id, 4);
	id[4] = '\0';

	if (std::strcmp(id, "VOX ") == 0)
	{
		vox->header.id = id;
	}
	else
	{
		return vox;
	}

	int version{};
	voxFile.read(reinterpret_cast<char*>(&version), 4);
	vox->header.version = std::to_string(version);

	char main[5];
	voxFile.read(main, 4);
	main[4] = '\0';

	if (std::strcmp(main, "MAIN") == 0)
	{
		char pack[5];
		voxFile.read(pack, 4);
		pack[4] = '\0';

		if (std::strcmp(pack, "PACK") == 0)
		{
			// how many SIZE and XYZI this vox contains
			parse_PACK(vox, voxFile);
		}

		while (!voxFile.eof())
		{
			char chunk[5];
			voxFile.read(chunk, 4);
			chunk[4] = '\0';

			if (std::strcmp(chunk, "SIZE") == 0)
			{
				parse_SIZE(vox, voxFile);
			}
			else if (std::strcmp(chunk, "XYZI") == 0)
			{
				parse_XYZI(vox, voxFile);
			}
			else if (std::strcmp(chunk, "nTRN") == 0)
			{
				parse_nTRN(vox, voxFile);
			}
			else if (std::strcmp(chunk, "nGRP") == 0)
			{
				parse_nGRP(vox, voxFile);
			}
			else if (std::strcmp(chunk, "nSHP") == 0)
			{
				parse_nSHP(vox, voxFile);
			}
			else if (std::strcmp(chunk, "MATL") == 0)
			{
				parse_MATL(vox, voxFile);
			}
			else if (std::strcmp(chunk, "RGBA") == 0)
			{
				parse_RGBA(vox, voxFile);
			}
		}

		if (voxFile.eof())
		{
			std::cout << "vox file fully read\n";

			if (vox->pallete.empty()) 
			{
				// TODO: use default pallete
			}
		}
	}
	else
	{
		std::cout << "invalid .vox file";
		return vox;
	}

	vox->isValid = true;

	return vox;
}

void vox_parser::parse_SIZE(std::shared_ptr<vox_file> vox, std::ifstream& voxFile)
{
	int numBytesChunkContent = {};
	voxFile.read(reinterpret_cast<char*>(&numBytesChunkContent), 4);

	int numBytesChildrenChunks = {};
	voxFile.read(reinterpret_cast<char*>(&numBytesChildrenChunks), 4);

	vox_size& size = vox->sizes.emplace_back();

	voxFile.read(reinterpret_cast<char*>(&size.x), 4);
	voxFile.read(reinterpret_cast<char*>(&size.y), 4); 
	voxFile.read(reinterpret_cast<char*>(&size.z), 4);

	std::cout << "sX: " << size.x << ", sY: " << size.y << ", sZ: " << size.z << '\n';
}

void vox_parser::parse_XYZI(std::shared_ptr<vox_file> vox, std::ifstream& voxFile)
{
	int numBytesChunkContent = {};
	voxFile.read(reinterpret_cast<char*>(&numBytesChunkContent), 4);

	int numBytesChildrenChunks = {};
	voxFile.read(reinterpret_cast<char*>(&numBytesChildrenChunks), 4);

	int voxelCount{};
	voxFile.read(reinterpret_cast<char*>(&voxelCount), 4);

	vox_model& model = vox->voxModels.emplace_back();
	model.voxels.resize(voxelCount);
	
	model.boundingBox = 
	{
		static_cast<float>(INT_MAX),
		static_cast<float>(INT_MAX),
		static_cast<float>(INT_MAX),

		static_cast<float>(INT_MIN),
		static_cast<float>(INT_MIN),
		static_cast<float>(INT_MIN),
	};
	
	const vox_size size = vox->sizes[modelIndex++];

	model.voxel_3dGrid = new int**[size.z];

	for (int z = 0; z < size.z; z++)
	{
		model.voxel_3dGrid[z] = new int* [size.y];

		for (int y = 0; y < size.y; y++)
		{
			model.voxel_3dGrid[z][y] = new int[size.x];

			for (int x = 0; x < size.x; x++)
			{
				model.voxel_3dGrid[z][y][x] = -1;
			}
		}
	}

	for (size_t i = 0; i < voxelCount; i++)
	{
		vox_voxel& voxel = model.voxels[i];

		voxFile.read(reinterpret_cast<char*>(&voxel.x), sizeof(char));
		voxFile.read(reinterpret_cast<char*>(&voxel.y), sizeof(char));
		voxFile.read(reinterpret_cast<char*>(&voxel.z), sizeof(char));
		voxFile.read(reinterpret_cast<char*>(&voxel.colorIndex), sizeof(char));

		model.voxel_3dGrid[voxel.z][voxel.y][voxel.x] = voxel.colorIndex;

		model.boundingBox.minX = std::min(model.boundingBox.minX, static_cast<float>(voxel.x));
		model.boundingBox.minY = std::min(model.boundingBox.minY, static_cast<float>(voxel.y));
		model.boundingBox.minZ = std::min(model.boundingBox.minZ, static_cast<float>(voxel.z));

		model.boundingBox.maxX = std::max(model.boundingBox.maxX, static_cast<float>(voxel.x));
		model.boundingBox.maxY = std::max(model.boundingBox.maxY, static_cast<float>(voxel.y));
		model.boundingBox.maxZ = std::max(model.boundingBox.maxZ, static_cast<float>(voxel.z));
	}
}

void vox_parser::parse_RGBA(std::shared_ptr<vox_file> vox, std::ifstream& voxFile)
{
	constexpr int size = 255;

	vox->pallete.resize(size);

	// When reading these colors to paint the textures keep in mind that the reading from the array starts from 1: 
	/** <NOTICE>
			  | * color [0-254] are mapped to palette index [1-255], e.g :
			  |
			  | for ( int i = 0; i <= 254; i++ )  {
			  |     palette[i + 1] = ReadRGBA();
			  | }*/

	for (size_t i = 0; i < vox->pallete.size(); i++)
	{
		color& color = vox->pallete[i];

		voxFile.read(reinterpret_cast<char*>(&color.r), sizeof(char));
		voxFile.read(reinterpret_cast<char*>(&color.g), sizeof(char));
		voxFile.read(reinterpret_cast<char*>(&color.b), sizeof(char));
		voxFile.read(reinterpret_cast<char*>(&color.a), sizeof(char));
	}
}

void vox_parser::parse_PACK(std::shared_ptr<vox_file> vox, std::ifstream& voxFile)
{

}

void vox_parser::parse_nTRN(std::shared_ptr<vox_file> vox, std::ifstream& voxFile)
{
	// TODO: in some platforms 'int' is not 4 bytes, use an alias instead of'int'

	int numBytesChunkContent = {};
	voxFile.read(reinterpret_cast<char*>(&numBytesChunkContent), 4);

	int numBytesChildrenChunks = {};
	voxFile.read(reinterpret_cast<char*>(&numBytesChildrenChunks), 4);


	int id{};
	voxFile.read(reinterpret_cast<char*>(&id), 4);

	vox_nTRN& transform = vox->transforms[id];
	transform.nodeID = id;

	int kvPairs{};
	voxFile.read(reinterpret_cast<char*>(&kvPairs), 4);

	for (size_t i = 0; i < kvPairs; i++)
	{
		// TODO
		std::cout << "parse_nTRN, TODO: kvPairs for\n";
		throw;
		int keyBufferSize{};
		voxFile.read(reinterpret_cast<char*>(&keyBufferSize), 4);
	}

	voxFile.read(reinterpret_cast<char*>(&transform.childNodeID), 4);
	voxFile.read(reinterpret_cast<char*>(&transform.reservedID), 4);
	voxFile.read(reinterpret_cast<char*>(&transform.layerID), 4);
	voxFile.read(reinterpret_cast<char*>(&transform.framesCount), 4);

	transform.frameAttrib.resize(transform.framesCount);

	for (size_t i = 0; i < transform.framesCount; i++)
	{
		int kvPairFrames{};
		voxFile.read(reinterpret_cast<char*>(&kvPairFrames), 4);

		vox_frame_attrib& frameAttrib = transform.frameAttrib[i];

		frameAttrib.translation = { };
		frameAttrib.rotation = vox_imat3::identity;
		frameAttrib.frameIndex = i;

		for (size_t j = 0; j < kvPairFrames; j++)
		{
			int keyStrLen{};
			voxFile.read(reinterpret_cast<char*>(&keyStrLen), 4);
			
			std::string st{};
			st.resize(keyStrLen);
			voxFile.read(st.data(), st.size());

			int valStrLen{};
			voxFile.read(reinterpret_cast<char*>(&valStrLen), 4);

			if (st.compare("_r") == 0)
			{
				std::string rot_str{};
				rot_str.resize(valStrLen);

				voxFile.read(reinterpret_cast<char*>(rot_str.data()), valStrLen);

				static const uint32_t k_row2_index[] = { UINT32_MAX, UINT32_MAX, UINT32_MAX, 2, UINT32_MAX, 1, 0, UINT32_MAX };

				uint32_t packed_rotation_bits = atoi(rot_str.c_str());
				uint32_t row0_vec_index = (packed_rotation_bits >> 0) & 3;
				uint32_t row1_vec_index = (packed_rotation_bits >> 2) & 3;
				uint32_t row2_vec_index = k_row2_index[(1 << row0_vec_index) | (1 << row1_vec_index)];

				// lookup table for _vox_make_transform_from_dict_strings
				static const vox_vec3 k_vectors[4] = 
				{
					vox_vec3{1.0f, 0.0f, 0.0f},
					vox_vec3{0.0f, 1.0f, 0.0f},
					vox_vec3{0.0f, 0.0f, 1.0f},
					vox_vec3{0.0f, 0.0f, 0.0f}    // invalid!
				};

				vox_vec3 row0 = k_vectors[row0_vec_index];
				vox_vec3 row1 = k_vectors[row1_vec_index];
				vox_vec3 row2 = k_vectors[row2_vec_index];

				if (packed_rotation_bits & (1 << 4))
					row0 = row0 * -1;
				if (packed_rotation_bits & (1 << 5))
					row1 = row1 * -1;
				if (packed_rotation_bits & (1 << 6))
					row2 = row2 * -1;

				vox_imat3 rotMatrix{};

				// magicavoxel stores rows, we need columns, so we do the swizzle here into columns
				rotMatrix.m00 = row0.x; rotMatrix.m01 = row1.x; rotMatrix.m02 = row2.x;
				rotMatrix.m10 = row0.y; rotMatrix.m11 = row1.y; rotMatrix.m12 = row2.y;
				rotMatrix.m20 = row0.z; rotMatrix.m21 = row1.z; rotMatrix.m22 = row2.z;

				frameAttrib.rotation = rotMatrix;// rotMatrix.transpose();

				//char _r{}; 
				//
				//if (valStrLen == 1) 
				//{
				//	_r = (digits[0]);
				//}
				//else if (valStrLen == 2) 
				//{
				//	_r = (digits[0] - '0') * 10 + (digits[1] - '0');
				//}
				//else 
				//{
				//	throw; // this should never happen
				//}


				////std::string s_r;
				////s_r.resize(valStrLen);

				////voxFile.read(&s_r[0], valStrLen);
				////const char _r = static_cast<char>(std::atoi(s_r.c_str())); // Combine into a single byte


				//int rotation[3][3] = { 0 };

				///*for (int k = 0; k < 3; ++k) 
				//{
				//	for (int l = 0; l < 3; ++l) 
				//	{
				//		rotation[k][l] = 0;
				//	}
				//}*/

				//int row1_index =		_r & 0x03; // Bits 0-1
				//int row2_index = (_r >> 2) & 0x03; // Bits 2-3
				//int row3_index = (_r >> 3) & 0x03; // Bits 4-5
				//bool row1_sign = (_r >> 4) & 0x01;  // Bit 4
				//bool row2_sign = (_r >> 5) & 0x01;  // Bit 5
				//bool row3_sign = (_r >> 6) & 0x01;  // Bit 6

				//rotation[0][row1_index] = row1_sign ? -1 : 1; 
				//rotation[1][row2_index] = row2_sign ? -1 : 1; 
				//rotation[2][row3_index] = row3_sign ? -1 : 1; 

				//vox_imat3 rotMatrix{};
				//rotMatrix.m00 = rotation[0][0];
				//rotMatrix.m01 = rotation[0][1];
				//rotMatrix.m02 = rotation[0][2];
				//
				//rotMatrix.m10 = rotation[1][0];
				//rotMatrix.m11 = rotation[1][1];
				//rotMatrix.m12 = rotation[1][2];

				//rotMatrix.m20 = rotation[2][0];
				//rotMatrix.m21 = rotation[2][1];
				//rotMatrix.m22 = rotation[2][2];

				//frameAttrib.rotation = rotMatrix.transpose();
			}
			else if (st.compare("_t") == 0)
			{
				std::vector<char> valstr(valStrLen);
				voxFile.read(valstr.data(), valstr.size());
				valstr.push_back('\0');

				std::string stv(valstr.data());
				std::vector<int> whiteSpaces{};

				for (size_t i = 0; i < stv.size(); i++)
				{
					if (stv[i] == ' ') 
					{
						whiteSpaces.push_back(i);
					}
				}

				frameAttrib.translation.x = std::atoi(stv.substr(0, whiteSpaces[0]).c_str());
				frameAttrib.translation.y = std::atoi(stv.substr(whiteSpaces[0] + 1, whiteSpaces[1]-2).c_str());
				frameAttrib.translation.z = std::atoi(stv.substr(whiteSpaces[1] + 1, stv.size() - 1 - whiteSpaces[1]).c_str());
			}
			else if (st.compare("_f") == 0)
			{
				std::vector<char> valstr(valStrLen);
				voxFile.read(valstr.data(), valstr.size());
				valstr.push_back('\0');
				//--frameAttrib.frameIndex = { -1 };
			}
			else 
			{
				int valStrLen{};
				voxFile.read(reinterpret_cast<char*>(&valStrLen), 4);
				std::vector<char> valstr(valStrLen);
				voxFile.read(valstr.data(), valstr.size());
				valstr.push_back('\0');
			}
		}
	}
}

void vox_parser::parse_nGRP(std::shared_ptr<vox_file> vox, std::ifstream& voxFile)
{
	int numBytesChunkContent = {};
	voxFile.read(reinterpret_cast<char*>(&numBytesChunkContent), 4);

	int numBytesChildrenChunks = {};
	voxFile.read(reinterpret_cast<char*>(&numBytesChildrenChunks), 4);

	int nodeID{};
	voxFile.read(reinterpret_cast<char*>(&nodeID), 4);

	// If this fails, it means that the ids are not unique for the children, so I will need to change from dictionary to vector hierarchy
	vox_nGRP& group = vox->groups[nodeID];
	group.nodeID = nodeID;

	// TODO:
	int kvPairs{};
	voxFile.read(reinterpret_cast<char*>(&kvPairs), 4);
	for (size_t i = 0; i < kvPairs; i++)
	{
		int strSize{};
		voxFile.read(reinterpret_cast<char*>(&strSize), 4);
	}

	voxFile.read(reinterpret_cast<char*>(&group.childrenCount), 4);

	group.childrenIDs.resize(group.childrenCount);

	for (size_t i = 0; i < group.childrenCount; i++)
	{
		voxFile.read(reinterpret_cast<char*>(&group.childrenIDs[i]), 4);
	}
}

void vox_parser::parse_nSHP(std::shared_ptr<vox_file> vox, std::ifstream& voxFile)
{
	int numBytesChunkContent = {};
	voxFile.read(reinterpret_cast<char*>(&numBytesChunkContent), 4);

	int numBytesChildrenChunks = {};
	voxFile.read(reinterpret_cast<char*>(&numBytesChildrenChunks), 4);

	int nodeID{};
	voxFile.read(reinterpret_cast<char*>(&nodeID), 4);

	// If this fails, it means that the ids are not unique for the children, so I will need to change from dictionary to vector hierarchy
	vox_nSHP& shapes = vox->shapes[nodeID];
	shapes.nodeID = nodeID;

	// TODO:
	int kvPairs{};
	voxFile.read(reinterpret_cast<char*>(&kvPairs), 4);
	for (size_t i = 0; i < kvPairs; i++)
	{
		int strSize{};
		voxFile.read(reinterpret_cast<char*>(&strSize), 4);
	}


	voxFile.read(reinterpret_cast<char*>(&shapes.modelsCount), 4);
	shapes.models.resize(shapes.modelsCount);

	for (size_t i = 0; i < shapes.modelsCount; i++)
	{
		vox_nSHP_model& model = shapes.models[i];

		voxFile.read(reinterpret_cast<char*>(&model.modelID), 4);

		// TODO:
		int kvPairsModel{};
		voxFile.read(reinterpret_cast<char*>(&kvPairsModel), 4);
		for (size_t j = 0; j < kvPairsModel; j++)
		{
			int strSizeModel{};
			voxFile.read(reinterpret_cast<char*>(&strSizeModel), 4);

			int keyStrLen{};
			voxFile.read(reinterpret_cast<char*>(&keyStrLen), 4);
			std::vector<char> str(keyStrLen);
			voxFile.read(str.data(), str.size());
			str.push_back('\0');

			std::string st(str.data());

			voxFile.read(reinterpret_cast<char*>(&model.frameIndex), 4);
		}
	}

}

void vox_parser::parse_MATL(std::shared_ptr<vox_file> vox, std::ifstream& voxFile)
{
	int numBytesChunkContent = {};
	voxFile.read(reinterpret_cast<char*>(&numBytesChunkContent), 4);

	int numBytesChildrenChunks = {};
	voxFile.read(reinterpret_cast<char*>(&numBytesChildrenChunks), 4);

	int matID{};
	voxFile.read(reinterpret_cast<char*>(&matID), 4);

	vox_MATL& material = vox->materials[matID];

	material.diffuse = { 1.0 };
	material.metal = {};
	material.glass = {};
	material.emit = {};
	material.weight = {};
	material.rough = {};
	material.spec = {};
	material.ior = {};
	material.att = {};
	material.flux = {};
	material.plastic = {};

	int kvPairs{};
	voxFile.read(reinterpret_cast<char*>(&kvPairs), 4);

	for (size_t i = 0; i < kvPairs; i++)
	{
		int keyStrLen{};
		voxFile.read(reinterpret_cast<char*>(&keyStrLen), 4);
		std::vector<char> str(keyStrLen);
		voxFile.read(str.data(), str.size());
		str.push_back('\0');

		int valStrLen{};
		voxFile.read(reinterpret_cast<char*>(&valStrLen), 4);
		std::vector<char> valstr(valStrLen);
		voxFile.read(valstr.data(), valstr.size());
		valstr.push_back('\0');

		std::string k_str(str.data());
		std::string v_str(valstr.data());

		if (!v_str.empty())
		{
			if (k_str.compare("_d") == 0)
			{
				material.diffuse = std::stof(v_str);
			}
			else if (k_str.compare("_weight") == 0)
			{
				material.weight = std::stof(v_str);
			}
			else if (k_str.compare("_rough") == 0)
			{
				material.rough = std::stof(v_str);
			}
			else if (k_str.compare("_spec") == 0)
			{
				material.spec = std::stof(v_str);
			}
			else if (k_str.compare("_ior") == 0)
			{
				material.ior = std::stof(v_str);
			}
			else if (k_str.compare("_att") == 0)
			{
				material.att = std::stof(v_str);
			}
			else if (k_str.compare("_flux") == 0)
			{
				material.flux = std::stof(v_str);
			}
			else
			{
			}
		}
	}
}

// default pallete
const std::vector<unsigned int> vox_parser::default_pallete =
{
	0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff,
	0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff,
	0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc,
	0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc,
	0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc,
	0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999,
	0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399,
	0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66,
	0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366,
	0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33,
	0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633,
	0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00,
	0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900,
	0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000,
	0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00,
	0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000,

	0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff,
	0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff,
	0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc,
	0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc,
	0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99,
	0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699,
	0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099,
	0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966,
	0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066,
	0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33,
	0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333,
	0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00,
	0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600,
	0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa,
	0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100,
	0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777,

	0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
	0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
	0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
	0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
	0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
	0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
	0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
	0xff669966, 0xff339966, 0xff009966, 0xffff6666,
	0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
	0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
	0xff663333, 0xff333333, 0xff003333, 0xffff0033,
	0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
	0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
	0xff000088, 0xff000077, 0xff000055, 0xff000044,
	0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
	0xff555555, 0xff444444, 0xff222222, 0xff111111
};

