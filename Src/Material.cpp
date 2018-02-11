#include "stdafx.h"
#include "Material.h"

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

uint32_t Material::GetDiffusemapRootParamIndex()
{
	return 2;
}