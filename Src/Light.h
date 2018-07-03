#pragma once

__declspec(align(256)) struct LightConstants
{
	DirectX::XMFLOAT3 direction;
	float _pad;
	DirectX::XMFLOAT3 color;
	float brightness;
};

__declspec(align(256))
struct ShadowConstants
{
	DirectX::XMFLOAT4X4 lightViewMatrix;
	DirectX::XMFLOAT4X4 lightViewProjectionMatrix;
};

class Light
{
public:
	Light(const DirectX::XMFLOAT3 direction, const DirectX::XMFLOAT3 color, const float brightness);
	void Update(float dt, const DirectX::BoundingBox& sceneBounds);
	void FillConstants(LightConstants* lightConst, ShadowConstants* shadowConst) const;

	const DirectX::XMFLOAT4X4& GetViewMatrix() const;
	const DirectX::XMFLOAT4X4& GetProjectionMatrix() const;
	const DirectX::XMFLOAT4X4& GetViewProjectionMatrix() const;

private:
	DirectX::XMFLOAT3 m_direction;
	DirectX::XMFLOAT3 m_color;
	float m_brightness;

	bool m_dirty = true;
	DirectX::XMFLOAT4X4 m_viewMatrix;
	DirectX::XMFLOAT4X4 m_projMatrix;
	DirectX::XMFLOAT4X4 m_viewProjMatrix;
};