#pragma once

#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <cstdlib>
#include <atomic>

class Segment
{
private:
    void* memoryBlock;
    size_t totalSize;
    size_t allocatedSize;
    char* nextPosPointer;
    std::unordered_map<void*, size_t> indexMap;
    std::unordered_map<size_t, void*> reverseIndexMap;
    std::vector<size_t> freeIndices;

public:
    explicit Segment(size_t size);
    ~Segment();

    void* allocate(size_t size);
    void deallocate(void* ptr);
    void compact();

    size_t getIndex(void* ptr) const;
    void* getPointer(size_t index) const;
};
