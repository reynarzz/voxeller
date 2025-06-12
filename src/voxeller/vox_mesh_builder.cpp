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
            (vox_model.boundingBox.maxX + vox_model.boundingBox.minX) / 2.0,
            (vox_model.boundingBox.maxY + vox_model.boundingBox.minY) / 2.0,
            (vox_model.boundingBox.maxZ + vox_model.boundingBox.minZ) / 2.0
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

std::shared_ptr<vox_scene> vox_mesh_builder::build_mesh_greedy(const std::shared_ptr<vox_file> vox)
{
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