#include "stdafx.h"
#include "Scene.h"
#include "View.h"

Scene::~Scene()
{
	m_objectConstantBuffer->Unmap(0, nullptr);
	m_lightConstantBuffer->Unmap(0, nullptr);
	m_shadowConstantBuffer->Unmap(0, nullptr);
}

void Scene::LoadMeshes(
	const aiScene* loader, 
	ID3D12Device5* device, 
	ID3D12GraphicsCommandList4* cmdList, 
	UploadBuffer* uploadBuffer,
	ResourceHeap* scratchHeap,
	ResourceHeap* resourceHeap)
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
		mesh->Init(device, cmdList, uploadBuffer, scratchHeap, resourceHeap, std::move(vertexData), std::move(indexData), srcMesh->mMaterialIndex);
		m_meshes.push_back(std::move(mesh));
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE Scene::LoadTexture(const std::string& textureName, ID3D12Device5* device, ID3D12DescriptorHeap* srvHeap, const size_t srvOffset, const size_t srvDescriptorSize, DirectX::ResourceUploadBatch& resourceUpload)
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
	ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	ID3D12CommandQueue* cmdQueue,
	UploadBuffer* uploadBuffer, 
	ResourceHeap* mtlConstantsHeap,
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
			const auto opacityMaskDescriptor = LoadTexture(opacityMaskTextureNameStr.C_Str(), device, srvHeap, srvStartOffset + descriptorIdx++, srvDescriptorSize, resourceUpload);
			m_materials.push_back(std::make_unique<DefaultMaskedMaterial>(std::string(materialName.C_Str()), headDescriptor, opacityMaskDescriptor));
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
			// Use untextured material

			UntexturedMaterial::Constants mtlConstantData;
			mtlConstantData.baseColor = DirectX::XMFLOAT3{ 0.75, 0.75, 0.75 };
			mtlConstantData.metallic = 0.f;
			mtlConstantData.roughness = 0.5;

			D3D12_RESOURCE_DESC cbDesc = {};
			cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			cbDesc.Width = sizeof(UntexturedMaterial::Constants);
			cbDesc.Height = 1;
			cbDesc.DepthOrArraySize = 1;
			cbDesc.MipLevels = 1;
			cbDesc.Format = DXGI_FORMAT_UNKNOWN;
			cbDesc.SampleDesc.Count = 1;
			cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			size_t offsetInHeap = mtlConstantsHeap->GetAlloc(cbDesc.Width, k_constantBufferAlignment);

			Microsoft::WRL::ComPtr<ID3D12Resource> mtlCb;
			CHECK(device->CreatePlacedResource(
				mtlConstantsHeap->GetHeap(),
				offsetInHeap,
				&cbDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(mtlCb.GetAddressOf())
			));

			// copy constant data to upload buffer
			uint64_t cbSizeInBytes;
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT cbLayout;
			device->GetCopyableFootprints(
				&cbDesc,
				0 /*subresource index*/, 1 /* num subresources */, 0 /*offset*/,
				&cbLayout, nullptr, &cbSizeInBytes, nullptr);

			auto[uploadPtr, uploadOffset] = uploadBuffer->GetAlloc(cbSizeInBytes);
			const auto* pSrc = reinterpret_cast<const uint8_t*>(&mtlConstantData);
			memcpy(uploadPtr, pSrc, cbSizeInBytes);

			// schedule copy to default vertex buffer
			cmdList->CopyBufferRegion(
				mtlCb.Get(),
				0,
				uploadBuffer->GetResource(),
				uploadOffset,
				cbLayout.Footprint.Width
			);

			// transition to constant buffer
			D3D12_RESOURCE_BARRIER cbBarrierDesc = {};
			cbBarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			cbBarrierDesc.Transition.pResource = mtlCb.Get();
			cbBarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			cbBarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			cbBarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			cmdList->ResourceBarrier(
				1,
				&cbBarrierDesc
			);

			m_materials.push_back(std::make_unique<UntexturedMaterial>(std::string("untextured_mtl"), mtlCb));
		}
	}

	// Upload the resources to the GPU.
	auto uploadResourcesFinished = resourceUpload.End(cmdQueue);

	// Wait for the upload thread to terminate
	uploadResourcesFinished.wait();
}

