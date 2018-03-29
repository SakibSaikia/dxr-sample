#include "stdafx.h"
#include "Scene.h"
#include "View.h"

void Scene::LoadMeshes(const aiScene* loader, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	for (auto meshIdx = 0u; meshIdx < loader->mNumMeshes; meshIdx++)
	{
		const aiMesh* srcMesh = loader->mMeshes[meshIdx];

		// vertex data
		std::vector<StaticMesh::VertexType> vertexData;
		for (auto vertIdx = 0u; vertIdx < srcMesh->mNumVertices; vertIdx++)
		{
			const aiVector3D& vertPos = srcMesh->mVertices[vertIdx];
			const aiVector3D& vertNormal = srcMesh->mNormals[vertIdx];
			const aiVector3D& vertUV = srcMesh->mTextureCoords[0][vertIdx];

			vertexData.emplace_back(
				DirectX::XMFLOAT3(vertPos.x, vertPos.y, vertPos.z),
				DirectX::XMFLOAT3(vertNormal.x, vertNormal.y, vertNormal.z),
				DirectX::XMFLOAT2(vertUV.x, vertUV.y)
			);
		}

		// index data
		std::vector<StaticMesh::IndexType> indexData;
		for (auto primIdx = 0u; primIdx<srcMesh->mNumFaces; primIdx++)
		{
			const aiFace& primitive = srcMesh->mFaces[primIdx];
			for (auto index = 0u; index < primitive.mNumIndices; index++)
			{
				indexData.push_back(primitive.mIndices[index]);
			}
		}

		auto mesh = std::make_unique<StaticMesh>();
		mesh->Init(device, cmdList, std::move(vertexData), std::move(indexData), srcMesh->mMaterialIndex);
		m_meshes.push_back(std::move(mesh));
	}
}

void Scene::LoadMaterials(
	const aiScene* loader, 
	ID3D12Device* device, 
	ID3D12CommandQueue* cmdQueue,
	ID3D12DescriptorHeap* srvHeap, 
	const size_t srvStartOffset, 
	const size_t srvDescriptorSize)
{
	// Used to batch texture uploads
	DirectX::ResourceUploadBatch resourceUpload(device);
	resourceUpload.Begin();

	auto descriptorIdx = 0;

	for (auto matIdx = 0u; matIdx < loader->mNumMaterials; matIdx++)
	{
		const aiMaterial* srcMat = loader->mMaterials[matIdx];

		aiString materialName;
		srcMat->Get(AI_MATKEY_NAME, materialName);

		aiString diffuseTextureNameStr;
		aiReturn bHasDiffuseTexture = srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), diffuseTextureNameStr);

		aiString opacityMaskTextureNameStr;
		aiReturn bHasOpacityMaskTexture = srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_OPACITY, 0), opacityMaskTextureNameStr);

		if (bHasDiffuseTexture == aiReturn_SUCCESS && bHasOpacityMaskTexture == aiReturn_SUCCESS)
		{
			std::string diffuseTextureName(diffuseTextureNameStr.C_Str());
			Texture* diffuseTexture;
			auto texIter = std::find_if(m_textures.cbegin(), m_textures.cend(),
				[&diffuseTextureName](const std::unique_ptr<Texture>& tex) { return tex->GetName() == diffuseTextureName; });

			if(texIter == m_textures.cend())
			{
				auto newTexture = std::make_unique<Texture>();
				newTexture->Init(device, resourceUpload, diffuseTextureName);
				diffuseTexture = newTexture.get();
				m_textures.push_back(std::move(newTexture));
			}
			else
			{
				diffuseTexture = texIter->get();
			}

			const auto diffuseTextureGPUDescriptorHandle = diffuseTexture->CreateDescriptor(device, srvHeap, srvStartOffset + descriptorIdx++, srvDescriptorSize);

			std::string opacityMaskTextureName(opacityMaskTextureNameStr.C_Str());
			Texture* opacityMaskTexture;
			texIter = std::find_if(m_textures.cbegin(), m_textures.cend(),
				[&opacityMaskTextureName](const std::unique_ptr<Texture>& tex) { return tex->GetName() == opacityMaskTextureName; });

			if (texIter == m_textures.cend())
			{
				auto newTexture = std::make_unique<Texture>();
				newTexture->Init(device, resourceUpload, opacityMaskTextureName);
				opacityMaskTexture = newTexture.get();
				m_textures.push_back(std::move(newTexture));
			}
			else
			{
				opacityMaskTexture = texIter->get();
			}

			const auto opacityMaskTextureGPUDescriptorHandle = opacityMaskTexture->CreateDescriptor(device, srvHeap, srvStartOffset + descriptorIdx++, srvDescriptorSize);

			m_materials.push_back(std::make_unique<DiffuseOnlyMaskedMaterial>(std::string(materialName.C_Str()),diffuseTextureGPUDescriptorHandle));
		}
		else if (bHasDiffuseTexture == aiReturn_SUCCESS)
		{
			std::string diffuseTextureName(diffuseTextureNameStr.C_Str());

			Texture* diffuseTexture;
			auto texIter = std::find_if(m_textures.cbegin(), m_textures.cend(),
				[&diffuseTextureName](const std::unique_ptr<Texture>& tex) { return tex->GetName() == diffuseTextureName; });

			if (texIter == m_textures.cend())
			{
				auto newTexture = std::make_unique<Texture>();
				newTexture->Init(device, resourceUpload, diffuseTextureName);
				diffuseTexture = newTexture.get();
				m_textures.push_back(std::move(newTexture));
			}
			else
			{
				diffuseTexture = texIter->get();
			}

			const auto diffuseTextureGPUDescriptorHandle = diffuseTexture->CreateDescriptor(device, srvHeap, srvStartOffset + descriptorIdx++, srvDescriptorSize);

			m_materials.push_back(std::make_unique<DiffuseOnlyOpaqueMaterial>(std::string(materialName.C_Str()), diffuseTextureGPUDescriptorHandle));
		}
		else
		{
			// Invalid material
			m_materials.push_back(std::make_unique<DiffuseOnlyOpaqueMaterial>());
		}
	}

	// Upload the resources to the GPU.
	auto uploadResourcesFinished = resourceUpload.End(cmdQueue);

	// Wait for the upload thread to terminate
	uploadResourcesFinished.wait();
}

