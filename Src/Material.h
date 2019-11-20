#pragma once

#include "Common.h"

struct RaytraceShader
{
	Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
	D3D12_EXPORT_DESC* exportDesc;
};

struct RtMaterialPipeline
{
	RaytraceShader closestHitShader;
	RaytraceShader missShader;
	D3D12_HIT_GROUP_DESC* hitGroupDesc;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
};

class RaytraceMaterialPipeline
{
public:
	RaytraceMaterialPipeline(class StackAllocator& stackAlloc, std::vector<D3D12_STATE_SUBOBJECT>& subObjects, size_t& payloadIndex, ID3D12Device5* device);
	template<class MaterialType> void BuildFromMaterial(class StackAllocator& stackAlloc, std::vector<D3D12_STATE_SUBOBJECT>& subObjects, size_t payloadIndex, ID3D12Device5* device);
	void Commit(const class StackAllocator& stackAlloc, const std::vector<D3D12_STATE_SUBOBJECT>& subObjects, ID3D12Device5* device);
	void Bind(ID3D12GraphicsCommandList4* cmdList, uint8_t* pData, D3D12_GPU_VIRTUAL_ADDRESS viewCBV, D3D12_GPU_DESCRIPTOR_HANDLE tlasSRV, D3D12_GPU_DESCRIPTOR_HANDLE outputUAV) const;
	void* GetShaderIdentifier(const wchar_t* exportName) const;

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRaygenRootSignature(ID3D12Device5* device);
	template<class MaterialType> RtMaterialPipeline GetMaterialPipeline(StackAllocator& stackAlloc, ID3D12Device5* device);

private:
	Microsoft::WRL::ComPtr<ID3D12StateObject> m_pso;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_psoProperties;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raygenRootSignature;
};

class Material
{
public:
	Material(const std::string& name);
	virtual ~Material() {}

	virtual void BindConstants(
		uint8_t* pData,
		const RaytraceMaterialPipeline* pipeline,
		D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer,
		D3D12_GPU_VIRTUAL_ADDRESS objConstants,
		D3D12_GPU_VIRTUAL_ADDRESS viewConstants,
		D3D12_GPU_VIRTUAL_ADDRESS lightConstants
	) const = 0;

protected:
	std::string m_name;
};

class UntexturedMaterial : public Material
{
public:
	static inline wchar_t* k_name = L"Untextured";
	static inline wchar_t* k_chs = L"mtl_untextured.chs";
	static inline wchar_t* k_ms = L"mtl_untextured.miss";

	__declspec(align(256))
	struct Constants
	{
		DirectX::XMFLOAT3 baseColor;
		float metallic;
		float roughness;
	};

	UntexturedMaterial(std::string& name, Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer);
	void BindConstants(
		uint8_t* pData,
		const RaytraceMaterialPipeline* pipeline,
		D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer,
		D3D12_GPU_VIRTUAL_ADDRESS objConstants,
		D3D12_GPU_VIRTUAL_ADDRESS viewConstants, 
		D3D12_GPU_VIRTUAL_ADDRESS lightConstants
	) const override;

	static Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRaytraceRootSignature(ID3D12Device5* device);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
};

class DefaultOpaqueMaterial : public Material
{
public:
	static inline wchar_t* k_name = L"DefaultOpaque";
	static inline wchar_t* k_chs = L"mtl_default.chs";
	static inline wchar_t* k_ms = L"mtl_default.miss";

	DefaultOpaqueMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);
	void BindConstants(
		uint8_t* pData,
		const RaytraceMaterialPipeline* pipeline,
		D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer, 
		D3D12_GPU_VIRTUAL_ADDRESS objConstants, 
		D3D12_GPU_VIRTUAL_ADDRESS viewConstants, 
		D3D12_GPU_VIRTUAL_ADDRESS lightConstants
	) const override;

	static Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRaytraceRootSignature(ID3D12Device5* device);

private:
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
};

class DefaultMaskedMaterial : public Material
{
public:
	static inline wchar_t* k_name = L"DefaultMasked";
	static inline wchar_t* k_chs = L"mtl_masked.chs";
	static inline wchar_t* k_ms = L"mtl_masked.miss";

	DefaultMaskedMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle, const D3D12_GPU_DESCRIPTOR_HANDLE opacityMaskSrvHandle);
	void BindConstants(
		uint8_t* pData, 
		const RaytraceMaterialPipeline* pipeline,
		D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer, 
		D3D12_GPU_VIRTUAL_ADDRESS objConstants, 
		D3D12_GPU_VIRTUAL_ADDRESS viewConstants, 
		D3D12_GPU_VIRTUAL_ADDRESS lightConstants
	) const override;

	static Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRaytraceRootSignature(ID3D12Device5* device);

private:
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
	D3D12_GPU_DESCRIPTOR_HANDLE m_opacityMaskSrv;
};

template void RaytraceMaterialPipeline::BuildFromMaterial<UntexturedMaterial>(class StackAllocator& stackAlloc, std::vector<D3D12_STATE_SUBOBJECT>& subObjects, size_t payloadIndex, ID3D12Device5* device);
template void RaytraceMaterialPipeline::BuildFromMaterial<DefaultOpaqueMaterial>(class StackAllocator& stackAlloc, std::vector<D3D12_STATE_SUBOBJECT>& subObjects, size_t payloadIndex, ID3D12Device5* device);
template void RaytraceMaterialPipeline::BuildFromMaterial<DefaultMaskedMaterial>(class StackAllocator& stackAlloc, std::vector<D3D12_STATE_SUBOBJECT>& subObjects, size_t payloadIndex, ID3D12Device5* device);
template RtMaterialPipeline RaytraceMaterialPipeline::GetMaterialPipeline<UntexturedMaterial>(StackAllocator& stackAlloc, ID3D12Device5* device);
template RtMaterialPipeline RaytraceMaterialPipeline::GetMaterialPipeline<DefaultOpaqueMaterial>(StackAllocator& stackAlloc, ID3D12Device5* device);
template RtMaterialPipeline RaytraceMaterialPipeline::GetMaterialPipeline<DefaultMaskedMaterial>(StackAllocator& stackAlloc, ID3D12Device5* device);