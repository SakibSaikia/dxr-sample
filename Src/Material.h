#pragma once

enum class MaterialType
{
	Default,
	Masked
};

struct MaterialPipelineDescription
{
	MaterialType type;
	uint32_t descriptorTableIndex;
	std::string rootsigBlobName;
	std::string vsBlobName;
	std::string psBlobName;
};

const MaterialPipelineDescription k_supportedMaterialPipelineDescriptions[] = {
	/*		type			descId			root signature				vertex shader			pixel shader		*/	
	{ MaterialType::Default,	2,		"mtl_default.rootsig.cso",	"mtl_default.vs.cso",	"mtl_default.ps.cso"	},
	{ MaterialType::Masked,		2,		"mtl_masked.rootsig.cso",	"mtl_masked.vs.cso",	"mtl_masked.ps.cso"		},
};

class MaterialPipeline
{
public:
	MaterialPipeline() = delete;
	MaterialPipeline(const MaterialPipelineDescription& desc, ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc);
	void Bind(ID3D12GraphicsCommandList* cmdList) const;

	MaterialType GetType() const;
	uint32_t GetDescriptorTableIndex() const;

private:
	MaterialType m_type;
	uint32_t m_descriptorTableIndex;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	Microsoft::WRL::ComPtr<ID3DBlob> vsByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> psByteCode;
};

class Material
{
public:
	Material() = default;
	Material(const MaterialPipeline* pipeline, std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);

	void BindPipeline(ID3D12GraphicsCommandList* cmdList);
	void BindConstants(ID3D12GraphicsCommandList* cmdList) const;
	bool IsValid() const;

private:
	const MaterialPipeline* m_pipeline;
	std::string m_name;
	uint32_t m_descriptorTableIndex;
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
	bool m_valid;
};

extern std::unordered_map<MaterialType, std::unique_ptr<MaterialPipeline>> k_materialPipelines;