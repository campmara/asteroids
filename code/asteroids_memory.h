#ifndef ASTEROIDS_MEMORY_H
#define ASTEROIDS_MEMORY_H

struct MemoryArena
{
    memsize size;
    memsize used_size;

    uint32 temporary_memory_count;

    uint8 *location;
};

// NOTE(mara): Structure for per-frame needed temporary memory.
// We will run a check at the beginning of each frame to make sure
// that we have called EndTemporaryMemory for every BeginTemporaryMemory
// in our code.
struct TemporaryMemory
{
    MemoryArena *arena;
    memsize used_size;
};

internal void InitializeArena(MemoryArena *arena, memsize size, uint8 *location)
{
    arena->size = size;
    arena->used_size = 0;
    arena->temporary_memory_count = 0;
    arena->location = location;
}

inline memsize GetAlignmentOffset(MemoryArena *arena, memsize alignment)
{
    memsize result = 0;

    memsize result_pointer = (memsize)arena->location + arena->used_size;
    memsize alignment_mask = alignment - 1;
    if (result_pointer & alignment_mask)
    {
        result = alignment - (result_pointer & alignment_mask);
    }

    return result;
}

inline memsize GetArenaSizeRemaining(MemoryArena *arena, memsize alignment = 4)
{
    return arena->size - (arena->used_size + GetAlignmentOffset(arena, alignment));
}

#define PushStruct(arena, type, ...) (type *)_PushSize(arena, sizeof(type), ## __VA_ARGS__)
#define PushArray(arena, count, type, ...) (type *)_PushSize(arena, (count) * sizeof(type), ## __VA_ARGS__)
#define PushSize(arena, size, ...) _PushSize(arena, size, ## __VA_ARGS__)
void *_PushSize(MemoryArena *arena, memsize size_init, memsize alignment = 4)
{
    memsize size = size_init;

    memsize alignment_offset = GetAlignmentOffset(arena, alignment);
    size += alignment_offset;

    Assert((arena->used_size + size) <= arena->size);
    void *result = arena->location + arena->used_size + alignment_offset;
    arena->used_size += size;

    Assert(size >= size_init);

    return result;
}

inline TemporaryMemory BeginTemporaryMemory(MemoryArena *arena)
{
    TemporaryMemory result;

    result.arena = arena;
    result.used_size = arena->used_size;

    ++arena->temporary_memory_count;

    return result;
}

inline void EndTemporaryMemory(TemporaryMemory temp_mem)
{
    MemoryArena *arena = temp_mem.arena;
    Assert(arena->used_size >= temp_mem.used_size);
    arena->used_size = temp_mem.used_size;
    Assert(arena->temporary_memory_count > 0);

    --arena->temporary_memory_count;
}

inline void CheckArenaForLingeringTemporaryMemory(MemoryArena *arena)
{
    Assert(arena->temporary_memory_count == 0);
}

#endif
