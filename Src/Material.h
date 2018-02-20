#pragma once

class Material
{
public:
	void Init(ID3D12Device* device, D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc, std::string&& name, uint32_t diffuseIdx);
	void Bind(ID3D12GraphicsCommandList* cmdList) const;
	bool IsValid() const;
	uint32_t GetDiffuseTextureIndex() const;
	static uint32_t GetDiffusemapRootParamIndex();

private:
	std::string m_name;
	bool m_valid = false;
	uint32_t m_diffuseTextureIndex;
	static D3D12_DESCRIPTOR_RANGE s_diffusemapRootDescriptorTable;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
	Microsoft::WRL::ComPtr<ID3DBlob> m_vsByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> m_psByteCode;
};
