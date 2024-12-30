#pragma once
#include "Core.Definition.h"
#include "Core.Mathf.h"

struct CameraComponent
{
    Mathf::Vector3      position{ 0.0f, 0.0f, 0.0f };
    Mathf::Vector3      startPosition{ 0.0f, 1.0f, -14.0f };
    Mathf::Quaternion   rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
    Mathf::Matrix       viewMatrix{};
    Mathf::Matrix       projMatrix{};
    float               fov{ XM_PIDIV4 };
    float               aspectRatio{ 1.0f };
    float               nearZ{ 0.1f };
    float               farZ{ 1000.0f };
    float               moveSpeed{ 3.0f };
    float               xRotation{ 0 };
    float               yRotation{ 0 };

    CameraComponent()
    {
        DirectX::XMStoreFloat4x4(&viewMatrix, DirectX::XMMatrixIdentity());
        DirectX::XMStoreFloat4x4(&projMatrix, DirectX::XMMatrixIdentity());
    }
};
