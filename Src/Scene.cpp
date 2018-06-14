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
			const aiVector3D& vertTangent = srcMesh->mTangents[vertIdx];
			const aiVector3D& vertBitangent = srcMesh->mBitangents[vertIdx];
			const aiVector3D& vertUV = srcMesh->mTextureCoords[0][vertIdx];

			vertexData.emplace_back(
				DirectX::XMFLOAT3(vertPos.x, vertPos.y, vertPos.z),
				DirectX::XMFLOAT3(vertNormal.x, vertNormal.y, vertNormal.z),
				DirectX::XMFLOAT3(vertTangent.x, vertTangent.y, vertTangent.z),
				DirectX::XMFLOAT3(vertBitangent.x, vertBitangent.y, vertBitangent.z),
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

D3D12_GPU_DESCRIPTOR_HANDLE Scene::LoadTexture(const std::string& textureName, ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, const size_t srvOffset, const size_t srvDescriptorSize, DirectX::ResourceUploadBatch& resourceUpload)
{
	Texture* tex;
	auto texIter = std::find_if(m_textures.cbegin(), m_textures.cend(),
		[&textureName](const std::unique_ptr<Texture>& tex) { return tex->GetName() == textureName; });

	if (texIter == m_textures.cend())
	{
		auto newTexture = std::make_unique<Texture>();
		newTexture->Init(device, resourceUpload, textureName);
		tex = newTexture.get();
		m_textures.push_back(std::move(newTexture));
	}
	else
	{
		tex = texIter->get();
	}

	const auto gpuDescriptorHandle = tex->CreateDescriptor(device, srvHeap, srvOffset, srvDescriptorSize);

	return gpuDescriptorHandle;
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

		aiString baseColorTextureNameStr;
		aiReturn bHasBaseColorTexture = srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), baseColorTextureNameStr);

		// roughness mapped to gloss 
		aiString roughnessTextureNameStr;
		aiReturn bHasRoughnessTexture = srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_SHININESS, 0), roughnessTextureNameStr);

		// metallic mapped to ambient
		aiString metallicTextureNameStr;
		aiReturn bHasMetallicTexture = srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_AMBIENT, 0), metallicTextureNameStr);

		aiString normalmapTextureNameStr;
		aiReturn bHasNormalmapTexture = srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_HEIGHT, 0), normalmapTextureNameStr);

		aiString opacityMaskTextureNameStr;
		aiReturn bHasOpacityMaskTexture = srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_OPACITY, 0), opacityMaskTextureNameStr);

		if (bHasBaseColorTexture == aiReturn_SUCCESS &&
			bHasRoughnessTexture == aiReturn_SUCCESS &&
			bHasMetallicTexture == aiReturn_SUCCESS &&
			bHasNormalmapTexture == aiReturn_SUCCESS && 
			bHasOpacityMaskTexture == aiReturn_SUCCESS)
		{
			const auto headDescriptor = LoadTexture(baseColorTextureNameStr.C_Str(), device, srvHeap, srvStartOffset + descriptorIdx++, srvDescriptorSize, resourceUpload);
			LoadTexture(roughnessTextureNameStr.C_Str(), device, srvHeap, srvStartOffset + descriptorIdx++, srvDescriptorSize, resourceUpload);
			LoadTexture(metallicTextureNameStr.C_Str(), device, srvHeap, srvStartOffset + descriptorIdx++, srvDescriptorSize, resourceUpload);
			LoadTexture(normalmapTextureNameStr.C_Str(), device, srvHeap, srvStartOffset + descriptorIdx++, srvDescriptorSize, resourceUpload);
			LoadTexture(opacityMaskTextureNameStr.C_Str(), device, srvHeap, srvStartOffset + descriptorIdx++, srvDescriptorSize, resourceUpload);
			m_materials.push_back(std::make_unique<DefaultMaskedMaterial>(std::string(materialName.C_Str()), headDescriptor));
		}
		else if (bHasBaseColorTexture == aiReturn_SUCCESS &&
				bHasRoughnessTexture == aiReturn_SUCCESS &&
				bHasMetallicTexture == aiReturn_SUCCESS &&
				bHasNormalmapTexture == aiReturn_SUCCESS)
		{
			const auto headDescriptor = LoadTexture(baseColorTextureNameStr.C_Str(), device, srvHeap, srvStartOffset + descriptorIdx++, srvDescriptorSize, resourceUpload);
			LoadTexture(roughnessTextureNameStr.C_Str(), device, srvHeap, srvStartOffset + descriptorIdx++, srvDescriptorSize, resourceUpload);
			LoadTexture(metallicTextureNameStr.C_Str(), device, srvHeap, srvStartOffset + descriptorIdx++, srvDescriptorSize, resourceUpload);
			LoadTexture(normalmapTextureNameStr.C_Str(), device, srvHeap, srvStartOffset + descriptorIdx++, srvDescriptorSize, resourceUpload);
			m_materials.push_back(std::make_unique<DefaultOpaqueMaterial>(std::string(materialName.C_Str()), headDescriptor));
		}
		else
		{
			// Invalid material
			m_materials.push_back(std::make_unique<NullMaterial>());
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
				m_meshEntities.push_back(std::make_unique<StaticMeshEntity>(
					std::string(childNode->mName.C_Str()), 
					childNode->mMeshes[meshIdx], 
					localToWorld));
			}
		}
		else
		{
			LoadEntities(childNode);
		}
	}

	// sort entities by pipeline state
	std::sort(m_meshEntities.begin(), m_meshEntities.end(),
		[this](auto& entity1, auto& entity2)
		{
			const auto& sm1 = m_meshes.at(entity1->GetMeshIndex());
			const auto& sm2 = m_meshes.at(entity2->GetMeshIndex());

			const auto& mat1 = m_materials.at(sm1->GetMaterialIndex());
			const auto& mat2 = m_materials.at(sm2->GetMaterialIndex());

			return mat1->GetHash(RenderPass::Geometry, VertexFormat::Type::P3N3T3B3U2) < mat2->GetHash(RenderPass::Geometry, VertexFormat::Type::P3N3T3B3U2);
		}
	);
}

