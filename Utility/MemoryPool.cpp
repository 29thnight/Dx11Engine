#include "MemoryPool.h"

MemoryPool::MemoryPool(const std::vector<size_t>& segmentSizes)
{
    for (size_t size : segmentSizes)
    {
        segments.emplace_back(size);
    }
}

void* MemoryPool::allocate(size_t size)
{
    for (Segment& segment : segments) {
        try {
            return segment.allocate(size);
        }
        catch (const std::bad_alloc&) {
            continue; // 다음 세그먼트로 이동
        }
    }

    // 모든 세그먼트가 실패 -> compact 호출
    compact();

    // compact 이후 다시 시도
    for (Segment& segment : segments) {
        try {
            return segment.allocate(size);
        }
        catch (const std::bad_alloc&) {
            continue;
        }
    }

    throw std::bad_alloc(); // compact 이후에도 실패하면 예외 발생
}

void MemoryPool::deallocate(void* ptr)
{
    for (Segment& segment : segments)
    {
        try
        {
            segment.deallocate(ptr);
            return;
        }
        catch (const std::invalid_argument&)
        {
            continue; // Try next segment
        }
    }
    throw std::invalid_argument("Pointer does not belong to any segment.");
}

void MemoryPool::compact()
{
    for (Segment& segment : segments)
    {
        segment.compact();
    }
}
