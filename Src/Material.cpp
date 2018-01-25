#include "Material.h"

void Material::Init(std::string&& name)
{
	m_name = std::move(name);
}