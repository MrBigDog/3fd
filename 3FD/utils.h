#ifndef UTILS_H // header guard
#define UTILS_H

#include "exceptions.h"

#include <map>
#include <stack>
#include <queue>
#include <vector>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <future>
#include <condition_variable>
#include <functional>
#include <cinttypes>

#ifdef _3FD_PLATFORM_WINRT
#   include "utils_lockfreequeue.h"
#endif

namespace _3fd
{
namespace utils
{
    /////////////////////////////////
    // Memory Allocation Utilities
    /////////////////////////////////

    /// <summary>
    /// Instantiates for the whole process one STL memory pool for each given set of options.
    /// </summary>
    template <size_t t_maxBlocksPerChunk, size_t t_blockSizeThresholdHint>
    class StlOptimizedMemoryPool
    {
    private:

        static constexpr std::pmr::synchronized_pool_resource synchronizedPool{
            std::pmr::pool_options{ t_maxBlocksPerChunk, t_blockSizeThresholdHint }
        };

        static constexpr std::pmr::unsynchronized_pool_resource unsynchronizedPool{
            std::pmr::pool_options{ t_maxBlocksPerChunk, t_blockSizeThresholdHint }
        };

    public:

        static constexpr std::pmr::memory_resource &GetPool(bool threadSafe) NOEXCEPT
        {
            return threadSafe ? synchronizedPool : unsynchronizedPool;
        }
    };

    /// <summary>
    /// Implements a minimal allocator (STL compatible) relying on C++17 STL pools.
    ///
    /// t_maxBlocksPerChunk: The maximum number of blocks that will be allocated at once from the
    /// upstream std::pmr::memory_resource to replenish the pool. If the value is zero or is greater than
    /// an implementation-defined limit, that limit is used instead. The STL implementation may choose to
    /// use a smaller value than is specified in this field and may use different values for different pools.
    ///
    /// t_blockSizeThresholdHint: The largest allocation size that is required to be fulfilled using the
    /// pooling mechanism. Attempts to allocate a single block larger than this threshold will be allocated
    /// directly from the upstream std::pmr::memory_resource. If largest_required_pool_block is zero or is
    /// greater than an implementation-defined limit, that limit is used instead. The implementation may
    /// choose a pass-through threshold larger than specified in this field.
    /// </summary>
    template <typename Type, size_t t_maxBlocksPerChunk, size_t t_blockSizeThresholdHint, bool t_threadSafe = true>
    class StlOptimizedAllocator
    {
    private:

        static constexpr std::pmr::memory_resource &GetPool() NOEXCEPT
        {
            return StlOptimizedMemoryPool<t_maxBlocksPerChunk, t_blockSizeThresholdHint>::GetPool(t_threadSafe);
        }

    public:

        typedef Type value_type;

        // ctor not required by STL
        StlOptimizedAllocator() NOEXCEPT {}

        // converting copy constructor (no-op because nothing is copied)
        template<typename OtherType>
        StlOptimizedAllocator(const StlOptimizedAllocator<OtherType,
            t_maxBlocksPerChunk, t_blockSizeThresholdHint, t_threadSafe> &) NOEXCEPT {}

        // instances are always equivalent (fine, because there is no copy ction)
        template<typename OtherType>
        bool operator==(const StlOptimizedAllocator<OtherType,
            t_maxBlocksPerChunk, t_blockSizeThresholdHint, t_threadSafe> &) const NOEXCEPT { return true; }

        // instances are always equivalent (fine, because there is no copy ction)
        template<typename OtherType>
        bool operator!=(const StlOptimizedAllocator<OtherType,
            t_maxBlocksPerChunk, t_blockSizeThresholdHint, t_threadSafe> &) const NOEXCEPT { return false; }

        // allocates blocks of memory
        Type *allocate(const size_t numBlocks) const
        {
            if (numBlocks != 0)
            {
                return static_cast<Type *> (
                    GetPool().allocate(numBlocks * sizeof(Type))
                );
            }

            return nullptr;
        }

