#include "stdafx.h"
#include "Camera.h"

void Camera::UpdateViewMatrix()
{
	if (m_viewDirty)
	{
		DirectX::XMVECTOR R = DirectX::XMLoadFloat4(&m_right);
		DirectX::XMVECTOR U = DirectX::XMLoadFloat4(&m_up);
		DirectX::XMVECTOR L = DirectX::XMLoadFloat4(&m_look);
		DirectX::XMVECTOR P = DirectX::XMLoadFloat4(&m_position);

		L = DirectX::XMVector4Normalize(L);
		U = DirectX::XMVector4Normalize(DirectX::XMVector3Cross(L, R));
		R = DirectX::XMVector3Cross(U, L);

		// v = inv(T) * transpose(R)
		float x = -DirectX::XMVectorGetX(DirectX::XMVector4Dot(P, R));
		float y = -DirectX::XMVectorGetX(DirectX::XMVector4Dot(P, U));
		float z = -DirectX::XMVectorGetX(DirectX::XMVector4Dot(P, L));

		DirectX::XMStoreFloat4(&m_right, R);
		DirectX::XMStoreFloat4(&m_up, U);
		DirectX::XMStoreFloat4(&m_look, L);

		m_viewMatrix(0, 0) = m_right.x;
		m_viewMatrix(1, 0) = m_right.y;
		m_viewMatrix(2, 0) = m_right.z;
		m_viewMatrix(3, 0) = x;

		m_viewMatrix(0, 1) = m_up.x;
		m_viewMatrix(1, 1) = m_up.y;
		m_viewMatrix(2, 1) = m_up.z;
		m_viewMatrix(3, 1) = y;

		m_viewMatrix(0, 2) = m_look.x;
		m_viewMatrix(1, 2) = m_look.y;
		m_viewMatrix(2, 2) = m_look.z;
		m_viewMatrix(3, 2) = z;

		m_viewMatrix(0, 3) = 0.0f;
		m_viewMatrix(1, 3) = 0.0f;
		m_viewMatrix(2, 3) = 0.0f;
		m_viewMatrix(3, 3) = 1.0f;

		m_viewDirty = false;

		// View projection matrix
		DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&m_projMatrix);
		DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&m_viewMatrix);
		DirectX::XMMATRIX viewProjMatrix = view * proj;
		XMStoreFloat4x4(&m_viewProjMatrix, viewProjMatrix);
	}
}

DirectX::XMFLOAT4 Camera::GetPosition()
{
	return m_position;
}

DirectX::XMFLOAT4 Camera::GetUp()
{
	return m_up;
}

DirectX::XMFLOAT4 Camera::GetRight()
{
	return m_right;
}

DirectX::XMFLOAT4 Camera::GetLook()
{
	return m_look;
}

DirectX::XMFLOAT4X4 Camera::GetViewMatrix()
{
	return m_viewMatrix;
}

DirectX::XMFLOAT4X4 Camera::GetProjectionMatrix()
{
	return m_projMatrix;
}

DirectX::XMFLOAT4X4 Camera::GetViewProjectionMatrix()
{
	return m_viewProjMatrix;
}

void FirstPersonCamera::Strafe(float d)
{
	// m_position += d*m_right
	DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
	DirectX::XMVECTOR r = DirectX::XMLoadFloat4(&m_right);
	DirectX::XMVECTOR p = DirectX::XMLoadFloat4(&m_position);
	DirectX::XMStoreFloat4(&m_position, DirectX::XMVectorMultiplyAdd(s, r, p));

	m_viewDirty = true;
}

void FirstPersonCamera::Walk(float d)
{
	// m_position += d*m_look
	DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
	DirectX::XMVECTOR l = DirectX::XMLoadFloat4(&m_look);
	DirectX::XMVECTOR p = DirectX::XMLoadFloat4(&m_position);
	DirectX::XMStoreFloat4(&m_position, DirectX::XMVectorMultiplyAdd(s, l, p));

	m_viewDirty = true;
}

void FirstPersonCamera::Pitch(float angle)
{
	// Rotate up and look vector about the right vector.
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat4(&m_right), angle);

	DirectX::XMVECTOR u = DirectX::XMLoadFloat4(&m_up);
	DirectX::XMVECTOR l = DirectX::XMLoadFloat4(&m_look);
	DirectX::XMStoreFloat4(&m_up, DirectX::XMVector3TransformNormal(u, R));
	DirectX::XMStoreFloat4(&m_look, DirectX::XMVector3TransformNormal(l, R));

	m_viewDirty = true;
}

void FirstPersonCamera::RotateWorldY(float angle)
{
	// Rotate the basis vectors about the world y-axis.
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(angle);

	DirectX::XMVECTOR r = DirectX::XMLoadFloat4(&m_right);
	DirectX::XMVECTOR u = DirectX::XMLoadFloat4(&m_up);
	DirectX::XMVECTOR l = DirectX::XMLoadFloat4(&m_look);
	DirectX::XMStoreFloat4(&m_right, DirectX::XMVector3TransformNormal(r, R));
	DirectX::XMStoreFloat4(&m_up, DirectX::XMVector3TransformNormal(u, R));
	DirectX::XMStoreFloat4(&m_look, DirectX::XMVector3TransformNormal(l, R));

	m_viewDirty = true;
}

