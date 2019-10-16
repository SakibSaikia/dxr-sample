#include "stdafx.h"
#include "Scene.h"
#include "View.h"

Scene::~Scene()
{
	m_objectConstantBuffer->Unmap(0, nullptr);
	m_lightConstantBuffer->Unmap(0, nullptr);
}

void Scene::LoadMeshes(
	const aiScene* loader, 
	ID3D12Device5* device, 
	ID3D12GraphicsCommandList4* cmdList, 
	UploadBuffer* uploadBuffer,
	ResourceHeap* scratchHeap,
	ResourceHeap* resourceHeap, 
	ID3D12DescriptorHeap* srvHeap, 
	const size_t srvStartOffset, 
	const size_t srvDescriptorSize)
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
		mesh->Init(device, cmdList, uploadBuffer, scratchHeap, resourceHeap, std::move(vertexData), std::move(indexData), srcMesh->mMaterialIndex, srvHeap, srvStartOffset + 2 * meshIdx, srvDescriptorSize);
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

	const auto gpuDescriptorHandle = tex->CreateShaderResourceView(device, srvHeap, srvOffset, srvDescriptorSize);

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
				const uint64_t sceneMeshIndex = childNode->mMeshes[meshIdx];

				m_meshEntities.push_back(std::make_unique<StaticMeshEntity>(
					std::string(childNode->mName.C_Str()), 
					sceneMeshIndex,
					localToWorld));
			}
		}
		else
		{
			LoadEntities(childNode);
		}
	}
}