void Scene::LoadEntities(const aiNode* node)
{
	const aiMatrix4x4& parentTransform = node->mTransformation;

	for (auto childIdx = 0u; childIdx < node->mNumChildren; childIdx++)
	{
		const aiNode* childNode = node->mChildren[childIdx];

		if (childNode->mChildren == nullptr)
		{
			const aiMatrix4x4& localTransform = childNode->mTransformation;
			aiMatrix4x4 localToWorldTransform = parentTransform * localTransform;
			DirectX::XMFLOAT4X4 localToWorld(reinterpret_cast<float*>(&localToWorldTransform));

			for (auto meshIdx = 0u; meshIdx < childNode->mNumMeshes; meshIdx++)
			{
				m_meshEntities.emplace_back(childNode->mMeshes[meshIdx], localToWorld);
			}
		}
		else
		{
			LoadEntities(childNode);
		}
	}
}

void Scene::InitResources(
	ID3D12Device* device, 
	ID3D12CommandQueue* cmdQueue, 
	ID3D12GraphicsCommandList* cmdList, 
	ID3D12DescriptorHeap* srvHeap, 
	const size_t srvStartOffset,
	const size_t srvDescriptorSize,
	const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc)
{
	// Load scene
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(R"(..\Content\Sponza\obj\sponza.obj)",
			aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType |
			aiProcess_MakeLeftHanded |
			aiProcess_FlipWindingOrder |
			aiProcess_FlipUVs
		);

		assert(scene != nullptr && L"Failed to load scene");

		LoadMeshes(scene, device, cmdList);
		LoadMaterials(scene, device, cmdQueue, srvHeap, srvStartOffset, srvDescriptorSize);
		LoadEntities(scene->mRootNode);
	}

	// Object Constant Buffer
	{
		uint32_t objectConstantsSize = sizeof(ObjectConstants);

		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = objectConstantsSize * m_meshEntities.size();
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc.Count = 1;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		D3D12_HEAP_PROPERTIES heapDesc = {};
		heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD; // must be CPU accessible
		heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; // GPU or system mem

		for (auto n = 0; n < k_gfxBufferCount; ++n)
		{
			DX_VERIFY(device->CreateCommittedResource(
				&heapDesc,
				D3D12_HEAP_FLAG_NONE,
				&resDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(m_objectConstantBuffers.at(n).GetAddressOf())
			));

			// Get ptr to mapped resource
			auto** ptr = reinterpret_cast<void**>(&m_objectConstantBufferPtr.at(n));
			m_objectConstantBuffers.at(n)->Map(0, nullptr, ptr);
		}
	}
}

void Scene::Update(uint32_t bufferIndex)
{
	ObjectConstants* c = m_objectConstantBufferPtr.at(bufferIndex);
	for (const auto& meshEntity : m_meshEntities)
	{
		c->localToWorldMatrix = meshEntity.GetLocalToWorldMatrix();
		c++;
	}
}

void Scene::Render(ID3D12GraphicsCommandList* cmdList, uint32_t bufferIndex, const View& view)
{
	PIXScopedEvent(cmdList, 0, L"render_scene");

	int entityId = 0;
	for (auto& meshEntity : m_meshEntities)
	{
		StaticMesh* sm = m_meshes.at(meshEntity.GetMeshIndex()).get();
		Material* mat = m_materials.at(sm->GetMaterialIndex()).get();

		if (mat->IsValid())
		{
			// Root signature set by the material
			mat->BindPipeline(cmdList);

			cmdList->SetGraphicsRootConstantBufferView(0, view.GetConstantBuffer(bufferIndex)->GetGPUVirtualAddress());

			cmdList->SetGraphicsRootConstantBufferView(
				StaticMeshEntity::GetObjectConstantsRootParamIndex(),
				m_objectConstantBuffers.at(bufferIndex)->GetGPUVirtualAddress() +
				entityId * sizeof(ObjectConstants)
			);

			mat->BindConstants(cmdList);

			sm->Render(cmdList);
		}

		++entityId;
	}
}

const size_t Scene::GetNumTextures() const
{
	return m_textures.size();
}