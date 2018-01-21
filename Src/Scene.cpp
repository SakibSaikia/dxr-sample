#include "Scene.h"
#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

std::vector<StaticMesh::keep_alive_type> Scene::Init(const uint32_t cbvRootParamIndex, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	m_objectCBVRootParameterIndex = cbvRootParamIndex;

	// Load scene
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile("..\\Content\\Sponza\\obj\\sponza.obj",
			aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_SortByPType |
			aiProcess_MakeLeftHanded |
			aiProcess_FlipWindingOrder |
			aiProcess_FlipUVs
		);

		assert(scene != nullptr && L"Failed to load scene");
	}


	std::vector<StaticMesh::keep_alive_type> ret;

	auto mesh = std::make_unique<StaticMesh>();
	auto keepAlive = mesh->Init(device, cmdList);

	m_meshes.push_back(std::move(mesh));
	ret.push_back(keepAlive);

	m_meshEntities.emplace_back(m_meshes.size() - 1, DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) * DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f));

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
			void** ptr = reinterpret_cast<void**>(&m_objectConstantBufferPtr.at(n));
			m_objectConstantBuffers.at(n)->Map(0, nullptr, ptr);
		}
	}

	return ret;
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