void Scene::CreateTLAS(
	ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	UploadBuffer* uploadBuffer,
	ResourceHeap* scratchHeap,
	ResourceHeap* resourceHeap,
	ID3D12DescriptorHeap* srvHeap,
	const size_t srvHeapOffset,
	const size_t srvDescriptorSize)
{
	// Instance description for top level acceleration structure
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> sceneEntities;
	sceneEntities.reserve(m_meshEntities.size());

	for (size_t entityIndex = 0; entityIndex < m_meshEntities.size(); entityIndex++)
	{
		const StaticMeshEntity* meshEntity = m_meshEntities[entityIndex].get();
		const StaticMesh* mesh = m_meshes[meshEntity->GetMeshIndex()].get();

		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc;
		instanceDesc.InstanceID = entityIndex;
		instanceDesc.InstanceContributionToHitGroupIndex = 0;
		instanceDesc.InstanceMask = 1;
		instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		instanceDesc.AccelerationStructure = mesh->GetBLASAddress();

		// Transpose and convert to 3x4 matrix
		const DirectX::XMFLOAT4X4& localToWorld = meshEntity->GetLocalToWorldMatrix();
		decltype(instanceDesc.Transform)& dest = instanceDesc.Transform;
		dest[0][0] = localToWorld._11;	dest[1][0] = localToWorld._12;	dest[2][0] = localToWorld._13;
		dest[0][1] = localToWorld._21;	dest[1][1] = localToWorld._22;	dest[2][1] = localToWorld._23;
		dest[0][2] = localToWorld._31;	dest[1][2] = localToWorld._32;	dest[2][2] = localToWorld._33;
		dest[0][3] = localToWorld._41;	dest[1][3] = localToWorld._42;	dest[2][3] = localToWorld._43;

		sceneEntities.emplace_back(instanceDesc);
	}

	// Copy instance desc to GPU-side buffer
	const size_t instanceDescBufferSize = sceneEntities.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
	auto[destPtr, offset] = uploadBuffer->GetAlloc(instanceDescBufferSize);
	memcpy(destPtr, sceneEntities.data(), instanceDescBufferSize);

	// Compute size for top level acceleration structure buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS asInputs{};
	asInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	asInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	asInputs.InstanceDescs = uploadBuffer->GetResource()->GetGPUVirtualAddress() + offset;
	asInputs.NumDescs = static_cast<UINT>(sceneEntities.size());
	asInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO asPrebuildInfo{};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&asInputs, &asPrebuildInfo);

	const size_t alignedScratchSize = (asPrebuildInfo.ScratchDataSizeInBytes + D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1) & ~D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
	const size_t alignedTLASBufferSize = (asPrebuildInfo.ResultDataMaxSizeInBytes + D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1) & ~D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;

	// Create scratch buffer
	D3D12_RESOURCE_DESC scratchBufDesc = {};
	scratchBufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	scratchBufDesc.Alignment = max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	scratchBufDesc.Width = alignedScratchSize;
	scratchBufDesc.Height = 1;
	scratchBufDesc.DepthOrArraySize = 1;
	scratchBufDesc.MipLevels = 1;
	scratchBufDesc.Format = DXGI_FORMAT_UNKNOWN;
	scratchBufDesc.SampleDesc.Count = 1;
	scratchBufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	scratchBufDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	auto offsetInHeap = scratchHeap->GetAlloc(scratchBufDesc.Width);

	ID3D12Resource* scratchBuffer;
	CHECK(device->CreatePlacedResource(
		scratchHeap->GetHeap(),
		offsetInHeap,
		&scratchBufDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&scratchBuffer)
	));

	// Create TLAS buffer
	D3D12_RESOURCE_DESC tlasBufDesc = {};
	tlasBufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	tlasBufDesc.Alignment = std::max<UINT64>(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	tlasBufDesc.Width = alignedTLASBufferSize;
	tlasBufDesc.Height = 1;
	tlasBufDesc.DepthOrArraySize = 1;
	tlasBufDesc.MipLevels = 1;
	tlasBufDesc.Format = DXGI_FORMAT_UNKNOWN;
	tlasBufDesc.SampleDesc.Count = 1;
	tlasBufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	tlasBufDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	offsetInHeap = resourceHeap->GetAlloc(tlasBufDesc.Width);

	CHECK(device->CreatePlacedResource(
		resourceHeap->GetHeap(),
		offsetInHeap,
		&tlasBufDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS(m_tlasBuffer.GetAddressOf())
	));

	// Now, build the top level acceleration structure
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
	buildDesc.Inputs = asInputs;
	buildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = m_tlasBuffer->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// Insert UAV barrier 
	D3D12_RESOURCE_BARRIER uavBarrier{};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_tlasBuffer.Get();

	cmdList->ResourceBarrier(1, &uavBarrier);

	// TLAS SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC tlasSrvDesc{};
	tlasSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	tlasSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	tlasSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	tlasSrvDesc.RaytracingAccelerationStructure.Location = m_tlasBuffer->GetGPUVirtualAddress();

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHnd;
	cpuHnd.ptr = srvHeap->GetCPUDescriptorHandleForHeapStart().ptr + srvHeapOffset * srvDescriptorSize;
	m_tlasSrv.ptr = srvHeap->GetGPUDescriptorHandleForHeapStart().ptr + srvHeapOffset * srvDescriptorSize;

	device->CreateShaderResourceView(nullptr, &tlasSrvDesc, cpuHnd);
}

void Scene::CreateShaderBindingTable(ID3D12Device5* device)
{
	const size_t numEntities = m_meshEntities.size();
	const size_t sbtSize = k_shaderRecordSize * (1 + 2 * numEntities); // 1 for RGS. CHS and MS params are unique per material

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sbtSize * k_gfxBufferCount;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES heapDesc = {};
	heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// Create a shader binding table
	CHECK(device->CreateCommittedResource(
		&heapDesc,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_shaderBindingTable.GetAddressOf())
	));

	D3D12_RANGE readRange = { 0 };
	CHECK(m_shaderBindingTable->Map(0, &readRange, &reinterpret_cast<void*>(m_sbtPtr)));
}

