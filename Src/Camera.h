#pragma once

#include <DirectXMath.h>
#include <windows.h>

class Camera
{
public:
	void Update(const float dt, const POINT mouseDelta);
	DirectX::XMFLOAT4X4 GetViewMatrix();

private:
	void Strafe(float d);
	void Walk(float d);
	void Pitch(float angle);
	void RotateWorldY(float angle);
	void UpdateViewMatrix();

private:
	DirectX::XMFLOAT3 m_position = { 0.f, 0.f, 0.f };
	DirectX::XMFLOAT3 m_right = { 1.f, 0.f, 0.f };
	DirectX::XMFLOAT3 m_up = { 0.f, 1.f, 0.f };
	DirectX::XMFLOAT3 m_look = { 0.f, 0.f, 1.f };

	bool m_viewDirty = false;
	DirectX::XMFLOAT4X4 m_viewMatrix;
};
