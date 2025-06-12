#ifndef VOXELLER_API_EXPORT
#define VOXELLER_API_EXPORT
#endif
#include <iostream>

#include <voxeller/vox_mesh_builder.h>
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/mesh.h>

void DestroyScene(aiScene* scene) 
{
    if (!scene)
        return;

    //// Delete meshes
    //for (unsigned int i = 0; i < scene->mNumMeshes; ++i) 
    //{
    //    delete[] scene->mMeshes[i]->mVertices;
    //    delete[] scene->mMeshes[i]->mNormals;  // Delete normals
    //    delete[] scene->mMeshes[i]->mTextureCoords[0]; // Delete UVs

    //    for (size_t i = 0; i < scene->mMeshes[i]->mFaces->mNumIndices; i++)
    //    {
    //        delete[] scene->mMeshes[i]->mFaces[i]; // Delete UVs

    //    }

    //    // Delete other mesh data (e.g., faces, tangents, bitangents, etc.) if allocated
    //    delete scene->mMeshes[i];
    //}
    //delete[] scene->mMeshes;

    // Delete materials, animations, lights, cameras, textures, and other scene data if allocated

    // Delete the scene itself
  //  delete scene;
}

std::shared_ptr<vox_scene> vox_mesh_builder::build_mesh(const std::shared_ptr<vox_file> vox, const mesh_algorithm algorithm)
{
	switch (algorithm)
	{
	case mesh_algorithm::VOXEL:
		return build_mesh_voxel(vox);
	case mesh_algorithm::SMALL_TEXTURE:
		return build_mesh_small_texture(vox);
	case mesh_algorithm::GREEDY_MESHING:
		return build_mesh_greedy(vox);
	default:
		std::cout << "meshing algorithm not implemented";
		break;
	}

	return nullptr;
}

std::vector<vox_voxel> vox_mesh_builder::removeHiddenVoxels(const vox_size& size, const vox_model& model)
{
    std::vector<vox_voxel> voxels{};

    auto canAdd = [&model](int x, int y, int z) -> bool
    {
        const int colorAdj1 = model.voxel_3dGrid[z + 1][y][x];// right voxel
        const int colorAdj2 = model.voxel_3dGrid[z - 1][y][x];// left voxel
        const int colorAdj3 = model.voxel_3dGrid[z][y + 1][x];// front voxel
        const int colorAdj4 = model.voxel_3dGrid[z][y - 1][x];// back voxel
        const int colorAdj5 = model.voxel_3dGrid[z][y][x + 1];// top voxel
        const int colorAdj6 = model.voxel_3dGrid[z][y][x - 1];// bottom voxel

        return (colorAdj1 == -1 ||
                colorAdj2 == -1 ||
                colorAdj3 == -1 ||
                colorAdj4 == -1 ||
                colorAdj5 == -1 ||
                colorAdj6 == -1);
    };

    for (int z = 0; z < size.z; z++)
    {
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                const int colorIndex = model.voxel_3dGrid[z][y][x];

              /*  bool isAdd = canAdd(x, y, z);

                bool canRemoveDueAdjancents = (x > 0 && y > 0 && z > 0 &&
                                               x + 1 < size.x && y + 1 < size.y && 
                                               z + 1< size.z) && (
                                canAdd(x + 1, y, z) ||
                                canAdd(x - 1, y, z) ||
                                canAdd(x, y + 1, z) ||
                                canAdd(x, y - 1, z) ||
                                canAdd(x, y, z + 1) ||
                                canAdd(x, y, z - 1));

                if (colorIndex && isAdd && canRemoveDueAdjancents)
                {
                    voxels.push_back({ x, y, z, colorIndex });
                }*/

                if (colorIndex != -1)
                {
                    if (x > 0 && y > 0 && z > 0 && 
                        x + 1 < size.x && y + 1 < size.y && z + 1 < size.z) 
                    {
                        const int colorAdj1 = model.voxel_3dGrid[z + 1][y][x];// right voxel
                        const int colorAdj2 = model.voxel_3dGrid[z - 1][y][x];// left voxel
                        const int colorAdj3 = model.voxel_3dGrid[z][y + 1][x];// front voxel
                        const int colorAdj4 = model.voxel_3dGrid[z][y - 1][x];// back voxel
                        const int colorAdj5 = model.voxel_3dGrid[z][y][x + 1];// top voxel
                        const int colorAdj6 = model.voxel_3dGrid[z][y][x - 1];// bottom voxel

                        if (colorAdj1 == -1 ||
                            colorAdj2 == -1 ||
                            colorAdj3 == -1 ||
                            colorAdj4 == -1 ||
                            colorAdj5 == -1 ||
                            colorAdj6 == -1)
                        {   
                            voxels.push_back({ x, y, z, colorIndex });
                        }
                    }
                    else 
                    {
                        voxels.push_back({x, y, z, colorIndex });
                    }
                }
            }
        }
    }

    return voxels;
}

