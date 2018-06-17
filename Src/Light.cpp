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

	// light projection
	DirectX::XMVECTOR minExtent = DirectX::XMVectorSubtract(boundsCenter, boundsExtent);
	DirectX::XMVECTOR maxExtent = DirectX::XMVectorAdd(boundsCenter, boundsExtent);

	DirectX::XMFLOAT3 min, max;
	DirectX::XMStoreFloat3(&min, minExtent);
	DirectX::XMStoreFloat3(&max, maxExtent);

	DirectX::XMMATRIX lightProj = DirectX::XMMatrixOrthographicOffCenterLH(
		min.x, max.x,
		min.y, max.y,
		min.z, max.z
	);

	// texture space transform 
	DirectX::XMMATRIX texTransform{
		0.5f, 0.f, 0.f, 0.f,
		0.f, -0.5f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.5f, 0.5f, 0.f, 1.f
	};

	// shadow matrix 
	DirectX::XMMATRIX shadowMatrix = lightView * lightProj * texTransform;

	// fill constants
	DirectX::XMStoreFloat4x4(&lightConst->shadowMatrix, shadowMatrix);
	lightConst->direction = m_direction;
	lightConst->color = m_color;
	lightConst->brightness = m_brightness;
}
