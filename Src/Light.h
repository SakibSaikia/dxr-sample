#pragma once

__declspec(align(256)) struct LightConstants
{
	DirectX::XMFLOAT3 direction;
	float _pad;
	DirectX::XMFLOAT3 color;
	float brightness;
	DirectX::XMFLOAT4X4 lightViewProjectionMatrix;
};

class Light
{
public:
	Light(const DirectX::XMFLOAT3 direction, const DirectX::XMFLOAT3 color, const float brightness);
	void FillConstants(LightConstants* lightConst, const DirectX::BoundingBox& sceneBounds) const;

private:
	DirectX::XMFLOAT3 m_direction;
	DirectX::XMFLOAT3 m_color;
	float m_brightness;
};