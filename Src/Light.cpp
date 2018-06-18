#include "stdafx.h"
#include "Light.h"

Light::Light(const DirectX::XMFLOAT3 direction, const DirectX::XMFLOAT3 color, const float brightness) :
	m_direction{ direction },
	m_color{ color },
	m_brightness{ brightness }
{
}

void Light::FillConstants(LightConstants* lightConst, const DirectX::BoundingBox& sceneBounds) const
{
	DirectX::XMVECTOR boundsCenter = DirectX::XMLoadFloat3(&sceneBounds.Center);
	DirectX::XMVECTOR boundsExtent = DirectX::XMLoadFloat3(&sceneBounds.Extents);

	// light view
	DirectX::XMVECTOR boundsRadius = DirectX::XMVector3Length(boundsExtent);
	DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&m_direction);

	DirectX::XMVECTOR lightPos = DirectX::XMVectorScale(DirectX::XMVectorMultiply(boundsRadius, lightDir), -2.f);
	DirectX::XMVECTOR targetPos = boundsCenter;
	DirectX::XMVECTORF32 lightUp{ 0.f, 1.f, 0.f, 0.f };

	DirectX::XMMATRIX lightView = DirectX::XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	// transform bounds to light space
	DirectX::XMVECTOR lightBoundsCenter = DirectX::XMVector3Transform(boundsCenter, lightView);

	// light projection
	DirectX::XMVECTOR minExtent = DirectX::XMVectorSubtract(lightBoundsCenter, boundsExtent);
	DirectX::XMVECTOR maxExtent = DirectX::XMVectorAdd(lightBoundsCenter, boundsExtent);

	DirectX::XMFLOAT3 min, max;
	DirectX::XMStoreFloat3(&min, minExtent);
	DirectX::XMStoreFloat3(&max, maxExtent);

	DirectX::XMMATRIX lightProj = DirectX::XMMatrixOrthographicOffCenterLH(
		min.x, max.x,
		min.y, max.y,
		min.z, max.z
	);

	// fill constants
	DirectX::XMStoreFloat4x4(&lightConst->lightViewProjectionMatrix, lightView * lightProj);
	lightConst->direction = m_direction;
	lightConst->color = m_color;
	lightConst->brightness = m_brightness;
}
