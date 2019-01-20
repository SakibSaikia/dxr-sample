#pragma once

__declspec(align(256)) struct LightConstants
{
	DirectX::XMFLOAT3 direction;
	float _pad;
	DirectX::XMFLOAT3 color;
	float brightness;
};

class Light
{
public:
	Light(const DirectX::XMFLOAT3 direction, const DirectX::XMFLOAT3 color, const float brightness);
	void Update(float dt, const DirectX::BoundingBox& sceneBounds);
	void FillConstants(LightConstants* lightConst) const;

private:
	DirectX::XMFLOAT3 m_originalDirection;
	DirectX::XMFLOAT3 m_direction;
	DirectX::XMFLOAT3 m_color;
	float m_brightness;
};