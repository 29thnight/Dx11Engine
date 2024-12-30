#pragma once
#include "Core.Definition.h"
#include "DirectXHelper.h"
#include "Core.Memory.h"
#include <future>
#include <atomic>

struct Texture final
{
	Texture() = default;
	Texture(file::path path, ID3D11Device* pDevice) : m_isLoaded(false)
	{
		m_loadTexture = std::async(std::launch::async, [&]()
		{
			DirectX::CreateTextureFromFile(pDevice, path, &m_SRV);
			m_isLoaded = true;
		});
	}

	~Texture()
	{
		if (m_loadTexture.valid())
		{
			m_loadTexture.wait();
		}

		Memory::SafeDelete(m_texture2D);
	}

	operator bool() const { return m_isLoaded; }

	union
	{
		ID3D11Texture2D* m_texture2D{};
		ID3D11Texture3D* m_texture3D;
	};
	ID3D11ShaderResourceView* m_SRV{};
	ID3D11UnorderedAccessView* m_UAV{};
	uint32 m_width{};
	uint32 m_height{};
	uint32 m_levels{};
	std::future<void> m_loadTexture{};
	std::atomic_bool m_isLoaded{ false };
};