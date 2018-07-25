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

void Light::FillConstants(LightConstants* lightConst, ShadowConstants* shadowConst) const
{
	lightConst->direction = m_direction;
	lightConst->color = m_color;
	lightConst->brightness = m_brightness;

	shadowConst->lightViewMatrix = m_viewMatrix;
	shadowConst->lightViewProjectionMatrix = m_viewProjMatrix;
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

	if (m_dirty)
	{
		DirectX::XMVECTOR boundsCenter = DirectX::XMLoadFloat3(&sceneBounds.Center);
		DirectX::XMVECTOR boundsExtent = DirectX::XMLoadFloat3(&sceneBounds.Extents);

		// light view
		DirectX::XMVECTOR boundsRadius = DirectX::XMVector3Length(boundsExtent);
		DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&m_direction);

		DirectX::XMVECTOR lightPos = DirectX::XMVectorScale(DirectX::XMVectorMultiply(boundsRadius, lightDir), 2.f);
		DirectX::XMVECTOR targetPos = boundsCenter;
		DirectX::XMVECTORF32 lightUp{ 0.f, 1.f, 0.f, 0.f };

		DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(lightPos, targetPos, lightUp);

		DirectX::BoundingSphere lightBounds;
		DirectX::BoundingSphere::CreateFromBoundingBox(lightBounds, sceneBounds);
		lightBounds.Transform(lightBounds, view);

		// light projection
		DirectX::XMFLOAT3 min = { lightBounds.Center.x - lightBounds.Radius, lightBounds.Center.y - lightBounds.Radius, lightBounds.Center.z - lightBounds.Radius };
		DirectX::XMFLOAT3 max = { lightBounds.Center.x + lightBounds.Radius, lightBounds.Center.y + lightBounds.Radius, lightBounds.Center.z + lightBounds.Radius };

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
