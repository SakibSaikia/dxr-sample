#pragma once

#include "StaticMesh.h"
#include "Material.h"
#include "Texture.h"

constexpr size_t k_gfxBufferCount = 2;

class Scene
{
	__declspec(align(256)) struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 localToWorldMatrix;
	};

public:
	void InitResources(uint32_t cbvRootParamIndex, ID3D12Device* device, ID3D12CommandQueue* cmdQueue, ID3D12GraphicsCommandList* cmdList);
	void InitDescriptors();
	void Update(uint32_t bufferIndex);
	void Render(ID3D12GraphicsCommandList* cmdList, uint32_t bufferIndex);

	const size_t GetNumTextures() const;

private:
	void LoadMeshes(const aiScene* loader, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void LoadMaterials(const aiScene* loader, ID3D12Device* device, ID3D12CommandQueue* cmdQueue);
	void LoadEntities(const aiNode* node);

private:
	std::vector<std::unique_ptr<StaticMesh>> m_meshes;
	std::vector<std::unique_ptr<Material>> m_materials;
	std::vector<StaticMeshEntity> m_meshEntities;
	std::vector<std::unique_ptr<Texture>> m_textures;
	std::unordered_map<std::string, uint32_t> m_textureDirectory;

	uint32_t m_objectCBVRootParameterIndex;
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, k_gfxBufferCount> m_objectConstantBuffers;
	std::array<ObjectConstants*, k_gfxBufferCount> m_objectConstantBufferPtr;
};
