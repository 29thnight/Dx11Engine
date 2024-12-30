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
            continue; // ���� ���׸�Ʈ�� �̵�
        }
    }

    // ��� ���׸�Ʈ�� ���� -> compact ȣ��
    compact();

    // compact ���� �ٽ� �õ�
    for (Segment& segment : segments) {
        try {
            return segment.allocate(size);
        }
        catch (const std::bad_alloc&) {
            continue;
        }
    }

    throw std::bad_alloc(); // compact ���Ŀ��� �����ϸ� ���� �߻�
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