        // deallocates blocks of memory
        void deallocate(Type * const ptr, size_t numBlocks) const NOEXCEPT
        {
            GetPool().deallocate(ptr, numBlocks * sizeof(Type));
        }
    };

    /// <summary>
    /// Provides uninitialized and contiguous memory.
    /// There is a limit with magnitude of megabytes, which is enough if you take into consideration
    /// that <see cref="DynamicMemPool"> will use several instances of this class when it needs more memory.
    /// The pool was designed for single-thread access.
    /// </summary>
    class MemoryPool
    {
    private:

        void *m_baseAddr,
             *m_nextAddr,
             *m_end;

        const uint16_t m_blockSize;

        /// <summary>
        /// Keeps available memory blocks as offset integers to the base address.
        /// Because the offset is a 16 bit unsigned integer, this imposes a practical
        /// limit of aproximatelt [block size] x 128 KB to the pool.
        /// </summary>
        std::stack<uint16_t> m_availableAddrsAsOffset;

    public:

        MemoryPool(uint16_t numBlocks, uint16_t blockSize);

		MemoryPool(const MemoryPool &) = delete;

        MemoryPool(MemoryPool &&ob);

        ~MemoryPool();

        size_t GetNumBlocks() const NOEXCEPT;

        void *GetBaseAddress() const NOEXCEPT;

        bool Contains(void *addr) const NOEXCEPT;

        bool IsFull() const NOEXCEPT;

        bool IsEmpty() const NOEXCEPT;

        void *GetFreeBlock() NOEXCEPT;

        void ReturnBlock(void *addr);
    };

    /// <summary>
    /// A template class for a memory pool that expands dynamically.
    /// The pool was designed for single-thread access.
    /// </summary>
    class DynamicMemPool
    {
    private:

        const float    m_increasingFactor;
        const uint16_t m_initialSize,
                       m_blockSize;

#if defined(_M_X64) || defined(__amd64__)
        typedef std::map<void *, MemoryPool, std::less<void *>,
            StlOptimizedAllocator<std::map<void *, MemoryPool>::value_type, 64, 128, false>> MapOfMemoryPools;
#else
        typedef std::map<void *, MemoryPool, std::less<void *>,
            StlOptimizedAllocator<std::pair<void *, MemoryPool>::value_type, 64, 64, false>> MapOfMemoryPools;
#endif
        MapOfMemoryPools m_memPools;
        std::queue<MemoryPool *> m_availableMemPools;

    public:

        DynamicMemPool(uint16_t initialSize,
                       uint16_t blockSize,
                       float increasingFactor);

		DynamicMemPool(const DynamicMemPool &) = delete;

        void *GetFreeBlock();

        void ReturnBlock(void *object);

        void Shrink();
    };


    ////////////////////////////////////////////////
    // Multi-thread and Synchronization Utilities
    ////////////////////////////////////////////////

    /// <summary>
    /// Implements an event for thread synchronization making 
    /// use of a lightweight mutex and a condition variable.
    /// </summary>
    class Event
    {
    private:

        std::mutex m_mutex;
        std::condition_variable m_condition;
        bool m_flag;

    public:

        Event();

        void Signalize();

        void Wait(const std::function<bool()> &predicate);

        bool WaitFor(unsigned long millisecs);
    };

    /// <summary>
    /// Provides helpers for asynchronous callbacks.
    /// </summary>
    class Asynchronous
    {
    public:

        static void InvokeAndLeave(const std::function<void()> &callback);
    };

#   ifdef _WIN32

    /// <summary>
    /// Alternative implementation for std::shared_mutex from C++14 Std library.
    /// </summary>
    class shared_mutex
    {
    private:

        enum LockType : short { None, Shared, Exclusive };

        SRWLOCK m_srwLockHandle;
        LockType m_curLockType;

    public:

        shared_mutex();
        ~shared_mutex();

		shared_mutex(const shared_mutex &) = delete;

        void lock_shared();
        void unlock_shared();

        void lock();
        void unlock();
    };

#   endif

}// end of namespace utils
}// end of namespace _3fd

#endif // end of header guard
