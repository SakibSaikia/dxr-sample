#pragma once
#include<string>

class Material
{
public:
	void Init(std::string&& name, const uint32_t diffuseIdx);
private:
	std::string m_name;
	uint32_t m_diffuseTextureIndex;
};