void Scene::InitLights(ID3D12Device* device)
{
	m_light = std::make_unique<Light>(DirectX::XMFLOAT3{ 1.f, 1.f, 0.f }, DirectX::XMFLOAT3{ 1.f, 1.f, 1.f }, 10000.f);
}

void Scene::InitBounds(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	// world space mesh bounds
	m_meshWorldBounds.resize(m_meshEntities.size());

	int i = 0;
	for (const auto& meshEntity : m_meshEntities)
	{
		DirectX::XMMATRIX worldTransform = DirectX::XMLoadFloat4x4(&meshEntity->GetLocalToWorldMatrix());

		const DirectX::BoundingBox& objectBounds = m_meshes.at(meshEntity->GetMeshIndex())->GetBounds();
		DirectX::BoundingBox& outWorldBounds = m_meshWorldBounds.at(i++);
		objectBounds.Transform(outWorldBounds, worldTransform);
	}

	// world space scene bounds
	m_sceneBounds = m_meshWorldBounds.at(0);
	for (const auto& meshWorldBounds : m_meshWorldBounds)
	{
		DirectX::BoundingBox::CreateMerged(m_sceneBounds, m_sceneBounds, meshWorldBounds);
	}

#if 0
	// debug rendering
	auto debugMesh = std::make_unique<DebugLineMesh>();
	debugMesh->Init(device, cmdList, m_meshWorldBounds, DirectX::XMFLOAT3{ 1.0, 0.0, 0.0 });
	m_debugMeshes.push_back(std::move(debugMesh));

	debugMesh = std::make_unique<DebugLineMesh>();
	debugMesh->Init(device, cmdList, { m_sceneBounds }, DirectX::XMFLOAT3{ 0.f, 1.f, 0.f });
	m_debugMeshes.push_back(std::move(debugMesh));
#endif
}

