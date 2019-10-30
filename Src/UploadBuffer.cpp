#include "stdafx.h"
#include "App.h"
#include "UploadBuffer.h"

UploadBuffer::~UploadBuffer()
{
	m_buffer->Unmap(0, nullptr);
}

ID3D12Resource* UploadBuffer::GetResource() const
{
	return m_buffer.Get();
}

void UploadBuffer::Init(ID3D12Device5* device, const size_t sizeInBytes)
{
	D3D12_HEAP_PROPERTIES heapDesc = {};
	heapDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN; // must be unknown for Dimension == BUFFER
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	CHECK(device->CreateCommittedResource(
		&heapDesc,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // required starting state for upload heaps
		nullptr,
		IID_PPV_ARGS(m_buffer.GetAddressOf())
	));

	// keep it mapped
	m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_dataPtr));

	m_totalSize = sizeInBytes;
	m_allocatedSize = 0;
}

auto UploadBuffer::GetAlloc(const size_t sizeInBytes, const size_t alignment) -> Alloc
{
	size_t capacity = m_totalSize - m_allocatedSize;
	size_t alignedSize = (sizeInBytes + (alignment - 1)) & ~(alignment - 1);

	if (alignedSize > capacity)
	{
		Flush();

		capacity = m_totalSize - m_allocatedSize;
		assert(alignedSize <= capacity && L"Upload buffer is too small. Consider increasing its size!");
	}

	uint8_t* allocPtr = m_dataPtr + m_allocatedSize;
	size_t offset = m_allocatedSize;
	m_allocatedSize += alignedSize;

	return std::make_pair(allocPtr, offset);

}

void UploadBuffer::Flush()
{
	AppInstance()->SubmitCommands();
	AppInstance()->FlushCmdQueue();

	// reset to beginning
	m_allocatedSize = 0;
}

