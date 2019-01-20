#include "stdafx.h"
#include "Common.h"
#include "Light.h"

Light::Light(const DirectX::XMFLOAT3 direction, const DirectX::XMFLOAT3 color, const float brightness) :
	m_color{ color },
	m_brightness{ brightness }
{
	DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&direction);
	DirectX::XMStoreFloat3(&m_originalDirection, DirectX::XMVector3Normalize(dir));
	m_direction = m_originalDirection;
}

void Light::FillConstants(LightConstants* lightConst) const
{
	lightConst->direction = m_direction;
	lightConst->color = m_color;
	lightConst->brightness = m_brightness;
}

void Light::Update(float dt, const DirectX::BoundingBox& sceneBounds)
{
	bool animateLight = false;
	if (animateLight)
	{
		static float lightRotationAngle = 0.f;
		lightRotationAngle += 0.05f*dt;

		DirectX::XMMATRIX r = DirectX::XMMatrixRotationY(lightRotationAngle);
		DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&m_originalDirection);
		lightDir = DirectX::XMVector3TransformNormal(lightDir, r);
		XMStoreFloat3(&m_direction, lightDir);
		m_dirty = true;
	}
}