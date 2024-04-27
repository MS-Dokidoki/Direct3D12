#pragma once
#include <D3DBase.h>

class Camera
{
public:
    Camera();
    ~Camera();

    bool GetViewDirty() const;

    DirectX::XMVECTOR GetPosition() const;
    DirectX::XMFLOAT3 GetPosition3f() const;
    void SetPosition(float x, float y, float z);
    void SetPosition3f(const DirectX::XMFLOAT3& v);

    DirectX::XMVECTOR GetRight() const;
    DirectX::XMFLOAT3 GetRight3f() const;
    DirectX::XMVECTOR GetUp() const;
    DirectX::XMFLOAT3 GetUp3f() const;
    DirectX::XMVECTOR GetLook() const;
    DirectX::XMFLOAT3 GetLook3f() const;

    float GetNearZ() const;
    float GetFarZ() const;
    float GetAspect() const;
    float GetFovY() const;
    float GetFovX() const;

    float GetNearWindowWidth() const;
    float GetFarWindowWidth() const;
    float GetNearWindowHeight() const;
    float GetFarWindowHeight() const;

    void SetLens(float fovY, float aspect, float nearZ, float farZ);

    void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR up);
    void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);

    DirectX::XMMATRIX GetView() const;
    DirectX::XMMATRIX GetProj() const;

    DirectX::XMFLOAT4X4 GetView4x4f() const;
    DirectX::XMFLOAT4X4 GetProj4x4f() const;

    void Strafe(float d);
    void Walk(float d);

    void Pitch(float angle);
    void RotateY(float angle);
	void Roll(float angle);
	
    void UpdateViewMatrix();

private:
    DirectX::XMFLOAT3 vec3Position = {0.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 vec3Right = {1.0f, 0.0f, 0.0f};
    DirectX::XMFLOAT3 vec3Up = {0.0f, 1.0f, 0.0f};
    DirectX::XMFLOAT3 vec3Look = {0.0f, 0.0f, 1.0f};

    float fNearZ = 0.0f, fFarZ = 0.0f, fAspect = 0.0f, fFovY = 0.0f, fNearWindowHeight = 0.0f, fFarWindowHeight = 0.0f;

    bool bViewDirty;

    DirectX::XMFLOAT4X4 matView;
    DirectX::XMFLOAT4X4 matProj;
};
