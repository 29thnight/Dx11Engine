#pragma once
#include "Core.Definition.h"
#include "MemoryPool.h"
#include <concepts>

template<typename T>
concept Pointer = 
std::is_pointer_v<T> || 
std::is_null_pointer_v<T> || 
std::derived_from<T, IUnknown> ||
requires { typename T::is_segmented_pointer; };

namespace Memory
{
    //�޸� �Ҵ� �� ����
    inline void AllocateAndCopy(void* pDst, const void* pSrc, uint32 size)
    {
        pDst = malloc(size);
        memcpy(pDst, pSrc, size);
    }

    //�޸� ���� : IUnknown�� ��ӹ��� Ŭ������ ��� Release() ȣ�� -> �� �� delete ȣ��
    void SafeDelete(Pointer auto& ptr)
    {
		if constexpr (std::is_pointer_v<decltype(ptr)>)
		{
			if constexpr (std::derived_from<decltype(ptr), IUnknown>)
			{
				ptr->Release();
			}
			else if constexpr (requires T::is_segmented_pointer)
			{
                if (ptr.get())
                {
					ptr->~T();
					ptr.getPool()->deallocate(ptr.get());
					ptr.reset();
                }
			}
			else
			{
				delete ptr;
				ptr = nullptr;
			}
		}
    }
}
//���� ������ : �����̳ʿ� ����� ���(������)���� �������� ����� �����ϴ� Ŭ���� -> function ��ü�� ����Ͽ� ���� ������ ������ �� ����
template<typename T, typename Container>
class DeferredDeleter final
{
public:
    using Func = std::function<bool(T*)>;
public:
    DeferredDeleter() = default;
    DeferredDeleter(Container* container, Func func = [](T* ptr) { return true; }) : _container(container), m_deleteElementFunc(func) {}
    ~DeferredDeleter()
    {
        for (auto& ptr : *_container)
        {
            if (m_deleteElementFunc(ptr))
            {
                Memory::SafeDelete(ptr);
            }
        }

        std::erase_if(*_container, [](T* ptr) { return ptr == nullptr; });
    }

    void operator()(Container* container)
    {
        _container = container;
    }

private:
    Container* _container{};
    Func m_deleteElementFunc{};
};
//���ø� �߷� ���̵�
template<typename T, typename Allocator>
DeferredDeleter(std::vector<T*, Allocator>*, auto) -> DeferredDeleter<T, std::vector<T*, Allocator>>;

template<typename T, typename Allocator>
DeferredDeleter(std::vector<T*, Allocator>*) -> DeferredDeleter<T, std::vector<T*, Allocator>>;

template <typename T>
class SegmentedPointer
{
private:
    MemoryPool* pool;
    void* rawPointer;

public:
	using is_segment_pointer = std::true_type;

public:
    SegmentedPointer() : pool(nullptr), rawPointer(nullptr) {}
    // ������: �޸� Ǯ���� �޸𸮸� �Ҵ��ϰ�, Placement New�� ������ ȣ��
    template <typename... Args>
    SegmentedPointer(MemoryPool* pool, Args&&... args)
        : pool(pool)
    {
        rawPointer = pool->allocate(sizeof(T));
        new (rawPointer) T(std::forward<Args>(args)...); // Placement New
    }

    // �̵� ������
    SegmentedPointer(SegmentedPointer&& other) noexcept : pool(other.pool), rawPointer(other.rawPointer)
    {
        other.rawPointer = nullptr;
        other.pool = nullptr;
    }

    // �̵� �Ҵ� ������
    SegmentedPointer& operator=(SegmentedPointer&& other) noexcept
    {
        if (this != &other)
        {
            if (rawPointer)
            {
                static_cast<T*>(rawPointer)->~T();
                pool->deallocate(rawPointer);
            }

            pool = other.pool;
            rawPointer = other.rawPointer;

            other.rawPointer = nullptr;
            other.pool = nullptr;
        }
        return *this;
    }

    // ���� ������ �� �Ҵ� ������ ����
    SegmentedPointer(const SegmentedPointer&) = delete;
    SegmentedPointer& operator=(const SegmentedPointer&) = delete;

	MemoryPool* getPool() const
	{
		return pool;
	}

    // reset �Լ� �߰�
    void reset() 
    {
        rawPointer = nullptr;
        pool = nullptr;
    }

    // ��ü�� ���� ����
    T* get() const
    {
        return static_cast<T*>(rawPointer);
    }

    T& operator*() const
    {
        return *get();
    }

    T* operator->() const
    {
        return get();
    }
};

template <typename T, typename... Args>
inline SegmentedPointer<T> make_segmented(MemoryPool* pool, Args&&... args)
{
    return SegmentedPointer<T>(pool, std::forward<Args>(args)...);
}
