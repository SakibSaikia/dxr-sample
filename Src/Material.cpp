#include "Material.h"

void Material::Init(std::string&& name, const uint32_t diffuseIdx) 
{
	m_name = std::move(name);
	m_diffuseTextureIndex = diffuseIdx;
}