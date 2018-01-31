#include "stdafx.h"
#include "Scene.h"

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

			vertexData.emplace_back(
				DirectX::XMFLOAT3(vertPos.x, vertPos.y, vertPos.z),
				DirectX::XMFLOAT3(vertNormal.x, vertNormal.y, vertNormal.z)
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

void Scene::LoadMaterials(const aiScene* loader, ID3D12Device* device, ID3D12CommandQueue* cmdQueue)
{
	// Used to batch texture uploads
	DirectX::ResourceUploadBatch resourceUpload(device);
	resourceUpload.Begin();

	for (auto matIdx = 0u; matIdx < loader->mNumMaterials; matIdx++)
	{
		const aiMaterial* srcMat = loader->mMaterials[matIdx];
		auto mat = std::make_unique<Material>();

		aiString materialName;
		srcMat->Get(AI_MATKEY_NAME, materialName);

		aiString textureNameStr;
		aiReturn ret = srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), textureNameStr);
		if (ret == aiReturn_SUCCESS)
		{
			std::string diffuseTextureName(textureNameStr.C_Str());

			uint32_t diffuseTextureIndex;
			const auto iter = m_textureDirectory.find(diffuseTextureName);
			if (iter != m_textureDirectory.cend())
			{
				diffuseTextureIndex = iter->second;
			}
			else
			{
				auto newTexture = std::make_unique<Texture>();
				newTexture->Init(device, resourceUpload, diffuseTextureName);
				m_textures.push_back(std::move(newTexture));
				diffuseTextureIndex = m_textures.size() - 1;
				m_textureDirectory[diffuseTextureName] = diffuseTextureIndex;
			}

			mat->Init(std::string(materialName.C_Str()), diffuseTextureIndex);
			m_materials.push_back(std::move(mat));
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

void Scene::InitResources(const uint32_t cbvRootParamIndex, ID3D12Device* device, ID3D12CommandQueue* cmdQueue, ID3D12GraphicsCommandList* cmdList)
{
	m_objectCBVRootParameterIndex = cbvRootParamIndex;

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
		LoadMaterials(scene, device, cmdQueue);
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
			HRESULT hr = device->CreateCommittedResource(
				&heapDesc,
				D3D12_HEAP_FLAG_NONE,
				&resDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(m_objectConstantBuffers.at(n).GetAddressOf())
			);
			assert(hr == S_OK && L"Failed to create constant buffer");

			// Get ptr to mapped resource
			auto** ptr = reinterpret_cast<void**>(&m_objectConstantBufferPtr.at(n));
			m_objectConstantBuffers.at(n)->Map(0, nullptr, ptr);
		}
	}
}

void Scene::InitDescriptors()
{

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

void Scene::Render(ID3D12GraphicsCommandList* cmdList, uint32_t bufferIndex)
{
	PIXScopedEvent(0, L"render_scene");

	int entityId = 0;
	for (auto& meshEntity : m_meshEntities)
	{
		cmdList->SetGraphicsRootConstantBufferView(
			m_objectCBVRootParameterIndex,
			m_objectConstantBuffers.at(bufferIndex)->GetGPUVirtualAddress() +
			entityId * sizeof(ObjectConstants)
		);

		m_meshes.at(meshEntity.GetMeshIndex())->Render(cmdList);
		++entityId;
	}
}