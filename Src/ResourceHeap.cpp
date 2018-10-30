#include "stdafx.h"
#include "ResourceHeap.h"

void ResourceHeap::Init(ID3D12Device5* device, const size_t sizeInBytes)
{
	D3D12_HEAP_DESC heapDesc = {};
	heapDesc.SizeInBytes = sizeInBytes;
	heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	device->CreateHeap(&heapDesc, IID_PPV_ARGS(m_heap.GetAddressOf()));

	m_totalSize = sizeInBytes;
	m_allocatedSize = 0;
}

auto ResourceHeap::GetAlloc(const size_t sizeInBytes, const size_t alignment) -> Alloc
{
	size_t capacity = m_totalSize - m_allocatedSize;
	size_t alignedSize = (sizeInBytes + (alignment - 1)) & ~(alignment - 1);
	assert(alignedSize <= capacity && L"Resource heap is too small. Consider increasing its size!");

	size_t offset = m_allocatedSize ;
	m_allocatedSize += alignedSize;
	return offset;
}

ID3D12Heap* ResourceHeap::GetHeap() const
{
	return m_heap.Get();
}