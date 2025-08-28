#include "VoxUtils.h"

std::shared_ptr<VoxObject> CreateVoxObject(const std::vector<std::shared_ptr<Unvoxeller::UnvoxScene>>& scenes)
{
	std::vector<std::shared_ptr<RenderableObject>> renderables{};
	for (const auto& scene : scenes)
	{
		s32 meshesIdx = {};
		for (const auto& mesh : scene->Meshes)
		{
			std::unique_ptr<MeshDescriptor> mDesc = std::make_unique<MeshDescriptor>();
			mDesc->RenderType = MeshRenderType::TRIANGLES;
	
			auto renderable = std::make_shared<RenderableObject>();

			mDesc->Vertices.resize(mesh->Vertices.size());

			for (size_t i = 0; i < mesh->Vertices.size(); i++)
			{
				const auto& vert = mesh->Vertices[i];
				const auto& normal = mesh->Normals[i];
				const auto& uv = mesh->UVs[i];

				mDesc->Vertices[i] = { {vert.x, vert.y, vert.z}, {normal.x, normal.y, normal.z}, {uv.x, uv.y }};
			}
			
			meshesIdx++;

			mDesc->Indices.resize(mesh->Faces.size() * 3);
			s32 idx = {};
			for (size_t i = 0; i < mesh->Faces.size(); i++)
			{
				const auto& face = mesh->Faces[i];

				for (size_t j = 0; j < face.Indices.size(); j++)
				{
					mDesc->Indices[idx] = face.Indices[j];
					idx++;
				}
			}

			auto tex = scene->Textures[scene->Materials[mesh->MaterialIndex]->TextureIndex];
			TextureDescriptor tDesc{};
			tDesc.width = tex->Width;
			tDesc.height= tex->Height;
			tDesc.image = tex->Buffer.data();
			renderable->SetTexture(Texture::Create(&tDesc));

			renderable->SetMesh(Mesh::CreateMesh(mDesc.get()));
			renderable->SetRenderType(PipelineRenderType::Opaque_Unlit);
			renderable->SetDrawType(RenderDrawType::Triangles);

			renderables.push_back(renderable);
		}
	}

	return std::make_shared<VoxObject>(renderables);
}