void Scene::InitLights(ID3D12Device5* device)
{
	m_light = std::make_unique<Light>(DirectX::XMFLOAT3{ 0.57735f, 1.57735f, 0.57735f }, DirectX::XMFLOAT3{ 1.f, 1.f, 1.f }, 10000.f);
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
		assert(scene->mNumMeshes < k_objectCount && L"Increase k_objectCount");

		LoadMeshes(scene, device, cmdList, uploadBuffer, scratchHeap, meshDataHeap, srvHeap, SrvUav::MeshdataBegin, srvDescriptorSize);
		LoadMaterials(scene, device, cmdList, cmdQueue, uploadBuffer, mtlConstantsHeap, srvHeap, SrvUav::MaterialTexturesBegin, srvDescriptorSize);
		LoadEntities(scene->mRootNode);
		CreateTLAS(device, cmdList, uploadBuffer, scratchHeap, meshDataHeap, srvHeap, SrvUav::TLASBegin, srvDescriptorSize);
		CreateShaderBindingTable(device);
		InitLights(device);
	}

	// Object Constant Buffer
	{
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = sizeof(ObjectConstants) * m_meshEntities.size() * k_gfxBufferCount;
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
		resDesc.Width = sizeof(LightConstants) * k_gfxBufferCount;
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
}

void Scene::Update(float dt)
{
	m_light->Update(dt, m_sceneBounds);
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
	m_light->FillConstants(l);
}

void Scene::Render(
	ID3D12Device5* device, 
	ID3D12GraphicsCommandList4* cmdList, 
	uint32_t bufferIndex, 
	const View& view, 
	const RaytraceMaterialPipeline* pipeline, 
	D3D12_GPU_DESCRIPTOR_HANDLE outputUAV
)
{
	const size_t sbtSize = k_shaderRecordSize * ( 1 + 2 * m_meshEntities.size());
	uint8_t* pData = m_sbtPtr + bufferIndex * sbtSize;
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = m_shaderBindingTable->GetGPUVirtualAddress() + bufferIndex * sbtSize;

	D3D12_GPU_VIRTUAL_ADDRESS viewConstants = view.GetConstantBuffer()->GetGPUVirtualAddress() + bufferIndex * sizeof(ViewConstants);
	D3D12_GPU_VIRTUAL_ADDRESS lightConstants = m_lightConstantBuffer->GetGPUVirtualAddress() + bufferIndex * sizeof(LightConstants);

	// Bind pipeline
	pipeline->Bind(cmdList, pData, viewConstants, m_tlasSrv, outputUAV);
	pData += k_shaderRecordSize;

	// Populate SBT
	int entityId = 0;
	for (const auto& meshEntity : m_meshEntities)
	{
		PIXScopedEvent(cmdList, 0, meshEntity->GetName().c_str());

		StaticMesh* sm = m_meshes.at(meshEntity->GetMeshIndex()).get();
		uint32_t materialIndex = sm->GetMaterialIndex();
		Material* mat = m_materials.at(materialIndex).get();

		D3D12_GPU_VIRTUAL_ADDRESS objConstants = m_objectConstantBuffer->GetGPUVirtualAddress() + (bufferIndex * m_meshEntities.size() + entityId) * sizeof(ObjectConstants);

		uint8_t* materialData = pData + materialIndex * k_shaderRecordSize;

		mat->BindConstants(
			materialData, 
			pipeline, 
			sm->GetVertexAndIndexBufferSRVHandle(), 
			objConstants,
			viewConstants, 
			lightConstants);

		++entityId;
	}

	// Dispatch rays!
	D3D12_DISPATCH_RAYS_DESC desc = {};
	desc.RayGenerationShaderRecord.StartAddress = gpuAddress;
	desc.RayGenerationShaderRecord.SizeInBytes = k_shaderRecordSize;

	desc.MissShaderTable.StartAddress = desc.RayGenerationShaderRecord.StartAddress + k_shaderRecordSize;
	desc.MissShaderTable.SizeInBytes = 2 * k_shaderRecordSize * m_meshEntities.size();
	desc.MissShaderTable.StrideInBytes = 2 * k_shaderRecordSize; // miss and hit shaders are interleaved

	desc.HitGroupTable.StartAddress = desc.MissShaderTable.StartAddress + k_shaderRecordSize;
	desc.HitGroupTable.SizeInBytes = 2 * k_shaderRecordSize * m_meshEntities.size();
	desc.HitGroupTable.StrideInBytes = 2 * k_shaderRecordSize; // miss and hit shaders are interleaved

	desc.Width = k_screenWidth;
	desc.Height = k_screenHeight;
	desc.Depth = 1;

	cmdList->DispatchRays(&desc);
}