void Scene::InitResources(
	ID3D12Device* device, 
	ID3D12CommandQueue* cmdQueue, 
	ID3D12GraphicsCommandList* cmdList, 
	ID3D12DescriptorHeap* srvHeap, 
	const size_t srvStartOffset,
	const size_t srvDescriptorSize)
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
		InitLights(device);
		InitBounds(device, cmdList);
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
			CHECK(device->CreateCommittedResource(
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

	// Light Constant Buffer
	{
		uint32_t lightConstantsSize = sizeof(ObjectConstants);

		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = lightConstantsSize;
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
			CHECK(device->CreateCommittedResource(
				&heapDesc,
				D3D12_HEAP_FLAG_NONE,
				&resDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(m_lightConstantBuffers.at(n).GetAddressOf())
			));

			// Get ptr to mapped resource
			auto** ptr = reinterpret_cast<void**>(&m_lightConstantBufferPtr.at(n));
			m_lightConstantBuffers.at(n)->Map(0, nullptr, ptr);
		}
	}
}

void Scene::Update(uint32_t bufferIndex)
{
	// mesh entities
	ObjectConstants* o = m_objectConstantBufferPtr.at(bufferIndex);
	for (const auto& meshEntity : m_meshEntities)
	{
		meshEntity->Fill(o++);
	}

	// light(s)
	LightConstants* l = m_lightConstantBufferPtr.at(bufferIndex);
	m_light->Fill(l);
}

void Scene::Render(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, uint32_t bufferIndex, const View& view)
{
	PIXScopedEvent(cmdList, 0, L"render_scene");

	{
		PIXScopedEvent(cmdList, 0, L"scene_geo");
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		int entityId = 0;
		size_t currentMaterialHash = 0;
		for (const auto& meshEntity : m_meshEntities)
		{
			StaticMesh* sm = m_meshes.at(meshEntity->GetMeshIndex()).get();
			Material* mat = m_materials.at(sm->GetMaterialIndex()).get();
			auto hash = mat->GetHash(RenderPass::Geometry, sm->GetVertexFormat());

			if (hash != currentMaterialHash)
			{
				PIXScopedEvent(cmdList, 0, L"bind_pipeline");

				// root sig
				mat->BindPipeline(device, cmdList, RenderPass::Geometry, sm->GetVertexFormat());

				// constants
				cmdList->SetGraphicsRootConstantBufferView(0, view.GetConstantBuffer(bufferIndex)->GetGPUVirtualAddress());
				cmdList->SetGraphicsRootConstantBufferView(2, m_lightConstantBuffers.at(bufferIndex)->GetGPUVirtualAddress());

				currentMaterialHash = hash;
			}

			if (mat->IsValid())
			{
				PIXScopedEvent(cmdList, 0, meshEntity->GetName().c_str());

				D3D12_GPU_VIRTUAL_ADDRESS objectConstantsDescriptor = m_objectConstantBuffers.at(bufferIndex)->GetGPUVirtualAddress() + entityId * sizeof(ObjectConstants);
				mat->BindConstants(cmdList, objectConstantsDescriptor);
				sm->Render(cmdList);
			}

			++entityId;
		}
	}

	{
		PIXScopedEvent(cmdList, 0, L"debug_draws");
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		m_debugMaterial.BindPipeline(device, cmdList, RenderPass::DebugDraw, VertexFormat::Type::P3C3);
		cmdList->SetGraphicsRootConstantBufferView(0, view.GetConstantBuffer(bufferIndex)->GetGPUVirtualAddress());

		for (const auto& debugMesh : m_debugMeshes)
		{
			debugMesh->Render(cmdList);
		}
	}
}