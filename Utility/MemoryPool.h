#pragma once
#include "Segment.h"
#include <vector>

class MemoryPool
{
private:
    std::vector<Segment> segments;

public:
    explicit MemoryPool(const std::vector<size_t>& segmentSizes);

    void* allocate(size_t size);
    void deallocate(void* ptr);
    void compact();

};
