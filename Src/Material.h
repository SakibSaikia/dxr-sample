#pragma once

#include "Common.h"

using MaterialRtPipelineDesc = std::tuple<std::string, std::string, D3D12_ROOT_SIGNATURE_DESC>;

class RaytraceMaterialPipeline
{
public:
	RaytraceMaterialPipeline(ID3D12Device5* device, RenderPass::Id pass);
	void BuildFromMaterial(ID3D12Device5* device, std::wstring materialName, MaterialRtPipelineDesc pipelineDesc, std::vector<IUnknown*>& pendingResources);
	void Commit(ID3D12Device5* device, std::vector<IUnknown*>& pendingResources);
	void Bind(ID3D12GraphicsCommandList4* cmdList, uint8_t* pData, RenderPass::Id pass, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_DESCRIPTOR_HANDLE outputUAV) const override;
	size_t GetRootSignatureSize() const;
	void* GetShaderIdentifier(const std::string& exportName) const;

private:
	static D3D12_ROOT_SIGNATURE_DESC BuildRaygenRootSignature(RenderPass::Id pass);

private:
	Microsoft::WRL::ComPtr<ID3D12StateObject> m_pso;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_psoProperties;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytraceRootSignature;
	size_t m_raytraceRootSignatureSize;
	std::list<D3D12_STATE_SUBOBJECT> m_pipelineSubObjects;
	D3D12_STATE_SUBOBJECT* m_pPayloadSubobject;
};

class Material
{
public:
	Material() = default;
	Material(const std::string& name);

	virtual void BindConstants(uint32_t bufferIndex, RenderPass::Id pass, D3D12_GPU_VIRTUAL_ADDRESS tlas, D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_DESCRIPTOR_HANDLE outputUAV) const = 0;
	bool IsValid() const;

protected:
	std::string m_name;
	bool m_valid;
};

class UntexturedMaterial : public Material
{
	static inline std::string k_chs = "mtl_untextured.chs";
	static inline std::string k_ms = "mtl_untextured.miss";

public:
	__declspec(align(256))
	struct Constants
	{
		DirectX::XMFLOAT3 baseColor;
		float metallic;
		float roughness;
	};

	UntexturedMaterial(std::string& name, Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer);
	static MaterialRtPipelineDesc GetRaytracePipelineDesc(RenderPass::Id renderPass);
	void BindConstants(uint8_t* pData, RaytraceMaterialPipeline* pipeline, D3D12_GPU_VIRTUAL_ADDRESS tlas, D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_DESCRIPTOR_HANDLE outputUAV) const override;

private:
	static D3D12_ROOT_SIGNATURE_DESC BuildRaytraceRootSignature(RenderPass::Id pass);

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
	size_t m_hash[RenderPass::Count];
};

class DefaultOpaqueMaterial : public Material
{
	static inline std::string k_chs = "mtl_default.chs";
	static inline std::string k_ms = "mtl_default.miss";

public:
	DefaultOpaqueMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);
	static MaterialRtPipelineDesc GetRaytracePipelineDesc(RenderPass::Id renderPass);
	void BindConstants(uint8_t* pData, RaytraceMaterialPipeline* pipeline, D3D12_GPU_VIRTUAL_ADDRESS tlas, D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_DESCRIPTOR_HANDLE outputUAV) const override;

private:
	static D3D12_ROOT_SIGNATURE_DESC BuildRaytraceRootSignature(ID3D12Device5* device, RenderPass::Id pass);

private:
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
	size_t m_hash[RenderPass::Count];
};

class DefaultMaskedMaterial : public Material
{
	static inline std::string k_chs = "mtl_masked.chs";
	static inline std::string k_ms = "mtl_masked.miss";

public:
	DefaultMaskedMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle, const D3D12_GPU_DESCRIPTOR_HANDLE opacityMaskSrvHandle);
	static MaterialRtPipelineDesc GetRaytracePipelineDesc(RenderPass::Id renderPass);
	void BindConstants(uint8_t* pData, RaytraceMaterialPipeline* pipeline, D3D12_GPU_VIRTUAL_ADDRESS tlas, D3D12_GPU_DESCRIPTOR_HANDLE meshBuffer, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_DESCRIPTOR_HANDLE outputUAV) const override;

private:
	static D3D12_ROOT_SIGNATURE_DESC BuildRaytraceRootSignature(ID3D12Device5* device, RenderPass::Id pass);

private:
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
	D3D12_GPU_DESCRIPTOR_HANDLE m_opacityMaskSrv;
	size_t m_hash[RenderPass::Count];
};