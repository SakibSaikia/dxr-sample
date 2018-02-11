#pragma once

class Material
{
public:
	void Init(std::string&& name, uint32_t diffuseIdx);
	void Bind(ID3D12GraphicsCommandList* cmdList) const;
	const uint32_t GetDiffuseTextureIndex() const;
	static uint32_t GetDiffusemapRootParamIndex();

private:
	std::string m_name;
	uint32_t m_diffuseTextureIndex;
	static D3D12_DESCRIPTOR_RANGE s_diffusemapRootDescriptorTable;
};
