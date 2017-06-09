#pragma once

#include "Log.h"
#include "macros.h"
#include "DataTypes.h"

#include <stdint.h>

namespace EngineCore
{
	template<typename T>
	// Queue for multithreading, for simple data types only
	class RQueueLockfree
	{
	public:
		RQueueLockfree(size_t size)
		{
			dataArray = nullptr;

			if( size < 2 || size & (size - 1) != 0 )
			{
				ERR("RQueueLockfree size must be power of 2 and 2 at least!");
				return;
			}

			dataMask = size - 1;
			dataArray = new QueueElement[size];

			for (size_t i = 0; i < size; ++i)
				dataArray[i].sequence.store(i, std::memory_order_relaxed);

			posLast.store(0, std::memory_order_relaxed);
			posFirst.store(0, std::memory_order_relaxed);
		}

		~RQueueLockfree()
		{
			_DELETE_ARRAY(dataArray);
		}

		bool push(T const& data)
		{
			QueueElement* queueElement;
			size_t pos = posLast.load(std::memory_order_relaxed);
			while(true)
			{
				queueElement = &dataArray[pos & dataMask];
				size_t sequence = queueElement->sequence.load(std::memory_order_acquire);
				intptr_t diff = (intptr_t)seq - (intptr_t)pos;
				if( diff == 0 )
				{
					if (posLast.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
						break;
				}
				else if( diff < 0 )
					return false;
				else
					pos = posLast.load(std::memory_order_relaxed);
			}
			queueElement->data = data;
			queueElement->sequence.store(pos + 1, std::memory_order_release);
			return true;
		}

		bool pop(T& data)
		{
			QueueElement* queueElement;
			size_t pos = posFirst.load(std::memory_order_relaxed);
			while(true)
			{
				queueElement = &dataArray[pos & dataMask];
				size_t seq = queueElement->sequence.load(std::memory_order_acquire);
				intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);
				if( diff == 0 )
				{
					if (posFirst.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
						break;
				}
				else if( diff < 0 )
					return false;
				else
					pos = posFirst.load(std::memory_order_relaxed);
			}
			data = queueElement->data;
			queueElement->sequence.store(pos + dataMask + 1, std::memory_order_release);
			return true;
		}

		// NOT ACCURATE!
		size_t count()
		{
			size_t last = posLast.load(std::memory_order_relaxed);
			size_t first = posFirst.load(std::memory_order_relaxed);
			return last > first ? last - first : 0;
		}
		
	private:
		struct QueueElement
		{
			std::atomic<size_t> sequence;
			T data;
		};

		typedef char padding[64];

		padding _p0;
		QueueElement* dataArray;
		size_t dataMask;

		padding _p1;
		std::atomic<size_t> posLast;
		padding _p2;
		std::atomic<size_t> posFirst;
		padding _p3;
	};
}