std::shared_ptr<vox_scene> vox_mesh_builder::build_mesh_voxel(const std::shared_ptr<vox_file> vox)
{
    // Create a new scene
    aiScene* scene = new aiScene();
    //std::vector<aiMesh*> meshes{};
    scene->mRootNode = new aiNode();
    scene->mRootNode->mName = "root";
   

    struct model_info 
    {
        int index;
        int nodeId;
    };

    std::vector<model_info> modelIndexes{};

    if (vox->shapes.size() > 0) 
    {
        for (size_t i = 0; i < vox->shapes.size(); i++)
        {
            auto it = vox->shapes.begin();
            std::advance(it, i);

            // TODO: take frames into account
            const vox_nSHP& shape = it->second;
            
            for (size_t j = 0; j < shape.models.size(); j++)
            {
                const int modelIndex = shape.models[j].modelID;
                //shape.attributes
                modelIndexes.push_back({ modelIndex, shape.nodeID });
            }
        }
    }
    else 
    {
        // Use default indexing
        modelIndexes.resize(vox->voxModels.size());
        for (int i = 0; i < vox->voxModels.size(); i++)
        {
            modelIndexes[i] = { i, i };
        }
    }

    scene->mMeshes = new aiMesh * [modelIndexes.size()];
    scene->mNumMeshes = modelIndexes.size();



	for (size_t i = 0; i < modelIndexes.size(); i++)
	{
		vox_model& vox_model = vox->voxModels.at(modelIndexes[i].index);
        const vox_size& voxSize = vox->sizes.at(modelIndexes[i].index);

        bool flipX = true;
        bool flipY = false;
        bool flipZ = false;

        const float xSign = flipX ? -1 : 1;
        const float ySign = flipY ? -1 : 1;
        const float zSign = flipZ ? -1 : 1;

        constexpr int cubeVerts = 8;
        constexpr int faces = 12;
        constexpr int faceIndexCount = 3;
        constexpr float offset = 0.5f;
        float scale = 100;

        vox_vec3 meshStartPos{};
        vox_imat3 rotation = vox_imat3::identity;

        const vox_vec3 model_center =
        {
            (vox_model.boundingBox.maxX + vox_model.boundingBox.minX) / 2.0f,
            (vox_model.boundingBox.maxY + vox_model.boundingBox.minY) / 2.0f,
            (vox_model.boundingBox.maxZ + vox_model.boundingBox.minZ) / 2.0f
        };

        if (vox->transforms.size() > 0)
        {
            const vox_nTRN& transform = vox->transforms.at(modelIndexes[i].nodeId - 1);

            if (transform.framesCount > 0)
            {
                const vox_frame_attrib& frameAttrib = transform.frameAttrib[0];
                meshStartPos = frameAttrib.translation;
                rotation = frameAttrib.rotation;
                
            }
        }

        const vox_vec3 offset_v = rotation * vox_vec3{ offset , offset , offset };

        // Create a new mesh
        aiMesh* mesh = new aiMesh();
        scene->mMeshes[i] = mesh;

        int size = vox_model.voxels.size();
        vox_model.voxels = removeHiddenVoxels(voxSize, vox_model);

        mesh->mPrimitiveTypes = aiPrimitiveType::aiPrimitiveType_TRIANGLE;
        //mesh->mNumVertices = cubeVerts * vox_model.voxels.size();
        //mesh->mNumFaces = faces * vox_model.voxels.size();

        auto child = new aiNode();
        child->mName = "vox" + std::to_string(i);
        child->mParent = scene->mRootNode;
        child->mMeshes = new unsigned int[1];
        child->mMeshes[0] = i;
        child->mNumMeshes = 1;
        scene->mRootNode->addChildren(1, &child);
        scene->mMaterials = new aiMaterial * [1]; // TODO: select if materials will be separrated per mesh, or a single material will be used for all meshes.

        auto mat = new aiMaterial();
        aiString matName("default");

        mat->AddProperty(&matName, AI_MATKEY_NAME);
        scene->mMaterials[0] = mat;
        scene->mNumMaterials = 1;

        // Set mesh position
        const aiVector3D originTranslation(xSign * (meshStartPos.x - vox_math::frac(model_center.x)) * scale,
                                           ySign * (meshStartPos.z - vox_math::frac(model_center.z)) * scale,
                                           zSign * (meshStartPos.y - vox_math::frac(model_center.y)) * scale); // Example translation values

        aiMatrix4x4::Translation(originTranslation, child->mTransformation);
        
        const vox_ivec3 faceIndices[faces] =
        {
            { 2, 0, 1 }, // topTriangle1
            { 1, 3, 2 }, // topTriangle2

            { 6, 4, 5 }, // bottomTriangle1
            { 5, 7, 6 }, // bottomTriangle2

            { 5, 1, 0 }, // frontTriangle1
            { 0, 4, 5 }, // frontTriangle2

           //{ 7, 3, 2 }, // backTriangle1
           //{ 2, 6, 7 }, // backTriangle2

            { 6, 2, 3 }, // backTriangle1
            { 3, 7, 6 }, // backTriangle2

            { 7, 3, 1 }, // rightTriangle1
            { 1, 5, 7 }, // rightTriangle2

            { 6, 2, 0 }, // leftTriangle1
            { 0, 4, 6 },  // leftTriangle2
        };

        const vox_ivec3 normals[] =
        {
            {  0,  1,  0 },
            {  0, -1,  0 },
            {  0,  0,  1 },
            {  0,  0,   -1 },
            {  1,  0,  0 },
            {  -1,  0,  0 },
        };

        std::vector<aiVector3D> validVertices{};
        std::vector<aiFace> validFaces{};

        //mesh->mNormals = new aiVector3D[mesh->mNumVertices];

		for (size_t j = 0; j < vox_model.voxels.size(); j++)
		{
			const vox_voxel& voxel = vox_model.voxels.at(j);

            const vox_vec3 vertsPosition[cubeVerts] =
            {
                { voxel.x - offset, voxel.y + offset, voxel.z + offset }, // top_front_left     0
                { voxel.x + offset, voxel.y + offset, voxel.z + offset }, // top_front_right    1

                { voxel.x - offset, voxel.y + offset, voxel.z - offset }, // top_back_left      2
                { voxel.x + offset, voxel.y + offset, voxel.z - offset }, // top_back_right     3

                { voxel.x - offset, voxel.y - offset, voxel.z + offset }, // bottom_front_left  4
                { voxel.x + offset, voxel.y - offset, voxel.z + offset }, // bottom_front_right 5

                { voxel.x - offset, voxel.y - offset, voxel.z - offset }, // bottom_back_left   6
                { voxel.x + offset, voxel.y - offset, voxel.z - offset }, // bottom_back_right  7
            };

            auto shouldRenderFace = [&voxSize, &vox_model](int x, int y, int z) -> bool
            {
                if (x >= 0 && y >= 0 && z >= 0 && x < voxSize.x && y < voxSize.y && z < voxSize.z)
                {
                    // if there is no voxel here, then, a face should be rendered.
                    return vox_model.voxel_3dGrid[z][y][x] == -1;
                }

                return true;
            };


            for (size_t k = 0; k < faces; k++)
            {
                int offsetX = 0;
                int offsetY = 0;
                int offsetZ = 0;

                if (k <= 1)
                {
                    offsetY = 1;
                }
                else if (k <= 3)
                {
                    offsetY = -1;
                }
                else if (k <= 5)
                {
                    offsetZ = 1;
                }
                else if (k <= 7)
                {
                    offsetZ = -1;
                }
                else if (k <= 9)
                {
                    offsetX = 1;
                }
                else if (k <= 11)
                {
                    offsetX = -1;
                }

                const bool isValidFace = shouldRenderFace(voxel.x + offsetX, 
                                                          voxel.y + offsetY, 
                                                          voxel.z + offsetZ);

                if (isValidFace)
                {
                    const int vIndex1 = 0 + validVertices.size();
                    const int vIndex2 = 1 + validVertices.size();
                    const int vIndex3 = 2 + validVertices.size();
                    const int vIndex4 = 2 + validVertices.size();
                    const int vIndex5 = 3 + validVertices.size();
                    const int vIndex6 = 0 + validVertices.size();

                    aiFace vface1{};
                    vface1.mNumIndices = faceIndexCount;
                    vface1.mIndices = new unsigned int[vface1.mNumIndices];

                    aiFace vface2{};
                    vface2.mNumIndices = faceIndexCount;
                    vface2.mIndices = new unsigned int[vface2.mNumIndices];

                    vface1.mIndices[0] = vIndex1;
                    vface1.mIndices[1] = vIndex2;
                    vface1.mIndices[2] = vIndex3;

                    vface2.mIndices[0] = vIndex4;
                    vface2.mIndices[1] = vIndex5;
                    vface2.mIndices[2] = vIndex6;

                    validFaces.emplace_back(vface1);
                    validFaces.emplace_back(vface2);

                    const vox_ivec3& vertIndex = faceIndices[k];

                    const vox_vec3 vert1 = rotation * vertsPosition[vertIndex.x];
                    validVertices.push_back(aiVector3D(xSign* (vert1.x - (model_center.x) + xSign * std::floor(offset_v.x))* scale,
                        ySign* (vert1.z - (model_center.z) + ySign * std::floor(offset_v.z))* scale,
                        zSign* (vert1.y - (model_center.y) + zSign * std::floor(offset_v.y))* scale));

                    const vox_vec3 vert2 = rotation * vertsPosition[vertIndex.y];
                    validVertices.push_back(aiVector3D(xSign * (vert2.x - (model_center.x) + xSign * std::floor(offset_v.x)) * scale,
                        ySign * (vert2.z - (model_center.z) + ySign * std::floor(offset_v.z)) * scale,
                        zSign * (vert2.y - (model_center.y) + zSign * std::floor(offset_v.y)) * scale));

                    const vox_vec3 vert3 = rotation * vertsPosition[vertIndex.z];
                    validVertices.push_back(aiVector3D(xSign* (vert3.x - (model_center.x) + xSign * std::floor(offset_v.x))* scale,
                        ySign* (vert3.z - (model_center.z) + ySign * std::floor(offset_v.z))* scale,
                        zSign* (vert3.y - (model_center.y) + zSign * std::floor(offset_v.y))* scale));

                    const vox_ivec3& vertIndex2 = faceIndices[k+1];

                    const vox_vec3 vert4 = rotation * vertsPosition[vertIndex2.y];
                    validVertices.push_back(aiVector3D(xSign * (vert4.x - (model_center.x) + xSign * std::floor(offset_v.x)) * scale,
                        ySign * (vert4.z - (model_center.z) + ySign * std::floor(offset_v.z)) * scale,
                        zSign * (vert4.y - (model_center.y) + zSign * std::floor(offset_v.y)) * scale));

                    k++;
                }
                else
                {
                    //vface.mNumIndices = faceIndexCount;
                    //vface.mIndices = new unsigned int[faceIndexCount];

                }

                //const vox_ivec3& normal = normals[static_cast<int>(std::floor(k / 2))];// vox_math::normalize(vox_math::cross(v2 - v1, v3 - v1));

                //for (int l = 0; l < vface.mNumIndices; l++)
                //{
                //    mesh->mNormals[vface.mIndices[l]] = aiVector3D(normal.x, normal.y, normal.z);
                //}



                //const aiVector3D& vert1 = mesh->mVertices[vertIndex.x + j * cubeVerts];
                //const aiVector3D& vert2 = mesh->mVertices[vertIndex.y + j * cubeVerts];
                //const aiVector3D& vert3 = mesh->mVertices[vertIndex.z + j * cubeVerts];

                //vox_vec3 v1 = { vert1.x,  vert1.y, vert1.z };
                //vox_vec3 v2 = { vert2.x,  vert2.y, vert2.z };
                //vox_vec3 v3 = { vert3.x,  vert3.y, vert3.z };

                //// This is very slow, please bake the values
                //const vox_ivec3& normal = vox_math::normalize(vox_math::cross(v2 - v1, v3 - v1));

                //mesh->mNormals[vertIndex.x + j * cubeVerts] = aiVector3D(normal.x, normal.z,  normal.y);
                //mesh->mNormals[vertIndex.y + j * cubeVerts] = aiVector3D(normal.x, normal.z,  normal.y);
                //mesh->mNormals[vertIndex.z + j * cubeVerts] = aiVector3D(normal.x, normal.z,  normal.y);
            }

            // Add UV coordinates
            //mesh->mNumUVComponents[0] = 2; // 2 components per UV coordinate (u, v)
            //mesh->mTextureCoords[0] = new aiVector3D[4]; // 4 UV coordinates for the 4 vertices
            //mesh->mTextureCoords[0][0] = aiVector3D(0, 0, 0); // UV for vertex 0
            //mesh->mTextureCoords[0][1] = aiVector3D(1, 0, 0); // UV for vertex 1
            //mesh->mTextureCoords[0][2] = aiVector3D(1, 1, 0); // UV for vertex 2
            //mesh->mTextureCoords[0][3] = aiVector3D(0, 1, 0); // UV for vertex 3


            //for (size_t l = 0; l < faces; l += 2)
            //{
            //    const vox_ivec3 vector = faceIndices[l];


            //    const aiVector3D& vert1 = mesh->mVertices[vector.x + j * cubeVerts];
            //    const aiVector3D& vert2 = mesh->mVertices[vector.y + j * cubeVerts];
            //    const aiVector3D& vert3 = mesh->mVertices[vector.z + j * cubeVerts];

            //    vox_vec3 v1 = { vert1.x,  vert1.y, vert1.z };
            //    vox_vec3 v2 = { vert2.x,  vert2.y, vert2.z };
            //    vox_vec3 v3 = { vert3.x,  vert3.y, vert3.z };

            //    const vox_ivec3& normal = normals[static_cast<int>(std::floor(l / 2))];// vox_math::normalize(vox_math::cross(v2 - v1, v3 - v1));

            //    // This is very slow, please bake the values
            //   // const vox_ivec3& normal = vox_math::normalize(vox_math::cross(v2 - v1, v3 - v1));

            //    auto realIndex1 = faceIndices[l];
            //    auto realIndex2 = faceIndices[l+1];

            //    mesh->mNormals[realIndex1.x + j * cubeVerts] = aiVector3D(normal.x, normal.z, normal.y);
            //    mesh->mNormals[realIndex1.y + j * cubeVerts] = aiVector3D(normal.x, normal.z, normal.y);
            //    mesh->mNormals[realIndex1.z + j * cubeVerts] = aiVector3D(normal.x, normal.z, normal.y);
            //    mesh->mNormals[realIndex2.x + j * cubeVerts] = aiVector3D(normal.x, normal.z, normal.y);
            //    mesh->mNormals[realIndex2.y + j * cubeVerts] = aiVector3D(normal.x, normal.z, normal.y);
            //    mesh->mNormals[realIndex2.z + j * cubeVerts] = aiVector3D(normal.x, normal.z, normal.y);
            //}
		}

        mesh->mNumVertices = validVertices.size();
        mesh->mVertices = new aiVector3D[mesh->mNumVertices];

        for (size_t l = 0; l < mesh->mNumVertices; l++)
        {
            mesh->mVertices[l] = validVertices.at(l);
        }

        mesh->mNumFaces = validFaces.size();
        mesh->mFaces = new aiFace[mesh->mNumFaces];
        
        for (size_t l = 0; l < mesh->mNumFaces; l++)
        {
            mesh->mFaces[l] = validFaces.at(l);
        }

        mesh->mNormals = nullptr;
	}

    // Export the scene
    Assimp::Exporter exporter;
    std::cout << "About to export\n";

    aiReturn result = exporter.Export(scene,"fbx", "scenes.fbx");
    std::cout << "exported\n";
    if (result != aiReturn_SUCCESS) 
    {
        // Handle export error
        std::cerr << "Error exporting scene: " << exporter.GetErrorString() << std::endl;
    }
    DestroyScene(scene);

	return {};
}

