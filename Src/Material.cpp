#include "stdafx.h"
#include "Material.h"

uint32_t Material::s_diffusemapRootParamIndex;
D3D12_DESCRIPTOR_RANGE Material::s_diffusemapRootDescriptorTable = { D3D12_DESCRIPTOR_RANGE_TYPE_SRV , 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND };

void Material::Init(std::string&& name, const uint32_t diffuseIdx) 
{
	m_name = std::move(name);
	m_diffuseTextureIndex = diffuseIdx;
}

void Material::Bind(ID3D12GraphicsCommandList* cmdList) const
{

}

const uint32_t Material::GetDiffuseTextureIndex() const
{
	return m_diffuseTextureIndex;
}

void Material::AppendRootParameters(std::vector<D3D12_ROOT_PARAMETER>& rootParams)
{
	D3D12_ROOT_PARAMETER param;
	param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	param.DescriptorTable.NumDescriptorRanges = 1;
	param.DescriptorTable.pDescriptorRanges = &s_diffusemapRootDescriptorTable;
	rootParams.push_back(std::move(param));

	s_diffusemapRootParamIndex = rootParams.size() - 1;
}

void Material::AppendStaticSamplers(std::vector<D3D12_STATIC_SAMPLER_DESC>& staticSamplers)
{
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_ANISOTROPIC;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.ShaderRegister = 0;
	sampler.MaxAnisotropy = 8;
	sampler.MinLOD = 0;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	staticSamplers.push_back(std::move(sampler));
}

uint32_t Material::GetDiffusemapRootParamIndex()
{
	return s_diffusemapRootParamIndex;
}