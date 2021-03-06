#pragma once

#include "Common.h"
#include "StaticMesh.h"
#include "Material.h"
#include "Texture.h"
#include "View.h"
#include "Light.h"

class Scene
{
public:
	~Scene();

	void InitResources(
		ID3D12Device5* device, 
		ID3D12CommandQueue* cmdQueue, 
		ID3D12GraphicsCommandList4* cmdList, 
		UploadBuffer* uploadBuffer, 
		ResourceHeap* scratchHeap, 
		ResourceHeap* meshDataHeap, 
		ResourceHeap* mtlConstantsHeap, 
		ID3D12DescriptorHeap* srvHeap, 
		size_t srvDescriptorSize);

	void Update(float dt);

	void UpdateRenderResources(uint32_t bufferIndex);

	void Render(
		ID3D12Device5* device, 
		ID3D12GraphicsCommandList4* cmdList, 
		uint32_t bufferIndex, 
		const View& view, 
		const RaytraceMaterialPipeline* pipeline,
		D3D12_GPU_DESCRIPTOR_HANDLE outputUAV);

private:
	void LoadMeshes(
		const aiScene* loader, 
		ID3D12Device5* device, 
		ID3D12GraphicsCommandList4* cmdList, 
		UploadBuffer* uploadBuffer, 
		ResourceHeap* scratchHeap, 
		ResourceHeap* resourceHeap, 
		ID3D12DescriptorHeap* srvHeap, 
		const size_t srvStartOffset, 
		const size_t srvDescriptorSize);

	void LoadMaterials(
		const aiScene* loader, 
		ID3D12Device5* device, 
		ID3D12GraphicsCommandList4* cmdList, 
		ID3D12CommandQueue* cmdQueue, 
		UploadBuffer* uploadBuffer, 
		ResourceHeap* mtlConstantsHeap, 
		ID3D12DescriptorHeap* srvHeap, 
		const size_t srvStartOffset, 
		const size_t srvDescriptorSize);

	void LoadEntities(const aiNode* node);

	void CreateTLAS(
		ID3D12Device5* device, 
		ID3D12GraphicsCommandList4* cmdList, 
		UploadBuffer* uploadBuffer, 
		ResourceHeap* scratchHeap, 
		ResourceHeap* resourceHeap, 
		ID3D12DescriptorHeap* srvHeap, 
		const size_t srvStartOffset, 
		const size_t srvDescriptorSize);

	void CreateShaderBindingTable(ID3D12Device5* device);

	void InitLights(ID3D12Device5* device);

	D3D12_GPU_DESCRIPTOR_HANDLE LoadTexture(
		const std::string& textureName, 
		ID3D12Device5* device, 
		ID3D12DescriptorHeap* srvHeap, 
		const size_t srvOffset, 
		const size_t srvDescriptorSize, 
		DirectX::ResourceUploadBatch& resourceUpload);

private:
	std::vector<std::unique_ptr<StaticMesh>> m_meshes;
	std::vector<std::unique_ptr<StaticMeshEntity>> m_meshEntities;
	std::vector<DirectX::BoundingBox> m_meshWorldBounds;
	std::vector<std::unique_ptr<Material>> m_materials;
	std::vector<std::unique_ptr<Texture>> m_textures;
	std::unique_ptr<Light> m_light;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_tlasBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE m_tlasSrv;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_shaderBindingTable;
	uint8_t* m_sbtPtr = {};

	Microsoft::WRL::ComPtr<ID3D12Resource> m_objectConstantBuffer;
	ObjectConstants* m_objectConstantBufferPtr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_lightConstantBuffer;
	LightConstants* m_lightConstantBufferPtr;

	DirectX::BoundingBox m_sceneBounds;
};