std::shared_ptr<vox_scene> vox_mesh_builder::build_mesh_small_texture(const std::shared_ptr<vox_file> vox)
{
	return {};
}

// Greedy meshing in all 6 directions with correct rotation, export-ready via Assimp
// Greedy meshing in all 6 directions using run-length merging (optimized topology)
// Greedy meshing in all 6 directions using full 3D surface merging with UVs and transform support
// Greedy meshing version that preserves rotation, translation, and correct axis orientation
// Greedy meshing with real merging and rotation support
struct vox_bbox
{
    int minX = 0;
    int minY = 0;
    int minZ = 0;

    int maxX = 0;
    int maxY = 0;
    int maxZ = 0;

    vox_bbox() = default;

    vox_bbox(int min_x, int min_y, int min_z, int max_x, int max_y, int max_z)
        : minX(min_x), minY(min_y), minZ(min_z), maxX(max_x), maxY(max_y), maxZ(max_z)
    {}

    void expandToFit(int x, int y, int z)
    {
        if (x < minX) minX = x;
        if (y < minY) minY = y;
        if (z < minZ) minZ = z;

        if (x > maxX) maxX = x;
        if (y > maxY) maxY = y;
        if (z > maxZ) maxZ = z;
    }

    vox_vec3 center() const
    {
        return {
            (minX + maxX) / 2.0f,
            (minY + maxY) / 2.0f,
            (minZ + maxZ) / 2.0f
        };
    }

