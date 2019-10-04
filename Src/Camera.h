#pragma once

class Camera
{
public:
	virtual void Init(float aspectRatio) = 0;
	virtual void Update(float dt, WPARAM mouseBtnState, POINT mouseDelta) = 0;
	DirectX::XMFLOAT4 GetPosition();
	DirectX::XMFLOAT4 GetUp();
	DirectX::XMFLOAT4 GetRight();
	DirectX::XMFLOAT4 GetLook();
	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjectionMatrix();
	DirectX::XMFLOAT4X4 GetViewProjectionMatrix();

protected:
	void UpdateViewMatrix();

protected:
	DirectX::XMFLOAT4 m_position;
	DirectX::XMFLOAT4 m_right;
	DirectX::XMFLOAT4 m_up;
	DirectX::XMFLOAT4 m_look;

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
	void Rotate(float dx, float dy);
	void Zoom(float d);
	void SphericalToViewBasis();

private:
	float m_azimuth;
	float m_elev;
	float m_radius;
	float m_worldUpDir;
};
