#pragma once

#include "Common.h"

class MaterialPipeline
{
public:
	MaterialPipeline() = delete;
	MaterialPipeline(ID3D12Device5* device, RenderPass::Id renderPass, VertexFormat::Type vertexFormat, const std::string vs, const std::string ps, const std::string rootSig);
	void Bind(ID3D12GraphicsCommandList4* cmdList) const;

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
};

class Material
{
public:
	Material() = default;
	Material(const std::string& name);

	virtual void BindPipeline(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, RenderPass::Id renderPass, VertexFormat::Type vertexFormat) = 0;
	virtual void BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const = 0;
	virtual size_t GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const = 0;
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
	void BindPipeline(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, RenderPass::Id renderPass, VertexFormat::Type vertexFormat) override {};
	void BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const override {};
	size_t GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const override { return 0; }
};

class UntexturedMaterial : public Material
{
	static inline std::string k_vs = "mtl_untextured.vs.cso";
	static inline std::string k_ps = "mtl_untextured.ps.cso";
	static inline std::string k_rootSig = "mtl_untextured.rootsig.cso";
	static inline std::string k_depthonly_vs = "mtl_depthonly_default.vs.cso";
	static inline std::string k_depthonly_ps = "mtl_depthonly_default.ps.cso";
	static inline std::string k_depthonly_rootSig = "mtl_depthonly_default.rootsig.cso";

public:
	__declspec(align(256))
	struct Constants
	{
		DirectX::XMFLOAT3 baseColor;
		float metallic;
		float roughness;
	};

	UntexturedMaterial(std::string& name, Microsoft::WRL::ComPtr<ID3D12Resource> constantBuffer);
	void BindPipeline(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, RenderPass::Id renderPass, VertexFormat::Type vertexFormat) override;
	void BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const override;
	size_t GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const override;

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
	size_t m_hash[RenderPass::Count];
};

class DefaultOpaqueMaterial : public Material
{
	static inline std::string k_vs = "mtl_default.vs.cso";
	static inline std::string k_ps = "mtl_default.ps.cso";
	static inline std::string k_rootSig = "mtl_default.rootsig.cso";
	static inline std::string k_depthonly_vs = "mtl_depthonly_default.vs.cso";
	static inline std::string k_depthonly_ps = "mtl_depthonly_default.ps.cso";
	static inline std::string k_depthonly_rootSig = "mtl_depthonly_default.rootsig.cso";

public:
	DefaultOpaqueMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);
	void BindPipeline(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, RenderPass::Id renderPass, VertexFormat::Type vertexFormat) override;
	void BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const override;
	size_t GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const override;

private:
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
	size_t m_hash[RenderPass::Count];
};

class DefaultMaskedMaterial : public Material
{
	static inline std::string k_vs = "mtl_masked.vs.cso";
	static inline std::string k_ps = "mtl_masked.ps.cso";
	static inline std::string k_rootSig = "mtl_masked.rootsig.cso";
	static inline std::string k_depthonly_vs = "mtl_depthonly_masked.vs.cso";
	static inline std::string k_depthonly_ps = "mtl_depthonly_masked.ps.cso";
	static inline std::string k_depthonly_rootSig = "mtl_depthonly_masked.rootsig.cso";

public:
	DefaultMaskedMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle, const D3D12_GPU_DESCRIPTOR_HANDLE opacityMaskSrvHandle);
	void BindPipeline(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, RenderPass::Id renderPass, VertexFormat::Type vertexFormat) override;
	void BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const override;
	size_t GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const override;

private:
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
	D3D12_GPU_DESCRIPTOR_HANDLE m_opacityMaskSrv;
	size_t m_hash[RenderPass::Count];
};

class DebugMaterial : public Material
{
	static inline std::string k_vs = "mtl_debug.vs.cso";
	static inline std::string k_ps = "mtl_debug.ps.cso";
	static inline std::string k_rootSig = "mtl_debug.rootsig.cso";

public:
	DebugMaterial();
	void BindPipeline(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, RenderPass::Id renderPass, VertexFormat::Type vertexFormat) override;
	void BindConstants(RenderPass::Id pass, ID3D12GraphicsCommandList4* cmdList, D3D12_GPU_VIRTUAL_ADDRESS objConstants, D3D12_GPU_VIRTUAL_ADDRESS viewConstants, D3D12_GPU_VIRTUAL_ADDRESS lightConstants, D3D12_GPU_VIRTUAL_ADDRESS shadowConstants, D3D12_GPU_DESCRIPTOR_HANDLE renderSurfaceSrvBegin) const override {}
	size_t GetHash(RenderPass::Id renderPass, VertexFormat::Type vertexFormat) const override { return 0; }
};