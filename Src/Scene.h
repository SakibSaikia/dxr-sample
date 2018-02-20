#pragma once

#include "StaticMesh.h"
#include "Material.h"
#include "Texture.h"
#include "View.h"

constexpr size_t k_gfxBufferCount = 2;

class Scene
{
	__declspec(align(256)) struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 localToWorldMatrix;
	};

public:
	void InitResources(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, ID3D12GraphicsCommandList* cmdList, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
	void InitDescriptors(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, size_t startOffset, uint32_t descriptorSize);
	void Update(uint32_t bufferIndex);
	void Render(ID3D12GraphicsCommandList* cmdList, uint32_t bufferIndex, const View& view);

	const size_t GetNumTextures() const;

private:
	void LoadMeshes(const aiScene* loader, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void LoadMaterials(const aiScene* loader, ID3D12Device* device, ID3D12CommandQueue* cmdQueue, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
	void LoadEntities(const aiNode* node);

private:
	std::vector<std::unique_ptr<StaticMesh>> m_meshes;
	std::vector<std::unique_ptr<Material>> m_materials;
	std::vector<StaticMeshEntity> m_meshEntities;
	std::vector<std::unique_ptr<Texture>> m_textures;
	std::unordered_map<std::string, uint32_t> m_textureDirectory;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, k_gfxBufferCount> m_objectConstantBuffers;
	std::array<ObjectConstants*, k_gfxBufferCount> m_objectConstantBufferPtr;
};
