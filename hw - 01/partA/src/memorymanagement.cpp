#include <memorymanagement.h>

using namespace myos::common;
using namespace myos;

// Static variable to keep track of the active memory manager
MemoryManager* MemoryManager::activeMemoryManager = 0;

// Constructor for the MemoryManager class
MemoryManager::MemoryManager(size_t start, size_t size)
{
    // Set the active memory manager to this instance
    activeMemoryManager = this;

    // If the size is less than the size of a MemoryChunk, set first to 0
    if (size < sizeof(MemoryChunk))
    {
        first = 0;
    }
    else
    {
        // Initialize the first memory chunk
        first = (MemoryChunk*)start;

        first->allocated = false;
        first->prev = 0;
        first->next = 0;
        first->size = size - sizeof(MemoryChunk);
    }
}

// Destructor for the MemoryManager class
MemoryManager::~MemoryManager()
{
    // If this instance is the active memory manager, set it to 0
    if (activeMemoryManager == this)
        activeMemoryManager = 0;
}

// Allocate memory of a given size
void* MemoryManager::malloc(size_t size)
{
    MemoryChunk* result = 0;

    // Find a suitable free memory chunk
    for (MemoryChunk* chunk = first; chunk != 0 && result == 0; chunk = chunk->next)
        if (!chunk->allocated && chunk->size > size)
            result = chunk;

    // If no suitable chunk is found, return 0
    if (result == 0)
        return 0;

    // If the chunk is larger than required, split it
    if (result->size >= size + sizeof(MemoryChunk) + 1)
    {
        MemoryChunk* temp = (MemoryChunk*)((size_t)result + sizeof(MemoryChunk) + size);

        temp->allocated = false;
        temp->size = result->size - size - sizeof(MemoryChunk);
        temp->prev = result;
        temp->next = result->next;
        if (temp->next != 0)
            temp->next->prev = temp;

        result->size = size;
        result->next = temp;
    }

    // Mark the chunk as allocated
    result->allocated = true;
    return (void*)((size_t)result + sizeof(MemoryChunk));
}

// Free previously allocated memory
void MemoryManager::free(void* ptr)
{
    // Get the memory chunk from the pointer
    MemoryChunk* chunk = (MemoryChunk*)((size_t)ptr - sizeof(MemoryChunk));

    // Mark the chunk as free
    chunk->allocated = false;

    // Merge with the previous chunk if it is free
    if (chunk->prev != 0 && !chunk->prev->allocated)
    {
        chunk->prev->next = chunk->next;
        chunk->prev->size += chunk->size + sizeof(MemoryChunk);
        if (chunk->next != 0)
            chunk->next->prev = chunk->prev;

        chunk = chunk->prev;
    }

    // Merge with the next chunk if it is free
    if (chunk->next != 0 && !chunk->next->allocated)
    {
        chunk->size += chunk->next->size + sizeof(MemoryChunk);
        chunk->next = chunk->next->next;
        if (chunk->next != 0)
            chunk->next->prev = chunk;
    }
}

// Overloaded new operator for single object allocation
void* operator new(unsigned size)
{
    if (myos::MemoryManager::activeMemoryManager == 0)
        return 0;
    return myos::MemoryManager::activeMemoryManager->malloc(size);
}

// Overloaded new[] operator for array allocation
void* operator new[](unsigned size)
{
    if (myos::MemoryManager::activeMemoryManager == 0)
        return 0;
    return myos::MemoryManager::activeMemoryManager->malloc(size);
}

// Placement new operator
void* operator new(unsigned size, void* ptr)
{
    return ptr;
}

// Overloaded delete operator for single object deallocation
void operator delete(void* ptr)
{
    if (myos::MemoryManager::activeMemoryManager != 0)
        myos::MemoryManager::activeMemoryManager->free(ptr);
}

// Overloaded delete[] operator for array deallocation
void operator delete[](void* ptr)
{
    if (myos::MemoryManager::activeMemoryManager != 0)
        myos::MemoryManager::activeMemoryManager->free(ptr);
}
