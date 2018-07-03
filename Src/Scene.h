#pragma once

#include "Common.h"
#include "StaticMesh.h"
#include "Material.h"
#include "Texture.h"
#include "View.h"
#include "DebugDraw.h"
#include "Light.h"

class Scene
{
public:
	void InitResources(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, ID3D12GraphicsCommandList* cmdList, ID3D12DescriptorHeap* srvHeap, size_t srvStartOffset, size_t srvDescriptorSize);
	void Update(float dt);
	void UpdateRenderResources(uint32_t bufferIndex);
	void Render(RenderPass::Id pass, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, uint32_t bufferIndex, const View& view, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin);
	void RenderDebugMeshes(RenderPass::Id pass, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, uint32_t bufferIndex, const View& view);

private:
	void LoadMeshes(const aiScene* loader, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void LoadMaterials(const aiScene* loader, ID3D12Device* device, ID3D12CommandQueue* cmdQueue, ID3D12DescriptorHeap* srvHeap, const size_t srvStartOffset, const size_t srvDescriptorSize);
	void LoadEntities(const aiNode* node);

	void InitLights(ID3D12Device* device);
	void InitBounds(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	D3D12_GPU_DESCRIPTOR_HANDLE LoadTexture(const std::string& textureName, ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, const size_t srvOffset, const size_t srvDescriptorSize, DirectX::ResourceUploadBatch& resourceUpload);

private:
	std::vector<std::unique_ptr<StaticMesh>> m_meshes;
	std::vector<std::unique_ptr<StaticMeshEntity>> m_meshEntities;
	std::vector<DirectX::BoundingBox> m_meshWorldBounds;
	std::vector<std::unique_ptr<Material>> m_materials;
	std::vector<std::unique_ptr<Texture>> m_textures;
	std::unique_ptr<Light> m_light;

	DebugMaterial m_debugMaterial;
	DebugLineMesh m_debugDraw;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_objectConstantBuffer;
	ObjectConstants* m_objectConstantBufferPtr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_lightConstantBuffer;
	LightConstants* m_lightConstantBufferPtr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_shadowConstantBuffer;
	ShadowConstants* m_shadowConstantBufferPtr;

	DirectX::BoundingBox m_sceneBounds;
};
