#pragma once

#define DEFAULT_ALIGNMENT (4)

class StackAllocator
{
public:
	StackAllocator() = delete;
	explicit StackAllocator(const size_t sizeInBytes) :
		m_size{sizeInBytes},
		m_current { 0 }
	{
		m_data = std::make_unique<uint8_t[]>(sizeInBytes);
	}

	template<class T>
	T* Allocate(const size_t arraySize = 1, const size_t alignment = DEFAULT_ALIGNMENT)
	{
		const size_t alignedSize = (arraySize * sizeof(T) + (alignment - 1)) & ~(alignment - 1);
		assert(m_current + alignedSize <= m_size && "Increase allocator size");

		const uint8_t* alloc = m_data.get() + m_current;
		m_current += alignedSize;

		return (T*)(alloc);
	}

	wchar_t* ConstructName(const wchar_t* name, const size_t alignment = DEFAULT_ALIGNMENT)
	{
		const size_t nameLen = wcslen(name) + sizeof('\0');
		const size_t alignedSize = (nameLen * sizeof(wchar_t) + (alignment - 1)) & ~(alignment - 1);
		assert(m_current + alignedSize <= m_size && "Increase allocator size");

		wchar_t* const alloc = reinterpret_cast<wchar_t*>(m_data.get() + m_current);
		m_current += alignedSize;

		wcscpy_s(alloc, alignedSize, name);
		return alloc;
	}

private:
	std::unique_ptr<uint8_t[]> m_data;
	size_t m_size;
	size_t m_current;
};
