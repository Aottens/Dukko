#include "AllocationGuard.h"
#include <cstdlib>
#include <new>

namespace
{
thread_local bool armed = false;
thread_local int allocations = 0;

void* allocate (std::size_t size, std::size_t align)
{
    if (armed)
        ++allocations;
    if (size == 0)
        size = 1;
    if (align <= alignof (std::max_align_t))
        return std::malloc (size);
    // ponytail: aligned_alloc is POSIX; use _aligned_malloc/_aligned_free when the tests build on Windows.
    return std::aligned_alloc (align, (size + align - 1) / align * align);
}

void* allocateOrThrow (std::size_t size, std::size_t align)
{
    if (void* p = allocate (size, align))
        return p;
    throw std::bad_alloc();
}
}

AllocationGuard::AllocationGuard() { allocations = 0; armed = true; }
AllocationGuard::~AllocationGuard() { armed = false; }
int AllocationGuard::count() const { return allocations; }

void* operator new (std::size_t n) { return allocateOrThrow (n, 0); }
void* operator new[] (std::size_t n) { return allocateOrThrow (n, 0); }
void* operator new (std::size_t n, const std::nothrow_t&) noexcept { return allocate (n, 0); }
void* operator new[] (std::size_t n, const std::nothrow_t&) noexcept { return allocate (n, 0); }
void* operator new (std::size_t n, std::align_val_t a) { return allocateOrThrow (n, (std::size_t) a); }
void* operator new[] (std::size_t n, std::align_val_t a) { return allocateOrThrow (n, (std::size_t) a); }
void* operator new (std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept { return allocate (n, (std::size_t) a); }
void* operator new[] (std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept { return allocate (n, (std::size_t) a); }

void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }
void operator delete (void* p, const std::nothrow_t&) noexcept { std::free (p); }
void operator delete[] (void* p, const std::nothrow_t&) noexcept { std::free (p); }
void operator delete (void* p, std::align_val_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::align_val_t) noexcept { std::free (p); }
void operator delete (void* p, std::size_t, std::align_val_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t, std::align_val_t) noexcept { std::free (p); }
void operator delete (void* p, std::align_val_t, const std::nothrow_t&) noexcept { std::free (p); }
void operator delete[] (void* p, std::align_val_t, const std::nothrow_t&) noexcept { std::free (p); }
