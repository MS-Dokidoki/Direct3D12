#include "Camera.h"

using namespace DirectX;

Camera::Camera() {}
Camera::~Camera() {}

void Camera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp)
{
	vec3Right = {1.0f, 0.0f, 0.0f};

	XMStoreFloat3(&vec3Position, pos);
	XMStoreFloat3(&vec3Look, (target - pos));
	XMStoreFloat3(&vec3Up, worldUp);

	bViewDirty = 1;
}

void Camera::LookAt(const XMFLOAT3 &pos, const XMFLOAT3 &target, const XMFLOAT3 &worldUp)
{
	vec3Right = {1.0f, 0.0f, 0.0f};
	vec3Position = pos;
	vec3Look = {target.x - pos.x, target.y - pos.y, target.z - pos.z};
	vec3Up = worldUp;

	bViewDirty = 1;
}

void Camera::SetLens(float fovY, float aspect, float nearZ, float farZ)
{
	fFovY = fovY;
	fAspect = aspect;
	fFarZ = farZ;
	fNearZ = nearZ;

	fNearWindowHeight = 2.0f * fNearZ * tanf(0.5f * fFovY);
	fFarWindowHeight = 2.0f * fFarZ * tanf(0.5f * fFovY);

	XMMATRIX P = XMMatrixPerspectiveFovLH(fFovY, fAspect, fNearZ, fFarZ);
	XMStoreFloat4x4(&matProj, P);
}

void Camera::Walk(float d)
{
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR l = XMLoadFloat3(&vec3Look);
	XMVECTOR p = XMLoadFloat3(&vec3Position);
	XMStoreFloat3(&vec3Position, XMVectorMultiplyAdd(s, l, p));

	bViewDirty = 1;
}

void Camera::Strafe(float d)
{
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR r = XMLoadFloat3(&vec3Right);
	XMVECTOR p = XMLoadFloat3(&vec3Position);
	XMStoreFloat3(&vec3Position, XMVectorMultiplyAdd(s, r, p));

	bViewDirty = 1;
}

void Camera::Pitch(float angle)
{
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&vec3Right), angle);

	XMStoreFloat3(&vec3Up, XMVector3TransformNormal(XMLoadFloat3(&vec3Up), R));
	XMStoreFloat3(&vec3Look, XMVector3TransformNormal(XMLoadFloat3(&vec3Look), R));

	bViewDirty = 1;
}

void Camera::RotateY(float angle)
{
	XMMATRIX R = XMMatrixRotationY(angle);

	XMStoreFloat3(&vec3Right, XMVector3TransformNormal(XMLoadFloat3(&vec3Right), R));
	XMStoreFloat3(&vec3Up, XMVector3TransformNormal(XMLoadFloat3(&vec3Up), R));
	XMStoreFloat3(&vec3Look, XMVector3TransformNormal(XMLoadFloat3(&vec3Look), R));

	bViewDirty = 1;
}

void Camera::Roll(float angle)
{
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&vec3Look), angle);
	
	XMStoreFloat3(&vec3Up, XMVector3TransformNormal(XMLoadFloat3(&vec3Up), R));
	XMStoreFloat3(&vec3Right, XMVector3TransformNormal(XMLoadFloat3(&vec3Right), R));
	
	bViewDirty = 1;
}

void Camera::UpdateViewMatrix()
{
	if (bViewDirty)
	{
		XMVECTOR p = XMLoadFloat3(&vec3Position);
		XMVECTOR l = XMLoadFloat3(&vec3Look);
		XMVECTOR u = XMLoadFloat3(&vec3Up);
		XMVECTOR r = XMLoadFloat3(&vec3Right);

		l = XMVector3Normalize(l);
		r = XMVector3Normalize(XMVector3Cross(u, l));
		u = XMVector3Cross(l, r);

		XMStoreFloat3(&vec3Up, u);
		XMStoreFloat3(&vec3Right, r);
		XMStoreFloat3(&vec3Look, l);

		float x = -XMVectorGetX(XMVector3Dot(p, r));
		float y = -XMVectorGetX(XMVector3Dot(p, u));
		float z = -XMVectorGetX(XMVector3Dot(p, l));

		matView(0, 0) = vec3Right.x;
		matView(1, 0) = vec3Right.y;
		matView(2, 0) = vec3Right.z;
		matView(3, 0) = x;

		matView(0, 1) = vec3Up.x;
		matView(1, 1) = vec3Up.y;
		matView(2, 1) = vec3Up.z;
		matView(3, 1) = y;

		matView(0, 2) = vec3Look.x;
		matView(1, 2) = vec3Look.y;
		matView(2, 2) = vec3Look.z;
		matView(3, 2) = z;

		matView(0, 3) = 0.0f;
		matView(1, 3) = 0.0f;
		matView(2, 3) = 0.0f;
		matView(3, 3) = 1.0f;

		bViewDirty = 0;
	}
}

bool Camera::GetViewDirty() const
{
	return bViewDirty;
}

XMVECTOR Camera::GetPosition() const
{
	return XMLoadFloat3(&vec3Position);
}

XMFLOAT3 Camera::GetPosition3f() const
{
	return vec3Position;
}

XMVECTOR Camera::GetRight() const
{
	return XMLoadFloat3(&vec3Right);
}

XMFLOAT3 Camera::GetRight3f() const
{
	return vec3Right;
}

XMVECTOR Camera::GetUp() const
{
	return XMLoadFloat3(&vec3Up);
}

XMFLOAT3 Camera::GetUp3f() const
{
	return vec3Up;
}

XMVECTOR Camera::GetLook() const
{
	return XMLoadFloat3(&vec3Look);
}

XMFLOAT3 Camera::GetLook3f() const
{
	return vec3Look;
}

float Camera::GetNearZ() const
{
	return fNearZ;
}

float Camera::GetFarZ() const
{
	return fFarZ;
}

float Camera::GetAspect() const
{
	return fAspect;
}

XMMATRIX Camera::GetProj() const
{
	return XMLoadFloat4x4(&matProj);
}

XMMATRIX Camera::GetView() const
{
	return XMLoadFloat4x4(&matView);
}

XMFLOAT4X4 Camera::GetProj4x4f() const
{
	return matProj;
}

XMFLOAT4X4 Camera::GetView4x4f() const
{
	return matView;
}

float Camera::GetFovX() const
{
	float halfWidth = 0.5f * GetNearWindowWidth();
	return 2.0f * atanf(halfWidth / fNearZ);
}

float Camera::GetNearWindowWidth() const
{
	return fAspect * fNearWindowHeight;
}

float Camera::GetNearWindowHeight() const
{
	return fNearWindowHeight;
}

float Camera::GetFarWindowHeight() const
{
	return fAspect * fFarWindowHeight;
}

float Camera::GetFarWindowWidth() const
{
	return fFarWindowHeight;
}
