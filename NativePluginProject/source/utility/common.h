#ifndef _COMMON_H
#define _COMMON_H

#include "types.h"
#include "intrinsics.h"

#define countof(Array) (sizeof(Array) / sizeof(Array[0]))

// stringize macros
#define xstr(s) str(s)
#define str(s) #s


// Assert macros
#if defined(SLOWCHECKS) && SLOWCHECKS != 0

#ifdef SDL_assert_h_
#define assert(a)	        SDL_assert((a))
#else
#define assert(a)	        if(!(a)) {*(int *)0 = 0;}
#endif

#else

#define assert(a)

#endif


// Alignment macros, use only with pow2 alignments
#ifdef _MSC_VER
#define is_aligned(addr, bytes)     (((uintptr_t)(const void *)(addr)) % (bytes) == 0)
#else
#define is_aligned(addr, bytes)     (((uintptr_t)(addr)) % (bytes) == 0)
#endif

#define _align_mask(addr, mask)     (((uintptr_t)(addr)+(mask))&~(mask))
#define _align(addr, bytes)          _align_mask(addr,(bytes)-1)

// static assert macro for checking struct size to a base alignment when packed in an array
#define static_assert_aligned_size(Type,bytes) \
	static_assert(sizeof(Type) % (bytes) == 0, \
                  #Type " size is not a multiple of " xstr(bytes))

inline bool is_power_of_2(s32 x) { return (x > 0 && !(x & (x-1))); }

// size in bytes macros
#define kilobytes(v)    v*1024
#define megabytes(v)    kilobytes(v)*1024
#define gigabytes(v)    megabytes(v)*1024

#define bytesToMegabytes(v)     v/1048576

// export macros
#ifdef _MSC_VER
#define _export __declspec(dllexport)
#else
#define _export __attribute__ ((visibility ("default")))
#endif

// malloc macros
#if defined(ALLOW_MALLOC) && ALLOW_MALLOC != 0

#define Q_malloc(size)   malloc(size)
#define Q_free(x)        free(x);

#else

#if defined(SLOWCHECKS) && SLOWCHECKS != 0
#define Q_malloc(size)   nullptr; assert(false && "malloc not allowed")
#define Q_free(x)        assert(false && "free not allowed")
#else
// if assert is ignored, just crash
#define Q_malloc(size)   nullptr; *(volatile int*)0 = 0
#define Q_free(x)        *(volatile int*)0 = 0
#endif

#endif

// min/max functions, should avoid using macros for this due to possible double-evaluation
inline u8  min(u8 a, u8 b)   { return (a < b ? a : b); }
inline i8  min(i8 a, i8 b)   { return (a < b ? a : b); }
inline u16 min(u16 a, u16 b) { return (a < b ? a : b); }
inline i16 min(i16 a, i16 b) { return (a < b ? a : b); }
inline u32 min(u32 a, u32 b) { return (a < b ? a : b); }
inline i32 min(i32 a, i32 b) { return (a < b ? a : b); }
inline u64 min(u64 a, u64 b) { return (a < b ? a : b); }
inline i64 min(i64 a, i64 b) { return (a < b ? a : b); }
inline r32 min(r32 a, r32 b) { return (a < b ? a : b); }
inline r64 min(r64 a, r64 b) { return (a < b ? a : b); }

inline u8  max(u8 a, u8 b)   { return (a > b ? a : b); }
inline i8  max(i8 a, i8 b)   { return (a > b ? a : b); }
inline u16 max(u16 a, u16 b) { return (a > b ? a : b); }
inline i16 max(i16 a, i16 b) { return (a > b ? a : b); }
inline u32 max(u32 a, u32 b) { return (a > b ? a : b); }
inline i32 max(i32 a, i32 b) { return (a > b ? a : b); }
inline u64 max(u64 a, u64 b) { return (a > b ? a : b); }
inline i64 max(i64 a, i64 b) { return (a > b ? a : b); }
inline r32 max(r32 a, r32 b) { return (a > b ? a : b); }
inline r64 max(r64 a, r64 b) { return (a > b ? a : b); }

// c std lib macros for msvc-only *_s functions
#ifdef _MSC_VER
    #define _strcpy_s(dest,destsz,src)					strcpy_s(dest,destsz,src)
    #define _strncpy_s(dest,destsz,src,count)			strncpy_s(dest,destsz,src,count)
    #define _strcat_s(dest,destsz,src)					strcat_s(dest,destsz,src)
	#define _vsnprintf_s(dest,destsz,count,fmt,valist)	vsnprintf_s(dest,destsz,count,fmt,valist)
	#define _memcpy_s(dest,destsz,src,count)			memcpy_s(dest,destsz,src,count)
	#define _fopen_s(pFile,filename,mode)				fopen_s(pFile,filename,mode)
#else
    #define _strcpy_s(dest,destsz,src)					strcpy(dest,src)
    #define _strncpy_s(dest,destsz,src,count)			strncpy((dest),(src),min((u64)(destsz),(u64)(count)))
    #define _strcat_s(dest,destsz,src)					strcat(dest,src)
	#define _vscprintf(fmt,valist)						vsnprintf(nullptr,0,fmt,valist)
	#define _memcpy_s(dest,destsz,src,count)			memcpy(dest,src,count)
	#define _fopen_s(pFile,filename,mode)				*pFile=fopen(filename,mode)

	#define _TRUNCATE									0xFFFFFFFF
	#include <cstdarg>
	#include <cstdio>

	int _vsnprintf_s(
		char* buffer,
		size_t sizeOfBuffer,
		size_t count,
		const char* format,
   		va_list args)
	{
		if ((count != _TRUNCATE) && (count < sizeOfBuffer)) {
			sizeOfBuffer = count;
		}

		int result = vsnprintf(buffer, sizeOfBuffer, format, args);

		if ((0 <= result) && (sizeOfBuffer <= (size_t)result)) {
			result = -1;
		}

		return result;
	}
#endif


// Lock condition
enum LockCond : u8 {
	DoNotLock = 0,
	DoLock = 1
};

// spin locks

// the atomic_lock implementation is more lightweight but does not prevent thread starvation, do
// not use this under high contention from many threads
void lock_spin(atomic_lock& lock)
{
	// test_and_set returns false when the last value was false, if true is returned, another
	// thread beat us to it
	while (lock.test_and_set(std::memory_order_acquire)) {
		_mm_pause();
	}
}

void unlock(atomic_lock& lock)
{
	// set the value back to false, allowing another thread to grab the lock with test_and_set
	lock.clear(std::memory_order_release);
}

// the ticket_mutex implementation prevents thread starvation and ensures each thread acquires the
// lock in order, but this costs 4 more bytes than the atomic_lock version
void lock_spin(ticket_mutex& mutex)
{
	// claim this thread's ticket atomically
	u32 ticket = mutex.ticket.fetch_add(1, std::memory_order_relaxed);
	// wait until the ticket serving reaches this one
	while (ticket != mutex.serving.load(std::memory_order_acquire)) {
		_mm_pause();
	}
}

void unlock(ticket_mutex& mutex)
{
	mutex.serving.fetch_add(1, std::memory_order_release);
}

// conditional versions of the locks above
void lock_spin(atomic_lock& lock, LockCond _lock) {
	if (_lock == DoLock) { lock_spin(lock); }
}

void unlock(atomic_lock& lock, LockCond _lock) {
	if (_lock == DoLock) { unlock(lock); }
}

void lock_spin(ticket_mutex& mutex, LockCond _lock) {
	if (_lock == DoLock) { lock_spin(mutex); }
}

void unlock(ticket_mutex& mutex, LockCond _lock) {
	if (_lock == DoLock) { unlock(mutex); }
}

#endif