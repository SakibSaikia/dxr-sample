#pragma once

#include "Common.h"

class MaterialPipeline
{
public:
	MaterialPipeline() = delete;
	MaterialPipeline(ID3D12Device5* device, RenderPass::Id renderPass, cconst std::string rgs, onst std::string chs, const std::string ms, const ID3D12RootSignature* rootSig);
	void Bind(ID3D12GraphicsCommandList4* cmdList) const;

private:
	Microsoft::WRL::ComPtr<ID3D12StateObject> m_pso;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_psoProperties;
};

class Material
{
public:
	Material() = default;
	Material(const std::string& name);

	virtual void BindPipeline(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, RenderPass::Id renderPass) = 0;
	virtual void BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const = 0;
	virtual size_t GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const = 0;
	bool IsValid() const;

protected:
	std::string m_name;
	bool m_valid;
};

class UntexturedMaterial : public Material
{
	static inline std::string k_rgs = "mtl_untextured.rgs.cso";
	static inline std::string k_chs = "mtl_untextured.chit.cso";
	static inline std::string k_ms = "mtl_untextured.miss.cso";

public:
	__declspec(align(256))
	struct Constants
	{
		DirectX::XMFLOAT3 baseColor;
		float metallic;
		float roughness;
	};

	UntexturedMaterial(std::string& name, Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer);
	void BindPipeline(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, RenderPass::Id renderPass) override;
	void BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const override;
	size_t GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const override;

private:
	static inline std::unique_ptr<MaterialPipeline> m_pipelines[RenderPass::Count];
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
	size_t m_hash[RenderPass::Count];
};

class DefaultOpaqueMaterial : public Material
{
	static inline std::string k_rgs = "mtl_default.rgs.cso";
	static inline std::string k_chs = "mtl_default.chit.cso";
	static inline std::string k_ms = "mtl_default.miss.cso";

public:
	DefaultOpaqueMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);
	void BindPipeline(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, RenderPass::Id renderPass) override;
	void BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const override;
	size_t GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const override;

private:
	static inline std::unique_ptr<MaterialPipeline> m_pipelines[RenderPass::Count];
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
	size_t m_hash[RenderPass::Count];
};

class DefaultMaskedMaterial : public Material
{
	static inline std::string k_rgs = "mtl_masked.rgs.cso";
	static inline std::string k_chs = "mtl_masked.chs.cso";
	static inline std::string k_ms = "mtl_masked.ms.cso";

public:
	DefaultMaskedMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle, const D3D12_GPU_DESCRIPTOR_HANDLE opacityMaskSrvHandle);
	void BindPipeline(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, RenderPass::Id renderPass) override;
	void BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const override;
	size_t GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const override;

private:
	static inline std::unique_ptr<MaterialPipeline> m_pipelines[RenderPass::Count];
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
	D3D12_GPU_DESCRIPTOR_HANDLE m_opacityMaskSrv;
	size_t m_hash[RenderPass::Count];
};