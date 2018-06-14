#pragma once

#include "Common.h"

class MaterialPipeline
{
public:
	MaterialPipeline() = delete;
	MaterialPipeline(ID3D12Device* device, RenderPass renderPass, VertexFormat::Type vertexFormat, const std::string vs, const std::string ps, const std::string rootSig);
	void Bind(ID3D12GraphicsCommandList* cmdList) const;

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
};

class Material
{
public:
	Material() = default;
	Material(const std::string& name);

	virtual void BindPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderPass renderPass, VertexFormat::Type vertexFormat) = 0;
	virtual void BindConstants(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objectConstantsDescriptor) const = 0;
	virtual size_t GetHash(RenderPass renderPass, VertexFormat::Type vertexFormat) const = 0;
	bool IsValid() const;

protected:
	std::unique_ptr<MaterialPipeline> m_pipelines[RenderPass::Count][VertexFormat::Type::Count];
	std::string m_name;
	bool m_valid;
};

class NullMaterial : public Material
{
public:
	NullMaterial() = default;
	void BindPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderPass renderPass, VertexFormat::Type vertexFormat) override {};
	void BindConstants(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objectConstantsDescriptor) const override {};
	size_t GetHash(RenderPass renderPass, VertexFormat::Type vertexFormat) const override { return 0; }
};

class DefaultOpaqueMaterial : public Material
{
	static inline std::string k_vs = "mtl_default.vs.cso";
	static inline std::string k_ps = "mtl_default.ps.cso";
	static inline std::string k_rootSig = "mtl_default.rootsig.cso";
	static inline uint32_t k_objectConstantsDescriptorIndex = 1;
	static inline uint32_t k_srvDescriptorIndex = 3;

public:
	DefaultOpaqueMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);
	void BindPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderPass renderPass, VertexFormat::Type vertexFormat) override;
	void BindConstants(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objectConstantsDescriptor) const override;
	size_t GetHash(RenderPass renderPass, VertexFormat::Type vertexFormat) const override;

private:
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
	size_t m_hash;
};

class DefaultMaskedMaterial : public Material
{
	static inline std::string k_vs = "mtl_masked.vs.cso";
	static inline std::string k_ps = "mtl_masked.ps.cso";
	static inline std::string k_rootSig = "mtl_masked.rootsig.cso";
	static inline uint32_t k_objectConstantsDescriptorIndex = 1;
	static inline uint32_t k_srvDescriptorIndex = 3;

public:
	DefaultMaskedMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);
	void BindPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderPass renderPass, VertexFormat::Type vertexFormat) override;
	void BindConstants(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objectConstantsDescriptor) const override;
	size_t GetHash(RenderPass renderPass, VertexFormat::Type vertexFormat) const override;

private:
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
	size_t m_hash;
};

class DebugMaterial : public Material
{
	static inline std::string k_vs = "mtl_debug.vs.cso";
	static inline std::string k_ps = "mtl_debug.ps.cso";
	static inline std::string k_rootSig = "mtl_debug.rootsig.cso";

public:
	DebugMaterial();
	void BindPipeline(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, RenderPass renderPass, VertexFormat::Type vertexFormat) override;
	void BindConstants(ID3D12GraphicsCommandList* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objectConstantsDescriptor) const override {}
	size_t GetHash(RenderPass renderPass, VertexFormat::Type vertexFormat) const override { return 0; }
};