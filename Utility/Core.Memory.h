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
    //메모리 할당 및 복사
    inline void AllocateAndCopy(void* pDst, const void* pSrc, uint32 size)
    {
        pDst = malloc(size);
        memcpy(pDst, pSrc, size);
    }

    //메모리 해제 : IUnknown을 상속받은 클래스의 경우 Release() 호출 -> 그 외 delete 호출
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
//지연 삭제자 : 컨테이너에 저장된 요소(포인터)들을 스코프가 벗어나면 삭제하는 클래스 -> function 객체를 사용하여 삭제 조건을 지정할 수 있음
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
//템플릿 추론 가이드
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
    // 생성자: 메모리 풀에서 메모리를 할당하고, Placement New로 생성자 호출
    template <typename... Args>
    SegmentedPointer(MemoryPool* pool, Args&&... args)
        : pool(pool)
    {
        rawPointer = pool->allocate(sizeof(T));
        new (rawPointer) T(std::forward<Args>(args)...); // Placement New
    }

    // 이동 생성자
    SegmentedPointer(SegmentedPointer&& other) noexcept : pool(other.pool), rawPointer(other.rawPointer)
    {
        other.rawPointer = nullptr;
        other.pool = nullptr;
    }

    // 이동 할당 연산자
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

    // 복사 생성자 및 할당 연산자 삭제
    SegmentedPointer(const SegmentedPointer&) = delete;
    SegmentedPointer& operator=(const SegmentedPointer&) = delete;

	MemoryPool* getPool() const
	{
		return pool;
	}

    // reset 함수 추가
    void reset() 
    {
        rawPointer = nullptr;
        pool = nullptr;
    }

    // 객체에 대한 접근
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
