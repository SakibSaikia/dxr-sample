#pragma once

__declspec(align(256)) struct LightConstants
{
	DirectX::XMFLOAT3 direction;
	DirectX::XMFLOAT3 color;
	float brightness;
};

class Light
{
public:
	Light(const DirectX::XMFLOAT3 direction, const DirectX::XMFLOAT3 color, const float brightness);
	void Fill(LightConstants* lightConst) const;

private:
	DirectX::XMFLOAT3 m_direction;
	DirectX::XMFLOAT3 m_color;
	float m_brightness;
};