#pragma once

class Camera
{
public:
	virtual void Init(float aspectRatio) = 0;
	virtual void Update(float dt, WPARAM mouseBtnState, POINT mouseDelta) = 0;
	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjectionMatrix();
	DirectX::XMFLOAT4X4 GetViewProjectionMatrix();

protected:
	void UpdateViewMatrix();

protected:
	DirectX::XMFLOAT3 m_position = { 0.f, 0.f, 0.f };
	DirectX::XMFLOAT3 m_right = { 1.f, 0.f, 0.f };
	DirectX::XMFLOAT3 m_up = { 0.f, 1.f, 0.f };
	DirectX::XMFLOAT3 m_look = { 0.f, 0.f, 1.f };

	bool m_viewDirty = false;
	DirectX::XMFLOAT4X4 m_viewMatrix;
	DirectX::XMFLOAT4X4 m_projMatrix;
	DirectX::XMFLOAT4X4 m_viewProjMatrix;
};

class FirstPersonCamera : public Camera
{
public:
	void Init(float aspectRatio) override;
	void Update(float dt, WPARAM mouseBtnState, POINT mouseDelta) override;

private:
	void Strafe(float d);
	void Walk(float d);
	void Pitch(float angle);
	void RotateWorldY(float angle);
};

class TrackballCamera : public Camera
{
public:
	void Init(float aspectRatio) override;
	void Update(float dt, WPARAM mouseBtnState, POINT mouseDelta) override;

private:
	void Rotate(float d);
	void Zoom(float d);
};