void FirstPersonCamera::Init(const float aspectRatio)
{
	DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(0.25f * 3.1415926535f, aspectRatio, 1.0f, 10000.0f);
	XMStoreFloat4x4(&m_projMatrix, proj);

	m_position = { 0.f, 0.f, 0.f, 1.f };
	m_right = { 1.f, 0.f, 0.f, 0.f };
	m_up = { 0.f, 1.f, 0.f, 0.f };
	m_look = { 0.f, 0.f, 1.f, 0.f };
}

void FirstPersonCamera::Update(const float dt, WPARAM mouseBtnState, const POINT mouseDelta)
{
	if ((GetAsyncKeyState('W') & 0x8000) != 0)
	{
		Walk(1000.f * dt);
	}

	if ((GetAsyncKeyState('S') & 0x8000) != 0)
	{
		Walk(-1000.f * dt);
	}

	if ((GetAsyncKeyState('A') & 0x8000) != 0)
	{
		Strafe(-1000.f * dt);
	}

	if ((GetAsyncKeyState('D') & 0x8000) != 0)
	{
		Strafe(1000.f * dt);
	}

	// Make each pixel correspond to a degree.
	float dx = DirectX::XMConvertToRadians(1.f*static_cast<float>(mouseDelta.x));
	float dy = DirectX::XMConvertToRadians(1.f*static_cast<float>(mouseDelta.y));

	Pitch(dy);
	RotateWorldY(dx);

	UpdateViewMatrix();
}

void TrackballCamera::SphericalToViewBasis()
{
	m_position = { m_radius * std::sin(m_elev) * std::cos(m_azimuth), m_radius * std::cos(m_elev), m_radius * std::sin(m_elev) * std::sin(m_azimuth), 1.f };
	m_look = { -m_position.x, -m_position.y, -m_position.z, 0.f };

	if (m_worldUpDir > 0.f)
	{
		m_up = { 0.f, 1.f, 0.f, 0.f };
	}
	else
	{
		m_up = { 0.f, -1.f, 0.f, 0.f };
	}

	DirectX::XMVECTOR U = DirectX::XMLoadFloat4(&m_up);
	DirectX::XMVECTOR L = DirectX::XMLoadFloat4(&m_look);
	DirectX::XMStoreFloat4(&m_right, DirectX::XMVector4Normalize(DirectX::XMVector3Cross(U, L)));
}

void TrackballCamera::Init(const float aspectRatio)
{
	DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(0.25f * 3.1415926535f, aspectRatio, 1.0f, 10000.0f);
	XMStoreFloat4x4(&m_projMatrix, proj);

	m_radius = 5.f;
	m_elev = 0.25f * DirectX::XM_PI;
	m_azimuth = 1.5f * DirectX::XM_PI;
	m_worldUpDir = 1.f;
	m_viewDirty = true;

	SphericalToViewBasis();
	UpdateViewMatrix();
}

void TrackballCamera::Update(const float dt, WPARAM mouseBtnState, POINT mouseDelta)
{
	// Make each pixel correspond to a degree
	float dx = DirectX::XMConvertToRadians(static_cast<float>(mouseDelta.x));
	float dy = DirectX::XMConvertToRadians(static_cast<float>(mouseDelta.y));

	if ((mouseBtnState & MK_LBUTTON) != 0)
	{
		Rotate(dx, dy);
	}

	if ((mouseBtnState & MK_RBUTTON) != 0)
	{
		float deltaSize = std::sqrt(dx*dx + dy*dy);
		Zoom(std::copysign(deltaSize, -dx));
	}

	SphericalToViewBasis();
	UpdateViewMatrix();
}

void TrackballCamera::Rotate(float dx, float dy)
{
	if (m_worldUpDir > 0.f)
	{
		m_azimuth -= dx;
	}
	else
	{
		m_azimuth += dx;
	}

	m_elev -= dy;

	if (m_elev > DirectX::XM_2PI)
	{
		m_elev -= DirectX::XM_2PI;
	}
	else if(m_elev < -DirectX::XM_2PI)
	{
		m_elev += DirectX::XM_2PI;
	}

	if ((m_elev > 0 && m_elev < DirectX::XM_PI) || (m_elev < -DirectX::XM_PI && m_elev > -DirectX::XM_2PI))
	{
		m_worldUpDir = 1.0f;
	}
	else 
	{
		m_worldUpDir = -1.0f;
	}

	m_viewDirty = true;
}

void TrackballCamera::Zoom(float d)
{
	m_radius += 0.25f * d;
	m_viewDirty = true;
}