    vox_vec3 size() const
    {
        return {
            static_cast<float>(maxX - minX + 1),
            static_cast<float>(maxY - minY + 1),
            static_cast<float>(maxZ - minZ + 1)
        };
    }
};

std::shared_ptr<vox_scene> vox_mesh_builder::build_mesh_greedy(const std::shared_ptr<vox_file> vox) {
    // Create a new Assimp scene and root node
    aiScene* scene = new aiScene();
    scene->mRootNode = new aiNode();
    scene->mRootNode->mName = "root";

    // Prepare list of model indices to process (similar to original code)
    struct model_info { int index; int nodeId; };
    std::vector<model_info> modelIndexes;
    if (!vox->shapes.empty()) {
        // Use defined shape models (nSHP) with frames
        for (auto it = vox->shapes.begin(); it != vox->shapes.end(); ++it) {
            const vox_nSHP& shape = it->second;
            for (size_t j = 0; j < shape.models.size(); ++j) {
                modelIndexes.push_back({ shape.models[j].modelID, shape.nodeID });
            }
        }
    } else {
        // Default: one model per voxel model
        modelIndexes.resize(vox->voxModels.size());
        for (int i = 0; i < vox->voxModels.size(); ++i) {
            modelIndexes[i] = { i, i };
        }
    }

    // Allocate meshes array
    scene->mNumMeshes = modelIndexes.size();
    scene->mMeshes = new aiMesh*[scene->mNumMeshes];

    // One material for all (can be extended to multiple if needed)
    scene->mMaterials = new aiMaterial*[1];
    aiMaterial* mat = new aiMaterial();
    aiString matName("default");
    mat->AddProperty(&matName, AI_MATKEY_NAME);
    scene->mMaterials[0] = mat;
    scene->mNumMaterials = 1;

    // Process each voxel model
    for (size_t mi = 0; mi < modelIndexes.size(); ++mi) {
        int modelIdx = modelIndexes[mi].index;
        vox_model& voxModel = vox->voxModels.at(modelIdx);
        const vox_size& voxSize = vox->sizes.at(modelIdx);

        // Determine transform (translation and rotation) from nTRN node if present
        vox_vec3 meshStartPos = {0, 0, 0};
        vox_imat3 rotation = vox_imat3::identity;
        if (!vox->transforms.empty()) {
            // Note: nodeId in modelIndexes is 1-based; adjust index
            int transformIndex = modelIndexes[mi].nodeId - 1;
            if (transformIndex >= 0 && transformIndex < (int)vox->transforms.size()) {
                const vox_nTRN& transform = vox->transforms[transformIndex];
                if (transform.framesCount > 0) {
                    const vox_frame_attrib& frameAttrib = transform.frameAttrib[0];
                    meshStartPos = frameAttrib.translation;
                    rotation = frameAttrib.rotation;
                }
            }
        }

        // Flip settings for coordinate system (Magicavoxel to output)
        bool flipX = true, flipY = false, flipZ = false;
        float xSign = flipX ? -1.0f : 1.0f;
        float ySign = flipY ? -1.0f : 1.0f;
        float zSign = flipZ ? -1.0f : 1.0f;
        float scale = 100.0f;  // scaling factor (from original code)

        // Compute model center (in voxel index space)
        const vox_bounding_box& bbox = voxModel.boundingBox;
        vox_vec3 model_center = {
            (bbox.maxX + bbox.minX) / 2.0f,
            (bbox.maxY + bbox.minY) / 2.0f,
            (bbox.maxZ + bbox.minZ) / 2.0f
        };

        // Precompute rotated offset (0.5,0.5,0.5) for adjusting vertex positions
        vox_vec3 offset_vec = rotation * vox_vec3{0.5f, 0.5f, 0.5f};
        // Preprocess voxels: remove fully hidden voxels to optimize (optional)
        voxModel.voxels = removeHiddenVoxels(voxSize, voxModel);

        // Prepare arrays for greedy meshing
        std::vector<aiVector3D> vertices;
        std::vector<aiFace> faces;
        vertices.reserve(voxModel.voxels.size() * 8); // Rough estimate
        faces.reserve(voxModel.voxels.size() * 6);

        // For easier neighbor checks, use a 3D grid of voxel indices
        const auto& grid = voxModel.voxel_3dGrid;  // grid[z][y][x] = color index or -1 if empty

        // Greedy merge for each face orientation
        // 1. Top faces (faces with normal +Y)
        for (int y = 0; y < voxSize.y; ++y) {
            std::vector<std::vector<bool>> visited(voxSize.x, std::vector<bool>(voxSize.z, false));
            for (int z = 0; z < voxSize.z; ++z) {
                for (int x = 0; x < voxSize.x; ++x) {
                    if (visited[x][z]) continue;
                    if (grid[z][y][x] == -1) continue;
                    bool neighborSolid = (y + 1 < voxSize.y && grid[z][y+1][x] != -1);
                    if (neighborSolid) continue;  // no top face if voxel above
                    // A top face exists at this voxel
                    int colorIndex = grid[z][y][x];
                    // Greedily expand rectangle in +X and +Z directions
                    int width = 1;
                    // Expand in X direction
                    while (x + width < voxSize.x) {
                        if (visited[x+width][z]) break;
                        if (grid[z][y][x+width] == -1) break;
                        if (y + 1 < voxSize.y && grid[z][y+1][x+width] != -1) break;
                        if (grid[z][y][x+width] != colorIndex) break;
                        ++width;
                    }
                    int height = 1;
                    // Expand in Z (for each new row, must check full width region)
                    while (z + height < voxSize.z) {
                        bool rowValid = true;
                        for (int xx = x; xx < x + width; ++xx) {
                            if (visited[xx][z + height]) { rowValid = false; break; }
                            if (grid[z+height][y][xx] == -1) { rowValid = false; break; }
                            if (y + 1 < voxSize.y && grid[z+height][y+1][xx] != -1) { rowValid = false; break; }
                            if (grid[z+height][y][xx] != colorIndex) { rowValid = false; break; }
                        }
                        if (!rowValid) break;
                        ++height;
                    }
                    // Mark all covered cells as visited
                    for (int zz = z; zz < z + height; ++zz) {
                        for (int xx = x; xx < x + width; ++xx) {
                            visited[xx][zz] = true;
                        }
                    }
                    // Compute world coordinates for the merged top face corners
                    float minX = (float)x;
                    float maxX = (float)(x + width - 1);
                    float minZ = (float)z;
                    float maxZ = (float)(z + height - 1);
                    float Yplane = (float)y + 0.5f;  // top face at voxel top
                    // Four corners of the rectangle (before rotation/transform):
                    // Note: "front" = +Z direction, "back" = -Z, "left" = -X, "right" = +X
                    vox_vec3 corner_back_left  = { minX - 0.5f, Yplane, minZ - 0.5f };
                    vox_vec3 corner_back_right = { maxX + 0.5f, Yplane, minZ - 0.5f };
                    vox_vec3 corner_front_left  = { minX - 0.5f, Yplane, maxZ + 0.5f };
                    vox_vec3 corner_front_right = { maxX + 0.5f, Yplane, maxZ + 0.5f };
                    // Rotate and transform each corner to output coordinates
                    aiVector3D vBL, vBR, vFL, vFR;
                    for (int ci = 0; ci < 4; ++ci) {
                        vox_vec3 c = (ci == 0 ? corner_back_left :
                                      ci == 1 ? corner_back_right :
                                      ci == 2 ? corner_front_left :
                                               corner_front_right);
                        // Apply rotation
                        vox_vec3 r = rotation * c;
                        // Apply axis flips, center offset, and scale (same formula as original code)
                        float outX = xSign * (r.x - model_center.x + xSign * floor(offset_vec.x));
                        float outY = ySign * (r.z - model_center.z + ySign * floor(offset_vec.z));
                        float outZ = zSign * (r.y - model_center.y + zSign * floor(offset_vec.y));
                        aiVector3D v(outX * scale, outY * scale, outZ * scale);
                        if      (ci == 0) vBL = v;
                        else if (ci == 1) vBR = v;
                        else if (ci == 2) vFL = v;
                        else if (ci == 3) vFR = v;
                    }
                    // Add vertices to list (note: each face uses its own 4 vertices)
                    unsigned vIndexBase = vertices.size();
                    vertices.push_back(vBL);
                    vertices.push_back(vBR);
                    vertices.push_back(vFL);
                    vertices.push_back(vFR);
                    // Create two triangles (indices relative to vIndexBase)
                    aiFace face1, face2;
                    face1.mNumIndices = 3;
                    face2.mNumIndices = 3;
                    face1.mIndices = new unsigned[3];
                    face2.mIndices = new unsigned[3];
                    // Triangle 1: back-left, front-left, front-right (using our corner vars: BL, FL, FR)
                    face1.mIndices[0] = vIndexBase + 0; // BL
                    face1.mIndices[1] = vIndexBase + 2; // FL
                    face1.mIndices[2] = vIndexBase + 3; // FR
                    // Triangle 2: front-right, back-right, back-left (FR, BR, BL)
                    face2.mIndices[0] = vIndexBase + 3; // FR
                    face2.mIndices[1] = vIndexBase + 1; // BR
                    face2.mIndices[2] = vIndexBase + 0; // BL
                    faces.push_back(face1);
                    faces.push_back(face2);
                }
            }
        }

        // 2. Bottom faces (normal -Y)
        for (int y = 0; y < voxSize.y; ++y) {
            std::vector<std::vector<bool>> visited(voxSize.x, std::vector<bool>(voxSize.z, false));
            for (int z = 0; z < voxSize.z; ++z) {
                for (int x = 0; x < voxSize.x; ++x) {
                    if (visited[x][z]) continue;
                    if (grid[z][y][x] == -1) continue;
                    bool neighborSolid = (y - 1 >= 0 && grid[z][y-1][x] != -1);
                    if (neighborSolid) continue;
                    int colorIndex = grid[z][y][x];
                    int width = 1;
                    while (x + width < voxSize.x) {
                        if (visited[x+width][z]) break;
                        if (grid[z][y][x+width] == -1) break;
                        if (y - 1 >= 0 && grid[z][y-1][x+width] != -1) break;
                        if (grid[z][y][x+width] != colorIndex) break;
                        ++width;
                    }
                    int height = 1;
                    while (z + height < voxSize.z) {
                        bool rowValid = true;
                        for (int xx = x; xx < x + width; ++xx) {
                            if (visited[xx][z+height]) { rowValid = false; break; }
                            if (grid[z+height][y][xx] == -1) { rowValid = false; break; }
                            if (y - 1 >= 0 && grid[z+height][y-1][xx] != -1) { rowValid = false; break; }
                            if (grid[z+height][y][xx] != colorIndex) { rowValid = false; break; }
                        }
                        if (!rowValid) break;
                        ++height;
                    }
                    for (int zz = z; zz < z + height; ++zz) {
                        for (int xx = x; xx < x + width; ++xx) {
                            visited[xx][zz] = true;
                        }
                    }
                    float minX = (float)x;
                    float maxX = (float)(x + width - 1);
                    float minZ = (float)z;
                    float maxZ = (float)(z + height - 1);
                    float Yplane = (float)y - 0.5f;
                    // Corners for bottom face (same XZ as top, Y shifted down)
                    vox_vec3 corner_back_left  = { minX - 0.5f, Yplane, minZ - 0.5f };
                    vox_vec3 corner_back_right = { maxX + 0.5f, Yplane, minZ - 0.5f };
                    vox_vec3 corner_front_left  = { minX - 0.5f, Yplane, maxZ + 0.5f };
                    vox_vec3 corner_front_right = { maxX + 0.5f, Yplane, maxZ + 0.5f };
                    // Transform corners
                    aiVector3D vBL, vBR, vFL, vFR;
                    for (int ci = 0; ci < 4; ++ci) {
                        vox_vec3 c = (ci == 0 ? corner_back_left :
                                      ci == 1 ? corner_back_right :
                                      ci == 2 ? corner_front_left :
                                               corner_front_right);
                        vox_vec3 r = rotation * c;
                        float outX = xSign * (r.x - model_center.x + xSign * floor(offset_vec.x));
                        float outY = ySign * (r.z - model_center.z + ySign * floor(offset_vec.z));
                        float outZ = zSign * (r.y - model_center.y + zSign * floor(offset_vec.y));
                        aiVector3D v(outX * scale, outY * scale, outZ * scale);
                        if      (ci == 0) vBL = v;
                        else if (ci == 1) vBR = v;
                        else if (ci == 2) vFL = v;
                        else if (ci == 3) vFR = v;
                    }
                    unsigned vIndexBase = vertices.size();
                    vertices.push_back(vBL);
                    vertices.push_back(vBR);
                    vertices.push_back(vFL);
                    vertices.push_back(vFR);
                    aiFace face1, face2;
                    face1.mNumIndices = face2.mNumIndices = 3;
                    face1.mIndices = new unsigned[3];
                    face2.mIndices = new unsigned[3];
                    // Triangles (same pattern as top face)
                    face1.mIndices[0] = vIndexBase + 0; // back-left
                    face1.mIndices[1] = vIndexBase + 2; // front-left
                    face1.mIndices[2] = vIndexBase + 3; // front-right
                    face2.mIndices[0] = vIndexBase + 3; // front-right
                    face2.mIndices[1] = vIndexBase + 1; // back-right
                    face2.mIndices[2] = vIndexBase + 0; // back-left
                    faces.push_back(face1);
                    faces.push_back(face2);
                }
            }
        }

        // 3. Front faces (normal +Z)
        for (int z = 0; z < voxSize.z; ++z) {
            std::vector<std::vector<bool>> visited(voxSize.x, std::vector<bool>(voxSize.y, false));
            for (int y = 0; y < voxSize.y; ++y) {
                for (int x = 0; x < voxSize.x; ++x) {
                    if (visited[x][y]) continue;
                    if (grid[z][y][x] == -1) continue;
                    bool neighborSolid = (z + 1 < voxSize.z && grid[z+1][y][x] != -1);
                    if (neighborSolid) continue;
                    int colorIndex = grid[z][y][x];
                    int width = 1;
                    while (x + width < voxSize.x) {
                        if (visited[x+width][y]) break;
                        if (grid[z][y][x+width] == -1) break;
                        if (z + 1 < voxSize.z && grid[z+1][y][x+width] != -1) break;
                        if (grid[z][y][x+width] != colorIndex) break;
                        ++width;
                    }
                    int height = 1;
                    while (y + height < voxSize.y) {
                        bool rowValid = true;
                        for (int xx = x; xx < x + width; ++xx) {
                            if (visited[xx][y+height]) { rowValid = false; break; }
                            if (grid[z][y+height][xx] == -1) { rowValid = false; break; }
                            if (z + 1 < voxSize.z && grid[z+1][y+height][xx] != -1) { rowValid = false; break; }
                            if (grid[z][y+height][xx] != colorIndex) { rowValid = false; break; }
                        }
                        if (!rowValid) break;
                        ++height;
                    }
                    for (int yy = y; yy < y + height; ++yy) {
                        for (int xx = x; xx < x + width; ++xx) {
                            visited[xx][yy] = true;
                        }
                    }
                    float minX = (float)x;
                    float maxX = (float)(x + width - 1);
                    float minY = (float)y;
                    float maxY = (float)(y + height - 1);
                    float Zplane = (float)z + 0.5f;
                    // Corners for front face (plane at z+0.5)
                    vox_vec3 corner_bottom_left  = { minX - 0.5f, minY - 0.5f, Zplane };
                    vox_vec3 corner_bottom_right = { maxX + 0.5f, minY - 0.5f, Zplane };
                    vox_vec3 corner_top_left     = { minX - 0.5f, maxY + 0.5f, Zplane };
                    vox_vec3 corner_top_right    = { maxX + 0.5f, maxY + 0.5f, Zplane };
                    aiVector3D vBL, vBR, vTL, vTR;
                    for (int ci = 0; ci < 4; ++ci) {
                        vox_vec3 c = (ci == 0 ? corner_bottom_left :
                                      ci == 1 ? corner_bottom_right :
                                      ci == 2 ? corner_top_left :
                                               corner_top_right);
                        vox_vec3 r = rotation * c;
                        float outX = xSign * (r.x - model_center.x + xSign * floor(offset_vec.x));
                        float outY = ySign * (r.z - model_center.z + ySign * floor(offset_vec.z));
                        float outZ = zSign * (r.y - model_center.y + zSign * floor(offset_vec.y));
                        aiVector3D v(outX * scale, outY * scale, outZ * scale);
                        if      (ci == 0) vBL = v;
                        else if (ci == 1) vBR = v;
                        else if (ci == 2) vTL = v;
                        else if (ci == 3) vTR = v;
                    }
                    unsigned vIndexBase = vertices.size();
                    vertices.push_back(vBL);
                    vertices.push_back(vBR);
                    vertices.push_back(vTL);
                    vertices.push_back(vTR);
                    aiFace face1, face2;
                    face1.mNumIndices = face2.mNumIndices = 3;
                    face1.mIndices = new unsigned[3];
                    face2.mIndices = new unsigned[3];
                    // Triangles for front face:
                    // (bottom_right, top_right, top_left) and (top_left, bottom_left, bottom_right)
                    face1.mIndices[0] = vIndexBase + 1; // bottom_right
                    face1.mIndices[1] = vIndexBase + 3; // top_right
                    face1.mIndices[2] = vIndexBase + 2; // top_left
                    face2.mIndices[0] = vIndexBase + 2; // top_left
                    face2.mIndices[1] = vIndexBase + 0; // bottom_left
                    face2.mIndices[2] = vIndexBase + 1; // bottom_right
                    faces.push_back(face1);
                    faces.push_back(face2);
                }
            }
        }

        // 4. Back faces (normal -Z)
        for (int z = 0; z < voxSize.z; ++z) {
            std::vector<std::vector<bool>> visited(voxSize.x, std::vector<bool>(voxSize.y, false));
            for (int y = 0; y < voxSize.y; ++y) {
                for (int x = 0; x < voxSize.x; ++x) {
                    if (visited[x][y]) continue;
                    if (grid[z][y][x] == -1) continue;
                    bool neighborSolid = (z - 1 >= 0 && grid[z-1][y][x] != -1);
                    if (neighborSolid) continue;
                    int colorIndex = grid[z][y][x];
                    int width = 1;
                    while (x + width < voxSize.x) {
                        if (visited[x+width][y]) break;
                        if (grid[z][y][x+width] == -1) break;
                        if (z - 1 >= 0 && grid[z-1][y][x+width] != -1) break;
                        if (grid[z][y][x+width] != colorIndex) break;
                        ++width;
                    }
                    int height = 1;
                    while (y + height < voxSize.y) {
                        bool rowValid = true;
                        for (int xx = x; xx < x + width; ++xx) {
                            if (visited[xx][y+height]) { rowValid = false; break; }
                            if (grid[z][y+height][xx] == -1) { rowValid = false; break; }
                            if (z - 1 >= 0 && grid[z-1][y+height][xx] != -1) { rowValid = false; break; }
                            if (grid[z][y+height][xx] != colorIndex) { rowValid = false; break; }
                        }
                        if (!rowValid) break;
                        ++height;
                    }
                    for (int yy = y; yy < y + height; ++yy) {
                        for (int xx = x; xx < x + width; ++xx) {
                            visited[xx][yy] = true;
                        }
                    }
                    float minX = (float)x;
                    float maxX = (float)(x + width - 1);
                    float minY = (float)y;
                    float maxY = (float)(y + height - 1);
                    float Zplane = (float)z - 0.5f;
                    vox_vec3 corner_bottom_left  = { minX - 0.5f, minY - 0.5f, Zplane };
                    vox_vec3 corner_bottom_right = { maxX + 0.5f, minY - 0.5f, Zplane };
                    vox_vec3 corner_top_left     = { minX - 0.5f, maxY + 0.5f, Zplane };
                    vox_vec3 corner_top_right    = { maxX + 0.5f, maxY + 0.5f, Zplane };
                    aiVector3D vBL, vBR, vTL, vTR;
                    for (int ci = 0; ci < 4; ++ci) {
                        vox_vec3 c = (ci == 0 ? corner_bottom_left :
                                      ci == 1 ? corner_bottom_right :
                                      ci == 2 ? corner_top_left :
                                               corner_top_right);
                        vox_vec3 r = rotation * c;
                        float outX = xSign * (r.x - model_center.x + xSign * floor(offset_vec.x));
                        float outY = ySign * (r.z - model_center.z + ySign * floor(offset_vec.z));
                        float outZ = zSign * (r.y - model_center.y + zSign * floor(offset_vec.y));
                        aiVector3D v(outX * scale, outY * scale, outZ * scale);
                        if      (ci == 0) vBL = v;
                        else if (ci == 1) vBR = v;
                        else if (ci == 2) vTL = v;
                        else if (ci == 3) vTR = v;
                    }
                    unsigned vIndexBase = vertices.size();
                    vertices.push_back(vBL);
                    vertices.push_back(vBR);
                    vertices.push_back(vTL);
                    vertices.push_back(vTR);
                    aiFace face1, face2;
                    face1.mNumIndices = face2.mNumIndices = 3;
                    face1.mIndices = new unsigned[3];
                    face2.mIndices = new unsigned[3];
                    // Triangles for back face:
                    // (bottom_left, top_left, top_right) and (top_right, bottom_right, bottom_left)
                    face1.mIndices[0] = vIndexBase + 0; // bottom_left
                    face1.mIndices[1] = vIndexBase + 2; // top_left
                    face1.mIndices[2] = vIndexBase + 3; // top_right
                    face2.mIndices[0] = vIndexBase + 3; // top_right
                    face2.mIndices[1] = vIndexBase + 1; // bottom_right
                    face2.mIndices[2] = vIndexBase + 0; // bottom_left
                    faces.push_back(face1);
                    faces.push_back(face2);
                }
            }
        }

        // 5. Right faces (normal +X)
        for (int x = 0; x < voxSize.x; ++x) {
            std::vector<std::vector<bool>> visited(voxSize.z, std::vector<bool>(voxSize.y, false));
            for (int y = 0; y < voxSize.y; ++y) {
                for (int z = 0; z < voxSize.z; ++z) {
                    if (visited[z][y]) continue;
                    if (grid[z][y][x] == -1) continue;
                    bool neighborSolid = (x + 1 < voxSize.x && grid[z][y][x+1] != -1);
                    if (neighborSolid) continue;
                    int colorIndex = grid[z][y][x];
                    int width = 1;
                    while (z + width < voxSize.z) {
                        if (visited[z+width][y]) break;
                        if (grid[z+width][y][x] == -1) break;
                        if (x + 1 < voxSize.x && grid[z+width][y][x+1] != -1) break;
                        if (grid[z+width][y][x] != colorIndex) break;
                        ++width;
                    }
                    int height = 1;
                    while (y + height < voxSize.y) {
                        bool rowValid = true;
                        for (int zz = z; zz < z + width; ++zz) {
                            if (visited[zz][y+height]) { rowValid = false; break; }
                            if (grid[zz][y+height][x] == -1) { rowValid = false; break; }
                            if (x + 1 < voxSize.x && grid[zz][y+height][x+1] != -1) { rowValid = false; break; }
                            if (grid[zz][y+height][x] != colorIndex) { rowValid = false; break; }
                        }
                        if (!rowValid) break;
                        ++height;
                    }
                    for (int yy = y; yy < y + height; ++yy) {
                        for (int zz = z; zz < z + width; ++zz) {
                            visited[zz][yy] = true;
                        }
                    }
                    float minZ = (float)z;
                    float maxZ = (float)(z + width - 1);
                    float minY = (float)y;
                    float maxY = (float)(y + height - 1);
                    float Xplane = (float)x + 0.5f;
                    // Corners for right face (plane at x+0.5)
                    vox_vec3 corner_bottom_back  = { Xplane, minY - 0.5f, minZ - 0.5f };
                    vox_vec3 corner_bottom_front = { Xplane, minY - 0.5f, maxZ + 0.5f };
                    vox_vec3 corner_top_back     = { Xplane, maxY + 0.5f, minZ - 0.5f };
                    vox_vec3 corner_top_front    = { Xplane, maxY + 0.5f, maxZ + 0.5f };
                    aiVector3D vBB, vBF, vTB, vTF;
                    for (int ci = 0; ci < 4; ++ci) {
                        vox_vec3 c = (ci == 0 ? corner_bottom_back :
                                      ci == 1 ? corner_bottom_front :
                                      ci == 2 ? corner_top_back :
                                               corner_top_front);
                        vox_vec3 r = rotation * c;
                        float outX = xSign * (r.x - model_center.x + xSign * floor(offset_vec.x));
                        float outY = ySign * (r.z - model_center.z + ySign * floor(offset_vec.z));
                        float outZ = zSign * (r.y - model_center.y + zSign * floor(offset_vec.y));
                        aiVector3D v(outX * scale, outY * scale, outZ * scale);
                        if      (ci == 0) vBB = v;
                        else if (ci == 1) vBF = v;
                        else if (ci == 2) vTB = v;
                        else if (ci == 3) vTF = v;
                    }
                    unsigned vIndexBase = vertices.size();
                    vertices.push_back(vBB);
                    vertices.push_back(vBF);
                    vertices.push_back(vTB);
                    vertices.push_back(vTF);
                    aiFace face1, face2;
                    face1.mNumIndices = face2.mNumIndices = 3;
                    face1.mIndices = new unsigned[3];
                    face2.mIndices = new unsigned[3];
                    // Triangles for right face:
                    // (bottom_back, top_back, top_front) and (top_front, bottom_front, bottom_back)
                    face1.mIndices[0] = vIndexBase + 0; // bottom_back
                    face1.mIndices[1] = vIndexBase + 2; // top_back
                    face1.mIndices[2] = vIndexBase + 3; // top_front
                    face2.mIndices[0] = vIndexBase + 3; // top_front
                    face2.mIndices[1] = vIndexBase + 1; // bottom_front
                    face2.mIndices[2] = vIndexBase + 0; // bottom_back
                    faces.push_back(face1);
                    faces.push_back(face2);
                }
            }
        }

        // 6. Left faces (normal -X)
        for (int x = 0; x < voxSize.x; ++x) {
            std::vector<std::vector<bool>> visited(voxSize.z, std::vector<bool>(voxSize.y, false));
            for (int y = 0; y < voxSize.y; ++y) {
                for (int z = 0; z < voxSize.z; ++z) {
                    if (visited[z][y]) continue;
                    if (grid[z][y][x] == -1) continue;
                    bool neighborSolid = (x - 1 >= 0 && grid[z][y][x-1] != -1);
                    if (neighborSolid) continue;
                    int colorIndex = grid[z][y][x];
                    int width = 1;
                    while (z + width < voxSize.z) {
                        if (visited[z+width][y]) break;
                        if (grid[z+width][y][x] == -1) break;
                        if (x - 1 >= 0 && grid[z+width][y][x-1] != -1) break;
                        if (grid[z+width][y][x] != colorIndex) break;
                        ++width;
                    }
                    int height = 1;
                    while (y + height < voxSize.y) {
                        bool rowValid = true;
                        for (int zz = z; zz < z + width; ++zz) {
                            if (visited[zz][y+height]) { rowValid = false; break; }
                            if (grid[zz][y+height][x] == -1) { rowValid = false; break; }
                            if (x - 1 >= 0 && grid[zz][y+height][x-1] != -1) { rowValid = false; break; }
                            if (grid[zz][y+height][x] != colorIndex) { rowValid = false; break; }
                        }
                        if (!rowValid) break;
                        ++height;
                    }
                    for (int yy = y; yy < y + height; ++yy) {
                        for (int zz = z; zz < z + width; ++zz) {
                            visited[zz][yy] = true;
                        }
                    }
                    float minZ = (float)z;
                    float maxZ = (float)(z + width - 1);
                    float minY = (float)y;
                    float maxY = (float)(y + height - 1);
                    float Xplane = (float)x - 0.5f;
                    // Corners for left face (plane at x-0.5)
                    vox_vec3 corner_bottom_back  = { Xplane, minY - 0.5f, minZ - 0.5f };
                    vox_vec3 corner_bottom_front = { Xplane, minY - 0.5f, maxZ + 0.5f };
                    vox_vec3 corner_top_back     = { Xplane, maxY + 0.5f, minZ - 0.5f };
                    vox_vec3 corner_top_front    = { Xplane, maxY + 0.5f, maxZ + 0.5f };
                    aiVector3D vBB, vBF, vTB, vTF;
                    for (int ci = 0; ci < 4; ++ci) {
                        vox_vec3 c = (ci == 0 ? corner_bottom_back :
                                      ci == 1 ? corner_bottom_front :
                                      ci == 2 ? corner_top_back :
                                               corner_top_front);
                        vox_vec3 r = rotation * c;
                        float outX = xSign * (r.x - model_center.x + xSign * floor(offset_vec.x));
                        float outY = ySign * (r.z - model_center.z + ySign * floor(offset_vec.z));
                        float outZ = zSign * (r.y - model_center.y + zSign * floor(offset_vec.y));
                        aiVector3D v(outX * scale, outY * scale, outZ * scale);
                        if      (ci == 0) vBB = v;
                        else if (ci == 1) vBF = v;
                        else if (ci == 2) vTB = v;
                        else if (ci == 3) vTF = v;
                    }
                    unsigned vIndexBase = vertices.size();
                    vertices.push_back(vBB);
                    vertices.push_back(vBF);
                    vertices.push_back(vTB);
                    vertices.push_back(vTF);
                    aiFace face1, face2;
                    face1.mNumIndices = face2.mNumIndices = 3;
                    face1.mIndices = new unsigned[3];
                    face2.mIndices = new unsigned[3];
                    // Triangles for left face:
                    // (bottom_back, top_back, top_front) and (top_front, bottom_front, bottom_back)
                    face1.mIndices[0] = vIndexBase + 0; // bottom_back
                    face1.mIndices[1] = vIndexBase + 2; // top_back
                    face1.mIndices[2] = vIndexBase + 3; // top_front
                    face2.mIndices[0] = vIndexBase + 3; // top_front
                    face2.mIndices[1] = vIndexBase + 1; // bottom_front
                    face2.mIndices[2] = vIndexBase + 0; // bottom_back
                    faces.push_back(face1);
                    faces.push_back(face2);
                }
            }
        }

        // Assemble the aiMesh for this model
        aiMesh* mesh = new aiMesh();
        mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
        mesh->mNumVertices = vertices.size();
        mesh->mVertices = new aiVector3D[mesh->mNumVertices];
        for (size_t v = 0; v < vertices.size(); ++v) {
            mesh->mVertices[v] = vertices[v];
        }
        mesh->mNumFaces = faces.size();
        mesh->mFaces = new aiFace[mesh->mNumFaces];
        for (size_t f = 0; f < faces.size(); ++f) {
            mesh->mFaces[f] = faces[f];
        }
        mesh->mMaterialIndex = 0;
        mesh->mNormals = nullptr; // Normals can be computed later if needed
        scene->mMeshes[mi] = mesh;

        // Create a child node for this mesh and apply translation
        aiNode* child = new aiNode();
        child->mName = "vox" + std::to_string(mi);
        child->mParent = scene->mRootNode;
        child->mMeshes = new unsigned int[1]{ (unsigned)mi };
        child->mNumMeshes = 1;
        // Set node transformation (translation only, rotation already applied to vertices)
        aiVector3D translation(
            xSign * (meshStartPos.x - vox_math::frac(model_center.x)) * scale,
            ySign * (meshStartPos.z - vox_math::frac(model_center.z)) * scale,
            zSign * (meshStartPos.y - vox_math::frac(model_center.y)) * scale
        );
        aiMatrix4x4::Translation(translation, child->mTransformation);
        scene->mRootNode->addChildren(1, &child);
    }

    // Export the scene (for example, to FBX) – identical to original usage
    Assimp::Exporter exporter;
    if (exporter.Export(scene, "fbx", "scenes_optimized.fbx") != aiReturn_SUCCESS) {
        std::cerr << "Error exporting scene: " << exporter.GetErrorString() << std::endl;
    }
    DestroyScene(scene);
    return {};
}

// assimp Export formats
/*collada
x
stp
obj
objnomtl
stl
stlb
ply
plyb
3ds
gltf2
glb2
gltf
glb
assbin
assxml
x3d
fbx
fbxa
m3d
m3da
3mf
pbrt
assjson*/
