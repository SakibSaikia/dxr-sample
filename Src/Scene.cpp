#include "Scene.h"

std::vector<StaticMesh::keep_alive_type> Scene::Init(const uint32_t cbvRootParamIndex, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	m_objectCBVRootParameterIndex = cbvRootParamIndex;

	std::vector<StaticMesh::keep_alive_type> ret;

	auto mesh = std::make_unique<StaticMesh>();
	auto keepAlive = mesh->Init(device, cmdList);

	m_meshes.emplace_back(std::move(mesh));
	ret.emplace_back(keepAlive);

	// Object Constant Buffer
	{
		uint32_t objectConstantsSize = sizeof(ObjectConstants);

		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resDesc.Width = objectConstantsSize * m_meshes.size();
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
	for (const auto& mesh : m_meshes)
	{
		c->localToWorldMatrix = mesh->GetLocalToWorldMatrix();
		c++;
	}
}

void Scene::Render(ID3D12GraphicsCommandList* cmdList, uint32_t bufferIndex)
{
	int meshId = 0;
	for (auto& mesh : m_meshes)
	{
		cmdList->SetGraphicsRootConstantBufferView(
			m_objectCBVRootParameterIndex, 
			m_objectConstantBuffers.at(bufferIndex)->GetGPUVirtualAddress() +
			meshId * sizeof(ObjectConstants));

		mesh->Render(cmdList);
		++meshId;
	}
}