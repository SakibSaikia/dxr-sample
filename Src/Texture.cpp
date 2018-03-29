#include "stdafx.h"
#include "Texture.h"

void Texture::Init(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, const std::string& name)
{
	m_name = name;

	// Convert UTF-8 string to wchar (See: http://forums.codeguru.com/showthread.php?t=231165 )
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &name[0], static_cast<int>(name.size()), nullptr, 0);
	std::wstring wideName(name.size(), 0);
	MultiByteToWideChar(CP_UTF8, 0, &name[0], static_cast<int>(name.size()), &wideName[0], size_needed);
	std::wstring filePath = L"..\\Content\\Sponza\\textures\\Compressed\\" + wideName + L".dds";

	DX_VERIFY(DirectX::CreateDDSTextureFromFile(
		device,
		resourceUpload,
		filePath.c_str(),
		m_resource.ReleaseAndGetAddressOf()
	));
}

D3D12_GPU_DESCRIPTOR_HANDLE Texture::CreateDescriptor(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, const size_t offsetInHeap, const size_t descriptorSize) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHnd;
	cpuHnd.ptr = srvHeap->GetCPUDescriptorHandleForHeapStart().ptr + offsetInHeap * descriptorSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_resource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = m_resource->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	device->CreateShaderResourceView(m_resource.Get(), &srvDesc, cpuHnd);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHnd;
	gpuHnd.ptr = srvHeap->GetGPUDescriptorHandleForHeapStart().ptr + offsetInHeap * descriptorSize;
	
	return gpuHnd;
}

ID3D12Resource* Texture::GetResource() const
{
	return m_resource.Get();
}

const std::string& Texture::GetName() const
{
	return m_name;
}