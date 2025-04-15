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

#endif