void Scene::LoadEntities(const aiNode* node, ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, UploadBuffer* uploadBuffer, ResourceHeap* scratchHeap, ResourceHeap* resourceHeap)
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
				const uint64_t sceneMeshIndex = childNode->mMeshes[meshIdx];

				m_meshEntities.push_back(std::make_unique<StaticMeshEntity>(
					device,
					cmdList,
					uploadBuffer,
					scratchHeap,
					resourceHeap,
					m_meshes[sceneMeshIndex]->GetBLASAddress(),
					std::string(childNode->mName.C_Str()), 
					sceneMeshIndex,
					localToWorld));
			}
		}
		else
		{
			LoadEntities(childNode, device, cmdList, uploadBuffer, scratchHeap, resourceHeap);
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

void Scene::InitLights(ID3D12Device5* device)
{
	m_light = std::make_unique<Light>(DirectX::XMFLOAT3{ 0.57735f, 1.57735f, 0.57735f }, DirectX::XMFLOAT3{ 1.f, 1.f, 1.f }, 10000.f);
}

void Scene::InitBounds(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList)
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
}

void Scene::InitResources(
	ID3D12Device5* device, 
	ID3D12CommandQueue* cmdQueue, 
	ID3D12GraphicsCommandList4* cmdList, 
	UploadBuffer* uploadBuffer,
	ResourceHeap* scratchHeap,
	ResourceHeap* meshDataHeap,
	ResourceHeap* mtlConstantsHeap,
	ID3D12DescriptorHeap* srvHeap, 
	const size_t srvStartOffset,
	const size_t srvDescriptorSize)
{
	// Load scene
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(R"(..\Content\sponza\obj\sponza.obj)",
			aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType |
			aiProcess_MakeLeftHanded |
			aiProcess_FlipWindingOrder |
			aiProcess_FlipUVs
		);

		assert(scene != nullptr && L"Failed to load scene");

		LoadMeshes(scene, device, cmdList, uploadBuffer, scratchHeap, meshDataHeap);
		LoadMaterials(scene, device, cmdList, cmdQueue, uploadBuffer, mtlConstantsHeap, srvHeap, srvStartOffset, srvDescriptorSize);
		LoadEntities(scene->mRootNode, device, cmdList, uploadBuffer, scratchHeap, meshDataHeap);
		InitLights(device);
		InitBounds(device, cmdList);
		m_debugDraw.Init(device, cmdList);
	}

	// Object Constant Buffer
	{
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = sizeof(ObjectConstants) * m_meshEntities.size() * k_gfxBufferCount; // n copies where n = buffer count
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc.Count = 1;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		D3D12_HEAP_PROPERTIES heapDesc = {};
		heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD; // must be CPU accessible
		heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; // GPU or system mem

		CHECK(device->CreateCommittedResource(
			&heapDesc,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_objectConstantBuffer.GetAddressOf())
		));

		// Get ptr to mapped resource
		auto** ptr = reinterpret_cast<void**>(&m_objectConstantBufferPtr);
		m_objectConstantBuffer->Map(0, nullptr, ptr);

	}

	// Light Constant Buffer
	{
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = sizeof(LightConstants) * k_gfxBufferCount; // n copies where n = buffer count
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc.Count = 1;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		D3D12_HEAP_PROPERTIES heapDesc = {};
		heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD; // must be CPU accessible
		heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; // GPU or system mem


		CHECK(device->CreateCommittedResource(
			&heapDesc,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_lightConstantBuffer.GetAddressOf())
		));

		// Get ptr to mapped resource
		auto** ptr = reinterpret_cast<void**>(&m_lightConstantBufferPtr);
		m_lightConstantBuffer->Map(0, nullptr, ptr);
	}

	// Shadow Constant Buffer
	{
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = sizeof(ShadowConstants) * k_gfxBufferCount; // n copies where n = buffer count
		resDesc.Height = 1;
		resDesc.DepthOrArraySize = 1;
		resDesc.MipLevels = 1;
		resDesc.Format = DXGI_FORMAT_UNKNOWN;
		resDesc.SampleDesc.Count = 1;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		D3D12_HEAP_PROPERTIES heapDesc = {};
		heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD; // must be CPU accessible
		heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; // GPU or system mem

		CHECK(device->CreateCommittedResource(
			&heapDesc,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_shadowConstantBuffer.GetAddressOf())
		));

		// Get ptr to mapped resource
		auto** ptr = reinterpret_cast<void**>(&m_shadowConstantBufferPtr);
		m_shadowConstantBuffer->Map(0, nullptr, ptr);
	}
}

