#pragma once

class Material
{
public:
	void Init(std::string&& name, uint32_t diffuseIdx);
	const uint32_t GetDiffuseTextureIndex() const;
private:
	std::string m_name;
	uint32_t m_diffuseTextureIndex;
};
