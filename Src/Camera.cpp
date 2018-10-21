#include "stdafx.h"
#include "Camera.h"

void Camera::UpdateViewMatrix()
{
	if (m_viewDirty)
	{
		DirectX::XMVECTOR R = DirectX::XMLoadFloat3(&m_right);
		DirectX::XMVECTOR U = DirectX::XMLoadFloat3(&m_up);
		DirectX::XMVECTOR L = DirectX::XMLoadFloat3(&m_look);
		DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&m_position);

		L = DirectX::XMVector3Normalize(L);
		U = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(L, R));
		R = DirectX::XMVector3Cross(U, L);

		// v = inv(T) * transpose(R)
		float x = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, R));
		float y = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, U));
		float z = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(P, L));

		DirectX::XMStoreFloat3(&m_right, R);
		DirectX::XMStoreFloat3(&m_up, U);
		DirectX::XMStoreFloat3(&m_look, L);

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
	DirectX::XMVECTOR r = DirectX::XMLoadFloat3(&m_right);
	DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&m_position);
	DirectX::XMStoreFloat3(&m_position, DirectX::XMVectorMultiplyAdd(s, r, p));

	m_viewDirty = true;
}

void FirstPersonCamera::Walk(float d)
{
	// m_position += d*m_look
	DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
	DirectX::XMVECTOR l = DirectX::XMLoadFloat3(&m_look);
	DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&m_position);
	DirectX::XMStoreFloat3(&m_position, DirectX::XMVectorMultiplyAdd(s, l, p));

	m_viewDirty = true;
}

void FirstPersonCamera::Pitch(float angle)
{
	// Rotate up and look vector about the right vector.
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&m_right), angle);

	DirectX::XMVECTOR u = DirectX::XMLoadFloat3(&m_up);
	DirectX::XMVECTOR l = DirectX::XMLoadFloat3(&m_look);
	DirectX::XMStoreFloat3(&m_up, DirectX::XMVector3TransformNormal(u, R));
	DirectX::XMStoreFloat3(&m_look, DirectX::XMVector3TransformNormal(l, R));

	m_viewDirty = true;
}

void FirstPersonCamera::RotateWorldY(float angle)
{
	// Rotate the basis vectors about the world y-axis.
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(angle);

	DirectX::XMVECTOR r = DirectX::XMLoadFloat3(&m_right);
	DirectX::XMVECTOR u = DirectX::XMLoadFloat3(&m_up);
	DirectX::XMVECTOR l = DirectX::XMLoadFloat3(&m_look);
	DirectX::XMStoreFloat3(&m_right, DirectX::XMVector3TransformNormal(r, R));
	DirectX::XMStoreFloat3(&m_up, DirectX::XMVector3TransformNormal(u, R));
	DirectX::XMStoreFloat3(&m_look, DirectX::XMVector3TransformNormal(l, R));

	m_viewDirty = true;
}

void FirstPersonCamera::Init(const float aspectRatio)
{
	DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(0.25f * 3.1415926535f, aspectRatio, 1.0f, 10000.0f);
	XMStoreFloat4x4(&m_projMatrix, proj);
}

void FirstPersonCamera::Update(const float dt, const POINT mouseDelta)
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