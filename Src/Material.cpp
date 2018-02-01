#include "stdafx.h"
#include "Material.h"

uint32_t Material::s_diffusemapRootParamIndex;

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
	param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	param.Descriptor.RegisterSpace = 0;
	param.Descriptor.ShaderRegister = 0;
	rootParams.push_back(std::move(param));

	s_diffusemapRootParamIndex = rootParams.size() - 1;
}

uint32_t Material::GetDiffusemapRootParamIndex()
{
	return s_diffusemapRootParamIndex;
}