void Scene::Update(float dt)
{
	m_light->Update(dt, m_sceneBounds);

#if 0
	m_debugDraw.AddBox(m_sceneBounds, DirectX::XMFLOAT3{ 0.0, 1.0, 0.0 });

	// shadow frustum
	DirectX::BoundingBox lightBounds = { {0.f,0.f,0.5f}, {1.f,1.f,0.5f} };
	DirectX::XMMATRIX mat = DirectX::XMLoadFloat4x4(&m_light->GetViewProjectionMatrix());
	mat = DirectX::XMMatrixInverse(nullptr, mat);
	m_debugDraw.AddTransformedBox(lightBounds, mat, DirectX::XMFLOAT3{ 1.f, 0.f, 0.f });

	// light location
	DirectX::XMMATRIX m = DirectX::XMLoadFloat4x4(&m_light->GetViewMatrix());
	m = DirectX::XMMatrixInverse(nullptr, m);
	m_debugDraw.AddAxes(m, 200.f);

	m_debugDraw.AddAxes(DirectX::XMMatrixIdentity(), 200.f);
#endif
}

void Scene::UpdateRenderResources(uint32_t bufferIndex)
{
	// mesh entities
	ObjectConstants* o = m_objectConstantBufferPtr + bufferIndex * m_meshEntities.size();
	for (const auto& meshEntity : m_meshEntities)
	{
		meshEntity->FillConstants(o++);
	}

	// light(s)
	LightConstants* l = m_lightConstantBufferPtr + bufferIndex;
	ShadowConstants* s = m_shadowConstantBufferPtr + bufferIndex;
	m_light->FillConstants(l, s);

	// debug draw
	m_debugDraw.UpdateRenderResources(bufferIndex);
}

void Scene::Render(RenderPass::Id pass, ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, uint32_t bufferIndex, const View& view, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin)
{
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	int entityId = 0;
	size_t currentMaterialHash = 0;
	for (const auto& meshEntity : m_meshEntities)
	{
		StaticMesh* sm = m_meshes.at(meshEntity->GetMeshIndex()).get();
		Material* mat = m_materials.at(sm->GetMaterialIndex()).get();
		auto hash = mat->GetHash(pass, sm->GetVertexFormat());

		if (hash != currentMaterialHash)
		{
			PIXScopedEvent(cmdList, 0, L"bind_pipeline");

			// root sig
			mat->BindPipeline(device, cmdList, pass, sm->GetVertexFormat());

			currentMaterialHash = hash;
		}

		if (mat->IsValid())
		{
			PIXScopedEvent(cmdList, 0, meshEntity->GetName().c_str());

			D3D12_GPU_VIRTUAL_ADDRESS ObjConstants = m_objectConstantBuffer->GetGPUVirtualAddress() + (bufferIndex * m_meshEntities.size() + entityId) * sizeof(ObjectConstants);
			D3D12_GPU_VIRTUAL_ADDRESS viewConstants = view.GetConstantBuffer()->GetGPUVirtualAddress() + bufferIndex * sizeof(ViewConstants);
			D3D12_GPU_VIRTUAL_ADDRESS lightConstants = m_lightConstantBuffer->GetGPUVirtualAddress() + bufferIndex * sizeof(LightConstants);
			D3D12_GPU_VIRTUAL_ADDRESS shadowConstants = m_shadowConstantBuffer->GetGPUVirtualAddress() + bufferIndex * sizeof(ShadowConstants);

			static_assert(sizeof(ShadowConstants) == sizeof(ViewConstants) && L"Size must match so that we can switch out view constants in the shadow map pass!");

			mat->BindConstants(pass, cmdList, ObjConstants, viewConstants, lightConstants, shadowConstants, renderSurfaceSrvBegin);
			sm->Render(cmdList);
		}

		++entityId;
	}
}