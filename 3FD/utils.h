#ifndef UTILS_H // header guard
#define UTILS_H

#include "base.h"
#include "exceptions.h"

#include <map>
#include <stack>
#include <queue>
#include <vector>
#include <memory>
#include <mutex>
#include <future>
#include <condition_variable>
#include <functional>
#include <cinttypes>

#ifdef _MSC_VER
#   include <allocators>
#endif

namespace _3fd
{
	namespace utils
	{
#ifdef _MSC_VER
		_ALLOCATOR_DECL(CACHE_CHUNKLIST, stdext::allocators::sync_none, unsafe_pool_allocator);
#endif

		/// <summary>
		/// Implements an event for thread synchronization making 
		/// use of a lightweight mutex and a condition variable.
		/// </summary>
		class Event
		{
		private:

			std::mutex m_mutex;
			std::condition_variable	m_condition;
			bool m_flag;

		public:

			Event();
			~Event();

			void Signalize();

			void Reset();

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

		/// <summary>
		/// Provides uninitialized and contiguous memory.
		/// There is a limit 4 GB, which is far more than enough.
		/// The pool was designed for single-thread access.
		/// </summary>
		class MemoryPool : notcopiable
		{
		private:

			void	*m_baseAddr,
					*m_nextAddr,
					*m_end;

			const uint32_t m_blockSize;

			/// <summary>
			/// Keeps available memory blocks as offset integers to the base address.
			/// Because the offset is a 32 bit integer, this imposes the practical limit of 4 GB to the pool.
			/// </summary>
			std::stack<uint32_t> m_availableAddrsAsOffset;

		public:

			MemoryPool(uint32_t numBlocks, uint32_t blockSize);

			MemoryPool(MemoryPool &&ob);

			~MemoryPool();

			size_t GetNumBlocks() const throw();

			void *GetBaseAddress() const throw();

			bool Contains(void *addr) const throw();

			bool IsFull() const throw();

			bool IsEmpty() const throw();

			void *GetFreeBlock() throw();

			void ReturnBlock(void *addr);
		};

		/// <summary>
		/// A template class for a memory pool that expands dynamically.
		/// The pool was designed for single-thread access.
		/// </summary>
		class DynamicMemPool : notcopiable
		{
		private:

			const float		m_increasingFactor;
			const size_t	m_initialSize,
							m_blockSize;

			std::map<void *, MemoryPool>	m_memPools;
			std::queue<MemoryPool *>		m_availableMemPools;

		public:

			DynamicMemPool(size_t initialSize,
				size_t blockSize,
				float increasingFactor);

			void *GetFreeBlock();

			void ReturnBlock(void *object);

			void Shrink();
		};

	}// end of namespace utils
}// end of namespace _3fd

#endif // end of header guard
