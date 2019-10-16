#pragma once

#include "Common.h"

struct MaterialRtPipelineDesc
{
	std::wstring closestHitShader;
	std::wstring missShader; 
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
};

class RaytraceMaterialPipeline
{
public:
	RaytraceMaterialPipeline(ID3D12Device5* device);
	void BuildFromMaterial(ID3D12Device5* device, std::wstring materialName, MaterialRtPipelineDesc pipelineDesc, std::vector<IUnknown*>& pendingResources);
	void Commit(ID3D12Device5* device, std::vector<IUnknown*>& pendingResources);
	void Bind(ID3D12GraphicsCommandList4* cmdList, uint8_t* pData, D3D12_GPU_VIRTUAL_ADDRESS viewCBV, D3D12_GPU_DESCRIPTOR_HANDLE tlasSRV, D3D12_GPU_DESCRIPTOR_HANDLE outputUAV) const;
	size_t GetRootSignatureSize() const;
	void* GetShaderIdentifier(const wchar_t* exportName) const;

private:
	static std::vector<D3D12_ROOT_PARAMETER> GetRaygenRootParams();

private:
	Microsoft::WRL::ComPtr<ID3D12StateObject> m_pso;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_psoProperties;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytraceRootSignature;
	size_t m_raytraceRootSignatureSize;
	std::vector<D3D12_STATE_SUBOBJECT> m_pipelineSubObjects;
	D3D12_STATE_SUBOBJECT* m_pPayloadSubobject;
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
	static inline wchar_t* k_chs = L"mtl_untextured.chs";
	static inline wchar_t* k_ms = L"mtl_untextured.miss";

public:
	__declspec(align(256))
	struct Constants
	{
		DirectX::XMFLOAT3 baseColor;
		float metallic;
		float roughness;
	};

	UntexturedMaterial(std::string& name, Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer);
	static MaterialRtPipelineDesc GetRaytracePipelineDesc();
	void BindConstants(
		uint8_t* pData,
		const RaytraceMaterialPipeline* pipeline,
		D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer,
		D3D12_GPU_VIRTUAL_ADDRESS objConstants,
		D3D12_GPU_VIRTUAL_ADDRESS viewConstants, 
		D3D12_GPU_VIRTUAL_ADDRESS lightConstants
	) const override;

private:
	static std::vector<D3D12_ROOT_PARAMETER> GetRaytraceRootParams();

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
};

class DefaultOpaqueMaterial : public Material
{
	static inline wchar_t* k_chs = L"mtl_default.chs";
	static inline wchar_t* k_ms = L"mtl_default.miss";

public:
	DefaultOpaqueMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);
	static MaterialRtPipelineDesc GetRaytracePipelineDesc();
	void BindConstants(
		uint8_t* pData,
		const RaytraceMaterialPipeline* pipeline,
		D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer, 
		D3D12_GPU_VIRTUAL_ADDRESS objConstants, 
		D3D12_GPU_VIRTUAL_ADDRESS viewConstants, 
		D3D12_GPU_VIRTUAL_ADDRESS lightConstants
	) const override;

private:
	static std::vector<D3D12_ROOT_PARAMETER> GetRaytraceRootParams();

private:
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
};

class DefaultMaskedMaterial : public Material
{
	static inline wchar_t* k_chs = L"mtl_masked.chs";
	static inline wchar_t* k_ms = L"mtl_masked.miss";

public:
	DefaultMaskedMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle, const D3D12_GPU_DESCRIPTOR_HANDLE opacityMaskSrvHandle);
	static MaterialRtPipelineDesc GetRaytracePipelineDesc();
	void BindConstants(
		uint8_t* pData, 
		const RaytraceMaterialPipeline* pipeline,
		D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer, 
		D3D12_GPU_VIRTUAL_ADDRESS objConstants, 
		D3D12_GPU_VIRTUAL_ADDRESS viewConstants, 
		D3D12_GPU_VIRTUAL_ADDRESS lightConstants
	) const override;

private:
	static std::vector<D3D12_ROOT_PARAMETER> GetRaytraceRootParams();

private:
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
	D3D12_GPU_DESCRIPTOR_HANDLE m_opacityMaskSrv;
};