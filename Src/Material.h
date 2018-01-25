#pragma once
#include<string>

class Material
{
public:
	void Init(std::string&& name);
private:
	std::string m_name;
	uint32_t m_diffuseTextureIndex;
};
