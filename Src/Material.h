#pragma once

struct MaterialPipelineState
{
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
	Microsoft::WRL::ComPtr<ID3DBlob> vsByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> psByteCode;
};

// #TODO - These can probably be changed into static/const array tables

class DiffuseOnlyOpaqueMaterialPipeline
{
public:
	static const uint32_t k_srvDescriptorTableIndex = 2;

	static void Init(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc);
	static void Bind(ID3D12GraphicsCommandList* cmdList);
private:
	static MaterialPipelineState m_pipelineState;
};

class DiffuseOnlyMaskedMaterialPipeline
{
public:
	static const uint32_t k_srvDescriptorTableIndex = 2;

	static void Init(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc);
	static void Bind(ID3D12GraphicsCommandList* cmdList);
private:
	static MaterialPipelineState m_pipelineState;
};

class NormalMappedOpaqueMaterialPipeline
{
public:
	static const uint32_t k_srvDescriptorTableIndex = 2;

	static void Init(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc);
	static void Bind(ID3D12GraphicsCommandList* cmdList);
private:
	static MaterialPipelineState m_pipelineState;
};

class NormalMappedMaskedMaterialPipeline
{
public:
	static const uint32_t k_srvDescriptorTableIndex = 2;

	static void Init(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc);
	static void Bind(ID3D12GraphicsCommandList* cmdList);
private:
	static MaterialPipelineState m_pipelineState;
};

class Material
{
public:
	virtual void BindPipeline(ID3D12GraphicsCommandList* cmdList) const = 0;
	virtual void BindConstants(ID3D12GraphicsCommandList* cmdList) const = 0;
	virtual bool IsValid() const = 0;
};

class DiffuseOnlyOpaqueMaterial : public Material
{
	using pipeline_type = DiffuseOnlyOpaqueMaterialPipeline;

public:
	DiffuseOnlyOpaqueMaterial() = default;
	DiffuseOnlyOpaqueMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);

	void BindPipeline(ID3D12GraphicsCommandList* cmdList) const override;
	void BindConstants(ID3D12GraphicsCommandList* cmdList) const override;
	bool IsValid() const override;

private:
	bool m_valid;
	std::string m_name;
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
};

class DiffuseOnlyMaskedMaterial : public Material
{
	using pipeline_type = DiffuseOnlyMaskedMaterialPipeline;

public:
	DiffuseOnlyMaskedMaterial() = default;
	DiffuseOnlyMaskedMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);

	void BindPipeline(ID3D12GraphicsCommandList* cmdList) const override;
	void BindConstants(ID3D12GraphicsCommandList* cmdList) const override;
	bool IsValid() const override;

private:
	bool m_valid;
	std::string m_name;
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
};

class NormalMappedOpaqueMaterial : public Material
{
	using pipeline_type = NormalMappedOpaqueMaterialPipeline;

public:
	NormalMappedOpaqueMaterial() = default;
	NormalMappedOpaqueMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);

	void BindPipeline(ID3D12GraphicsCommandList* cmdList) const override;
	void BindConstants(ID3D12GraphicsCommandList* cmdList) const override;
	bool IsValid() const override;

private:
	bool m_valid;
	std::string m_name;
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
};

class NormalMappedMaskedMaterial : public Material
{
	using pipeline_type = NormalMappedMaskedMaterialPipeline;

public:
	NormalMappedMaskedMaterial() = default;
	NormalMappedMaskedMaterial(std::string& name, const D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);

	void BindPipeline(ID3D12GraphicsCommandList* cmdList) const override;
	void BindConstants(ID3D12GraphicsCommandList* cmdList) const override;
	bool IsValid() const override;

private:
	bool m_valid;
	std::string m_name;
	D3D12_GPU_DESCRIPTOR_HANDLE m_srvBegin;
};

