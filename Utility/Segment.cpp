#include "Segment.h"
#include "SpinLock.h"

Segment::Segment(size_t size)
    : totalSize(size), allocatedSize(0)
{
    memoryBlock = std::malloc(size);
    if (!memoryBlock)
    {
        throw std::bad_alloc();
    }
    nextPosPointer = static_cast<char*>(memoryBlock);
}

Segment::~Segment()
{
    std::free(memoryBlock);
}

void* Segment::allocate(size_t size)
{
    // SpinLock lock(lockFlag);

    if (allocatedSize + size > totalSize)
    {
        throw std::bad_alloc();
    }

    size_t index;
    if (!freeIndices.empty())
    {
        index = freeIndices.back();
        freeIndices.pop_back();
    }
    else
    {
        index = indexMap.size();
    }

    // Placement new
    void* result = nextPosPointer;
    nextPosPointer += size;
    allocatedSize += size;

    indexMap[result] = index;
    reverseIndexMap[index] = result;

    return result;
}

void Segment::deallocate(void* ptr)
{
    //SpinLock lock(lockFlag);
    if (indexMap.find(ptr) == indexMap.end())
    {
        throw std::invalid_argument("Invalid pointer deallocation.");
    }

    size_t index = indexMap[ptr];
    freeIndices.push_back(index);
    indexMap.erase(ptr);
    reverseIndexMap.erase(index);
}

void Segment::compact()
{
    // SpinLock lock(lockFlag);
    char* compactedPointer = static_cast<char*>(memoryBlock);
    std::unordered_map<size_t, void*> newReverseIndexMap;

    for (auto& entry : indexMap)
    {
        void* oldPtr = entry.first;
        size_t index = entry.second;
        size_t blockSize = nextPosPointer - static_cast<char*>(oldPtr);

        std::memmove(compactedPointer, oldPtr, blockSize);
        newReverseIndexMap[index] = compactedPointer;

        indexMap[compactedPointer] = index;
        compactedPointer += blockSize;
    }

    reverseIndexMap = std::move(newReverseIndexMap);
    nextPosPointer = compactedPointer;
    allocatedSize = compactedPointer - static_cast<char*>(memoryBlock);
}

size_t Segment::getIndex(void* ptr) const
{
    // SpinLock lock(lockFlag);
    if (indexMap.find(ptr) == indexMap.end())
    {
        throw std::invalid_argument("Pointer not found.");
    }
    return indexMap.at(ptr);
}

void* Segment::getPointer(size_t index) const
{
    // SpinLock lock(lockFlag);
    if (reverseIndexMap.find(index) == reverseIndexMap.end())
    {
        throw std::invalid_argument("Invalid index.");
    }
    return reverseIndexMap.at(index);
}
