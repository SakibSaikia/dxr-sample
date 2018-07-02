#include "stdafx.h"
#include "Light.h"

Light::Light(const DirectX::XMFLOAT3 direction, const DirectX::XMFLOAT3 color, const float brightness) :
	m_color{ color },
	m_brightness{ brightness }
{
	DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&direction);
	DirectX::XMStoreFloat3(&m_direction, DirectX::XMVector2Normalize(dir));
}

void Light::FillConstants(LightConstants* lightConst) const
{
	lightConst->direction = m_direction;
	lightConst->color = m_color;
	lightConst->brightness = m_brightness;
	lightConst->lightViewProjectionMatrix = m_viewProjMatrix;
}

void Light::Update(float dt, const DirectX::BoundingBox& sceneBounds)
{
	if (m_dirty)
	{
		DirectX::XMVECTOR boundsCenter = DirectX::XMLoadFloat3(&sceneBounds.Center);
		DirectX::XMVECTOR boundsExtent = DirectX::XMLoadFloat3(&sceneBounds.Extents);

		// light view
		DirectX::XMVECTOR boundsRadius = DirectX::XMVector3Length(boundsExtent);
		DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&m_direction);

		DirectX::XMVECTOR lightPos = DirectX::XMVectorAdd(boundsCenter, DirectX::XMVectorScale(DirectX::XMVectorMultiply(boundsRadius, lightDir), 2.f));
		DirectX::XMVECTOR targetPos = boundsCenter;
		DirectX::XMVECTORF32 lightUp{ 0.f, 1.f, 0.f, 0.f };

		DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(lightPos, targetPos, lightUp);

		// transform bounds to light space
		DirectX::XMVECTOR lightBoundsCenter = DirectX::XMVector3Transform(boundsCenter, view);

		// light projection
		DirectX::XMVECTOR minExtent = DirectX::XMVectorSubtract(lightBoundsCenter, boundsExtent);
		DirectX::XMVECTOR maxExtent = DirectX::XMVectorAdd(lightBoundsCenter, boundsExtent);

		DirectX::XMFLOAT3 min, max;
		DirectX::XMStoreFloat3(&min, minExtent);
		DirectX::XMStoreFloat3(&max, maxExtent);

		DirectX::XMMATRIX proj = DirectX::XMMatrixOrthographicOffCenterLH(
			min.x, max.x,
			min.y, max.y,
			min.z, max.z
		);

		// light view projection
		DirectX::XMMATRIX viewProjMatrix = view * proj;

		DirectX::XMStoreFloat4x4(&m_viewMatrix, view);
		DirectX::XMStoreFloat4x4(&m_projMatrix, proj);
		DirectX::XMStoreFloat4x4(&m_viewProjMatrix, viewProjMatrix);

		m_dirty = false;
	}
}

const DirectX::XMFLOAT4X4& Light::GetViewMatrix() const
{
	return m_viewMatrix;
}

const DirectX::XMFLOAT4X4& Light::GetProjectionMatrix() const
{
	return m_projMatrix;
}

const DirectX::XMFLOAT4X4& Light::GetViewProjectionMatrix() const
{
	return m_viewProjMatrix;
}
