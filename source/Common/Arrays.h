#pragma once

#include "macros.h"
#include "DataTypes.h"
#include <stdint.h>

namespace EngineCore
{
	// -------------------------------------------
	template <typename T>
	// dynamic array
	class DArray
	{
	public:
		typedef T* iterator;
		typedef const T* const_iterator;

		DArray(size_t inital_capasity = 0, size_t inital_size = 0)
		{
			m_capacity = inital_capasity;
			m_size = min(inital_size, m_capacity);
			if (m_capacity > 0) m_data = new T[m_capacity];
			else m_data = nullptr;
		}

		~DArray() {destroy();}

		inline T& operator[] (const size_t i ) {return m_data[i];}
		inline const T& operator[] (const size_t i ) const {return m_data[i];}

		inline size_t size() const {return m_size;}

		inline void resize( size_t N )
		{
			if(m_capacity < N) grow( N );
			m_size = N;
		}

		inline bool empty() const {return m_size == 0 ? true : false;}
		inline bool full() const {return m_size == m_capacity ? true : false;}

		inline void push_back( const T& D )
		{
			if (m_capacity == m_size) grow( m_size + 1 );
			m_data[ m_size ] = D;
			++m_size;
		}
		inline T& push_back( )
		{
			if (m_capacity == m_size) grow( m_size + 1 );
			return m_data[ m_size++ ];
		}

		inline void pop_back() {if (m_size > 0) --m_size;}

		inline const T& back() const {return m_data[ m_size - 1 ];}
		inline T& back() {return m_data[ m_size - 1 ];}

		inline const T& front() const {return m_data[ 0 ];}
		inline T& front() {return m_data[ 0 ];}

		inline void assign( T val, size_t start = 0, size_t end = 0 )
		{
			if(end == 0)
				end = m_size;
			for ( size_t i = start; i < end; i++ )
				m_data[i] = val;
		}

		// NOT free mem
		inline void	clear() {m_size = 0;}

		// same as ~DArray(), array become dead until reserve()
		inline void	destroy()
		{
			deallocate();
			m_capacity = 0;
			m_size = 0;	
		}

		inline void	erase_and_pop_back( size_t i )
		{
			m_size--;
			std::swap(m_data[i], m_data[m_size]);
		}
		inline void	erase_and_pop_back( iterator it )
		{
			m_size--;
			std::swap((*it), m_data[m_size]);
		}
		inline void	erase_and_pop_back( const T& val ) 
		{
			iterator it = find( val );
			if(it != end()) erase_and_pop_back( it );
		}
		inline iterator	erase( iterator i )
		{
			iterator it = i;
			iterator last = end()-1;
			while(it < last)
			{
				*it = *(it+1);
				++it;
			}
			--m_size;
			return i;
		}

		inline iterator find( const T& val )
		{
			iterator it = begin();
			iterator fin = end();
			while(it != fin)
			{
				if ((*it) == val)
				{
					return it;
				}
				++it;
			}
			return end();
		}

		inline iterator push_back_unique( const T& val )
		{
			if(find( val ) == end())
			{
				push_back( val );
				return end() - 1;
			}
			return end();
		}

		inline T* data() {return m_data;}

		inline iterator begin() {return m_data;}
		inline const_iterator begin() const {return m_data;}

		inline iterator end() {return m_data + m_size;}
		inline const_iterator end() const {return m_data + m_size;}

		inline void	reserve( size_t N )
		{
			if (N <= m_capacity)
				return;
			
			T* nData = allocate_and_copy( N );
			deallocate();
			m_data = nData;
			m_capacity = N;
		}

		inline size_t capacity() const {return m_capacity;}

		inline void swap(DArray<T>& arr)
		{
			auto t_size = m_size;
			auto t_data = m_data;
			auto t_capacity = m_capacity;

			m_size = arr.m_size;
			m_data = arr.m_data;
			m_capacity = arr.m_capacity;

			arr.m_size = t_size;
			arr.m_data = t_data;
			arr.m_capacity = t_capacity;
		}		
		
	private:
		inline T* allocate_and_copy( size_t N )
		{
			T* nData = new T[N];
			T* res = nData;
			T* old_end = m_data + m_size;
			T* it = m_data;

			while(it != old_end)
				*nData++ = *it++;
			return res;
		}

		inline size_t calc_size( size_t N ) {return max<size_t>(N, m_size + m_size / 2 );}
		inline void	grow( size_t N ) {reserve( calc_size( N ) );}

		inline void	deallocate() {_DELETE_ARRAY(m_data);}

		size_t	m_size;
		T*	m_data;
		size_t	m_capacity;
	};

	template <typename T>
	// dynamic array
	class DArrayAligned : public DArray<T>
	{
		ALIGNED_ALLOCATION
	};

	// -------------------------------------------
	template <typename T, size_t S>
	// static array
	class SArray
	{
	public:
		typedef T* iterator;
		typedef const T* const_iterator;

		SArray(size_t inital_size = 0) {m_size = min(inital_size, S);}

		inline T& operator[] (const size_t i ) {return m_data[i];}
		inline const T& operator[] (const size_t i ) const {return m_data[i];}

		inline size_t size() const {return m_size;}

		inline void resize( size_t N ) {m_size = min(N, S);}

		inline bool empty() const {return m_size == 0 ? true : false;}
		inline bool full() const {return m_size == S ? true : false;}

		inline void push_back( const T& D )
		{
			if(S == m_size) return;
			m_data[ m_size ] = D;
			++m_size;
		}
		inline T* push_back( )
		{
			if(S == m_size) return nullptr;
			return &m_data[ m_size++ ];
		}

		inline void pop_back() {if (m_size > 0) --m_size;}

		inline const T& back() const {return m_data[ m_size - 1 ];}
		inline T& back() {return m_data[ m_size - 1 ];}

		inline const T& front() const {return m_data[ 0 ];}
		inline T& front() {return m_data[ 0 ];}

		inline void assign( T val, size_t start = 0, size_t end = 0 )
		{
			if(end == 0)
				end = m_size;
			for ( size_t i = start; i < end; i++ )
				m_data[i] = val;
		}

		// NOT free mem
		inline void	clear() {m_size = 0;}
		
		inline void	erase_and_pop_back( size_t i )
		{
			m_size--;
			std::swap(m_data[i], m_data[m_size]);
		}
		inline void	erase_and_pop_back( iterator it )
		{
			m_size--;
			std::swap((*it), m_data[m_size]);
		}
		inline void	erase_and_pop_back( const T& val ) 
		{
			iterator it = find( val );
			if(it != end()) erase_and_pop_back( it );
		}
		inline iterator	erase( iterator i )
		{
			iterator it = i;
			iterator last = end()-1;
			while(it < last)
			{
				*it = *(it+1);
				++it;
			}
			--m_size;
			return i;
		}

		inline iterator find( const T& val )
		{
			iterator it = begin();
			iterator fin = end();
			while(it != fin)
			{
				if ((*it) == val)
				{
					return it;
				}
				++it;
			}
			return end();
		}

		inline iterator push_back_unique( const T& val )
		{
			if(find( val ) == end())
			{
				push_back( val );
				return end() - 1;
			}
			return end();
		}

		inline T* data() {return m_data;}

		inline iterator begin() {return m_data;}
		inline const_iterator begin() const {return m_data;}

		inline iterator end() {return m_data + m_size;}
		inline const_iterator end() const {return m_data + m_size;}
		
		inline size_t capacity() const {return S;}
		
	private:
		size_t	m_size;
		T	m_data[S];
	};

	template <typename T, size_t S>
	// dynamic array
	class SArrayAligned : public SArray<T, S>
	{
		ALIGNED_ALLOCATION
	};

	// -------------------------------------------
	template <typename T>
	// runtime static array
	class RArray
	{
	public:
		typedef T* iterator;
		typedef const T* const_iterator;

		RArray()
		{
			m_capacity = 0;
			m_size = 0;
			m_data = nullptr;
		}

		RArray(size_t max_size)	{create(max_size);}
		~RArray() {destroy();}

		inline void create(size_t max_size)
		{
			if(m_data)return;
			m_capacity = max_size;
			m_size = 0;
			if (m_capacity > 0) m_data = new T[m_capacity];
			else m_data = nullptr;
		}

		inline T& operator[] (const size_t i ) {return m_data[i];}
		inline const T& operator[] (const size_t i ) const {return m_data[i];}

		inline size_t size() const {return m_size;}

		inline void resize( size_t N ) {m_size = min(N, m_capacity);}

		inline bool empty() const {return m_size == 0 ? true : false;}
		inline bool full() const {return m_size == m_capacity ? true : false;}

		inline void push_back( const T& D )
		{
			if(m_capacity == m_size) return;
			m_data[ m_size ] = D;
			++m_size;
		}
		inline T* push_back( )
		{
			if(m_capacity == m_size) return nullptr;
			return &m_data[ m_size++ ];
		}

		inline void pop_back() {if (m_size > 0) --m_size;}

		inline const T& back() const {return m_data[ m_size - 1 ];}
		inline T& back() {return m_data[ m_size - 1 ];}

		inline const T& front() const {return m_data[ 0 ];}
		inline T& front() {return m_data[ 0 ];}

		inline void assign( T val, size_t start = 0, size_t end = 0 )
		{
			if(end == 0)
				end = m_size;
			for ( size_t i = start; i < end; i++ )
				m_data[i] = val;
		}

		// NOT free mem
		inline void	clear() {m_size = 0;}

		// same as ~RArray(), array become dead until create
		inline void	destroy()
		{
			deallocate();
			m_capacity = 0;
			m_size = 0;	
		}

		inline void	erase_and_pop_back( size_t i )
		{
			m_size--;
			std::swap(m_data[i], m_data[m_size]);
		}
		inline void	erase_and_pop_back( iterator it )
		{
			m_size--;
			std::swap((*it), m_data[m_size]);
		}
		inline void	erase_and_pop_back( const T& val ) 
		{
			iterator it = find( val );
			if(it != end()) erase_and_pop_back( it );
		}
		inline iterator	erase( iterator i )
		{
			iterator it = i;
			iterator last = end()-1;
			while(it < last)
			{
				*it = *(it+1);
				++it;
			}
			--m_size;
			return i;
		}

		inline iterator find( const T& val )
		{
			iterator it = begin();
			iterator fin = end();
			while(it != fin)
			{
				if ((*it) == val)
				{
					return it;
				}
				++it;
			}
			return end();
		}

		inline iterator push_back_unique( const T& val )
		{
			if(find( val ) == end())
			{
				push_back( val );
				return end() - 1;
			}
			return end();
		}

		inline T* data() {return m_data;}

		inline iterator begin() {return m_data;}
		inline const_iterator begin() const {return m_data;}

		inline iterator end() {return m_data + m_size;}
		inline const_iterator end() const {return m_data + m_size;}
		
		inline size_t capacity() const {return m_capacity;}

		inline void swap(RArray<T>& arr)
		{
			auto t_size = m_size;
			auto t_data = m_data;
			auto t_capacity = m_capacity;

			m_size = arr.m_size;
			m_data = arr.m_data;
			m_capacity = arr.m_capacity;

			arr.m_size = t_size;
			arr.m_data = t_data;
			arr.m_capacity = t_capacity;
		}
		
	private:
		inline void	deallocate() {_DELETE_ARRAY(m_data);}

		size_t	m_size;
		T*	m_data;
		size_t	m_capacity;
	};
	
	template <typename T>
	// dynamic array
	class RArrayAligned : public RArray<T>
	{
		ALIGNED_ALLOCATION
	};

	// -------------------------------------------
	template <typename T, size_t S>
	// static deque
	class SDeque
	{
	public:
		SDeque(size_t inital_size = 0)
		{
			m_size = min(inital_size, S);
			m_begin = 0;
		}

		inline T& operator[] (const size_t i ) {return m_data[getIdx(i)];}
		inline const T& operator[] (const size_t i ) const {return m_data[getIdx(i)];}

		inline size_t size() const {return m_size;}

		inline void resize( size_t N ) {m_size = min(N, S);}

		inline bool empty() const {return m_size == 0 ? true : false;}
		inline bool full() const {return m_size == S ? true : false;}

		inline void push_back( const T& D )
		{
			m_data[ getIdx(m_size) ] = D;
			if(S == m_size)
			{
				if(m_begin == S-1) m_begin = 0;
				else m_begin++;
			}
			else m_size++;
		}
		inline T& push_back( )
		{
			T& res = m_data[ getIdx(m_size) ];
			if(S == m_size)
			{
				if(m_begin == S-1) m_begin = 0;
				else m_begin++;
			}			
			else m_size++;
			return res;
		}

		inline void pop_back()
		{
			if (m_size > 0) --m_size;
		}

		inline void push_front( const T& D )
		{
			if(m_begin == 0) m_begin = S-1;
			else m_begin--;
			m_data[ m_begin ] = D;
			if(S > m_size) m_size++;
		}
		inline T& push_front( )
		{
			if(m_begin == 0) m_begin = S-1;
			else m_begin--;
			T& res = m_data[ m_begin ];
			if(S > m_size) m_size++;
			return res;
		}

		inline void pop_front()
		{
			if (m_size > 0)
			{
				--m_size;
				if(m_begin == S-1) m_begin = 0;
				else m_begin++;
			}
		}

		inline const T& back() const {return m_data[ getIdx(m_size-1) ];}
		inline T& back() {return m_data[ getIdx(m_size-1) ];}

		inline size_t back_idx() {return getIdx(m_size-1);}

		inline const T& front() const {return m_data[ m_begin ];}
		inline T& front() {return m_data[ m_begin ];}

		inline size_t front_idx() {return m_begin;}

		inline void assign( T val, size_t start = 0, size_t end = 0 )
		{
			if(end == 0)
				end = m_size;
			for ( size_t i = start; i < end; i++ )
				m_data[getIdx(i)] = val;
		}

		inline size_t find( const T& val )
		{
			for ( size_t i = 0; i < m_size; i++ )
				if (m_data[getIdx(i)] == val)
					return i;
			return S;
		}

		inline void push_back_unique( const T& val )
		{
			if(find( val ) == S)
				push_back( val );
		}

		inline void push_front_unique( const T& val )
		{
			if(find( val ) == S)
				push_front( val );
		}

		// NOT free mem
		inline void	clear()
		{
			m_begin = 0; 
			m_size = 0;
		}
		
		inline void	erase_and_pop_back( size_t i )
		{
			m_size--;
			std::swap(m_data[getIdx(i)], m_data[getIdx(m_size)]);
		}
		inline void	erase_and_pop_back( const T& val ) 
		{
			size_t it = find( val );
			if(it != S) erase_and_pop_back( it );
		}
		inline void	erase( size_t i )
		{
			if(m_size == 0) return;
			for ( size_t j = i; j < m_size-1; j++ )
				m_data[getIdx(j)] = m_data[getIdx(j+1)];
			--m_size;
		}

		inline T* data() {return m_data;}

		inline size_t capacity() const {return S;}

	private:
		inline size_t getIdx(size_t i)
		{
			size_t idx = i + m_begin;
			if(idx >= S)
				idx -= S;
			return idx;
		}

		size_t	m_begin;
		size_t	m_size;
		T	m_data[S];
	};

	// -------------------------------------------
	template <typename T>
	// runtime deque
	class RDeque
	{
	public:
		RDeque()
		{
			m_capacity = 0;
			m_size = 0;
			m_begin = 0;
			m_data = nullptr;
		}

		RDeque(size_t max_size)	{create(max_size);}
		~RDeque() {destroy();}

		inline void create(size_t max_size)
		{
			if(m_data)return;
			m_capacity = max_size;
			m_size = 0;
			if (m_capacity > 0) m_data = new T[m_capacity];
			else m_data = nullptr;
		}

		inline T& operator[] (const size_t i ) {return m_data[getIdx(i)];}
		inline const T& operator[] (const size_t i ) const {return m_data[getIdx(i)];}

		inline size_t size() const {return m_size;}

		inline void resize( size_t N ) {m_size = min(N, m_capacity);}

		inline bool empty() const {return m_size == 0 ? true : false;}
		inline bool full() const {return m_size == m_capacity ? true : false;}

		inline void push_back( const T& D )
		{
			m_data[ getIdx(m_size) ] = D;
			if(m_capacity == m_size)
			{
				if(m_begin == m_capacity - 1) m_begin = 0;
				else m_begin++;
			}
			else m_size++;
		}
		inline T& push_back( )
		{
			T& res = m_data[ getIdx(m_size) ];
			if(m_capacity == m_size)
			{
				if(m_begin == m_capacity - 1) m_begin = 0;
				else m_begin++;
			}			
			else m_size++;
			return res;
		}

		inline void pop_back()
		{
			if (m_size > 0) --m_size;
		}

		inline void push_front( const T& D )
		{
			if(m_begin == 0) m_begin = m_capacity - 1;
			else m_begin--;
			m_data[ m_begin ] = D;
			if(m_capacity > m_size) m_size++;
		}
		inline T& push_front( )
		{
			if(m_begin == 0) m_begin = m_capacity - 1;
			else m_begin--;
			T& res = m_data[ m_begin ];
			if(m_capacity > m_size) m_size++;
			return res;
		}

		inline void pop_front()
		{
			if (m_size > 0)
			{
				--m_size;
				if(m_begin == m_capacity - 1) m_begin = 0;
				else m_begin++;
			}
		}

		inline const T& back() const {return m_data[ getIdx(m_size-1) ];}
		inline T& back() {return m_data[ getIdx(m_size-1) ];}

		inline size_t back_idx() {return getIdx(m_size-1);}

		inline const T& front() const {return m_data[ m_begin ];}
		inline T& front() {return m_data[ m_begin ];}

		inline size_t front_idx() {return m_begin;}

		inline void assign( T val, size_t start = 0, size_t end = 0 )
		{
			if(end == 0)
				end = m_size;
			for ( size_t i = start; i < end; i++ )
				m_data[getIdx(i)] = val;
		}

		inline size_t find( const T& val )
		{
			for ( size_t i = 0; i < m_size; i++ )
				if (m_data[getIdx(i)] == val)
					return i;
			return m_capacity;
		}

		inline void push_back_unique( const T& val )
		{
			if(find( val ) == m_capacity)
				push_back( val );
		}

		inline void push_front_unique( const T& val )
		{
			if(find( val ) == m_capacity)
				push_front( val );
		}

		// NOT free mem
		inline void	clear()
		{
			m_begin = 0; 
			m_size = 0;
		}
		
		// same as ~RDeque(), array become dead until create
		inline void	destroy()
		{
			deallocate();
			m_capacity = 0;
			m_size = 0;	
			m_begin = 0;
		}
		
		inline void	erase_and_pop_back( size_t i )
		{
			m_size--;
			std::swap(m_data[getIdx(i)], m_data[getIdx(m_size)]);
		}
		inline void	erase_and_pop_back( const T& val ) 
		{
			size_t it = find( val );
			if(it != m_capacity) erase_and_pop_back( it );
		}
		inline void	erase( size_t i )
		{
			if(m_size == 0) return;
			for ( size_t j = i; j < m_size-1; j++ )
				m_data[getIdx(j)] = m_data[getIdx(j+1)];
			--m_size;
		}

		inline T* data() {return m_data;}

		inline size_t capacity() const {return m_capacity;}

	private:
		inline void	deallocate() {_DELETE_ARRAY(m_data);}

		inline size_t getIdx(size_t i)
		{
			size_t idx = i + m_begin;
			if(idx >= m_capacity)
				idx -= m_capacity;
			return idx;
		}

		size_t	m_begin;
		size_t	m_size;
		T*	m_data;
		size_t	m_capacity;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////
	// COMPONENTS /////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////
	
	template <typename T>
	// for components only! static indirect array: ids with holes, packed data
	class ComponentRArray
	{
	public:
		ComponentRArray(){}

		void create(size_t max_size)
		{
			m_data.create(max_size);
			m_lookup.create(max_size);
			m_lookup.resize(max_size);
			m_lookup.assign(max_size);
		}
		
		inline size_t getArrayIdx(const size_t i) {return m_lookup[i];}
		inline T& getDataByArrayIdx(const size_t i) {return m_data[i];}
		inline T& getDataById(const size_t i) {return m_data[m_lookup[i]];}

		inline size_t dataSize() const {return m_data.size();}

		inline bool empty() const {return m_data.size() == 0 ? true : false;}
		inline bool full() const {return m_data.size() == m_data.capacity() ? true : false;}

		inline bool has(const size_t i) const {return i < m_data.capacity() && m_lookup[i] != m_data.capacity();}

		inline T* add(const size_t i)
		{
			if(i >= m_data.capacity()) return nullptr;
			if(m_lookup[i] == m_data.capacity())
			{
				m_lookup[i] = m_data.size();
				return m_data.push_back();
			}
			else
			{
				T* res = &m_data[m_lookup[i]];
				(*res) = T();
				return res;
			}
		}

		inline void add(const size_t i, T D)
		{
			if(i >= m_data.capacity()) return;
			if(m_lookup[i] == m_data.capacity())
			{
				m_lookup[i] = m_data.size();
				m_data.push_back(D);
			}
			else m_data[m_lookup[i]] = D;
		}

		inline void remove(const size_t i)
		{
			if(i >= m_data.capacity()) return;
			size_t rmv_id = m_lookup[i];
			if(rmv_id == m_data.capacity()) return;
			
			if(rmv_id != m_data.size() - 1)
			{
				size_t old_id = m_data.size() - 1;
				m_lookup[m_data[old_id].get_id()] = rmv_id;
			}
			m_data.erase_and_pop_back(rmv_id);
			m_lookup[i] = m_data.capacity();
		}

		// NOT free mem
		inline void	clear()
		{
			m_data.clear();
			m_lookup.assign(m_data.capacity());
		}

		inline RArray<T>* data() {return &m_data;}
		inline RArray<size_t>* lookupTable() {return &m_lookup;}

		inline size_t capacity() const {return m_data.capacity();}

	private:
		RArray<T> m_data;
		RArray<size_t> m_lookup;
	};

	// -------------------------------------------
	template <typename T>
	// for multiple components only! static indirect array: ids with holes, packed data
	class MultiComponentRArray
	{
	public:
		MultiComponentRArray() {}

		void create(size_t max_size, size_t max_data)
		{
			m_data.create(max_data);
			m_lookup.create(max_size);
			m_lookup.resize(max_size);
			m_lookup.assign(max_data);
		}
		
		inline size_t getArrayIdx(const size_t i) {return m_lookup[i];}
		inline T& getDataByArrayIdx(const size_t i) {return m_data[i];}

		inline T* getDataById(const size_t i, const size_t num = 0)
		{
			size_t idx = m_lookup[i];
			if(idx == m_data.capacity()) return nullptr;
			T* res = &m_data[idx];
			for(size_t j=0; j<num; j++)
			{
				if(res->next == (uint32_t)m_data.capacity()) return nullptr;
				res = &m_data[res->next];
			}
			return res;
		}

		inline size_t dataSize() const {return m_data.size();}

		inline bool empty() const {return m_data.size() == 0 ? true : false;}
		inline bool full() const {return m_data.size() == m_data.capacity() ? true : false;}

		inline bool has(const size_t i) const {return i < m_lookup.capacity() && m_lookup[i] != m_data.capacity();}

		inline T* add(const size_t i)
		{
			if(i >= m_lookup.capacity()) return nullptr;
			if(m_data.size() >= m_data.capacity()) return nullptr;
			if(m_lookup[i] == m_data.capacity())
			{
				m_lookup[i] = m_data.size();
				T* res = m_data.push_back();
				res->prev = (uint32_t)m_data.capacity();
				res->next = (uint32_t)m_data.capacity();
				res->num = 0;
				return res;
			}
			else
			{
				size_t idx = m_lookup[i];
				T* res = &m_data[idx];
				T* prev = res;
				uint num = 0;
				while(prev->next != (uint32_t)m_data.capacity())
				{
					idx = prev->next;
					prev = &m_data[idx];
					num++;
				}
				prev->next = (uint)m_data.size();
				res = m_data.push_back();
				res->prev = (uint)idx;
				res->next = (uint32_t)m_data.capacity();
				res->num = num + 1;
				return res;
			}
		}

		inline void add(const size_t i, T data)
		{
			T* res = add(i);
			(*res) = data;
		}

		inline uint remove(const size_t i)
		{
			uint removed = 0;

			if(i >= m_lookup.capacity()) return removed;
			size_t rmv_id = m_lookup[i];
			if(rmv_id == m_data.capacity()) return removed;
			
			while(rmv_id != m_data.capacity())
			{
				size_t t_next = m_data[rmv_id].next;

				if(rmv_id != m_data.size() - 1)
				{
					size_t old_id = m_data.size() - 1;
					T* alien_data = &m_data[old_id];
					if(alien_data->prev == (uint32_t)m_data.capacity())
						m_lookup[alien_data->get_id()] = rmv_id;
					else
						m_data[alien_data->prev].next = (uint)rmv_id;
					if(alien_data->next != (uint32_t)m_data.capacity())
						m_data[alien_data->next].prev = (uint)rmv_id;
				}

				if(m_data[rmv_id].num == 0) m_lookup[i] = m_data.capacity();
				m_data.erase_and_pop_back(rmv_id);
				
				if(t_next != m_data.size()) // else: deleted item points to last that was moved
					rmv_id = t_next;
				removed++;
			}
			return removed;
		}

		// NOT free mem
		inline void	clear()
		{
			m_data.clear();
			m_lookup.assign(m_data.capacity());
		}

		inline RArray<T>* data() {return &m_data;}
		inline RArray<size_t>* lookupTable() {return &m_lookup;}

		inline size_t capacity() const {return m_data.capacity();}

	private:
		RArray<T> m_data;
		RArray<size_t> m_lookup;
	};

	// -------------------------------------------
	template <typename T>
	// for components only! static indirect array with dynamic data: ids with holes, packed data
	class ComponentRDArray
	{
	public:
		ComponentRDArray() {}

		void create(size_t max_size)
		{
			m_lookup.create(max_size);
			m_lookup.resize(max_size);
			m_lookup.assign(max_size);
		}
		
		inline void reserve(size_t N) {m_data.reserve(min(m_lookup.capacity(), N));}

		inline size_t getArrayIdx(const size_t i) {return m_lookup[i];}
		inline T& getDataByArrayIdx(const size_t i) {return m_data[i];}
		inline T& getDataById(const size_t i) {return m_data[m_lookup[i]];}

		inline size_t dataSize() const {return m_data.size();}

		inline bool empty() const {return m_data.size() == 0 ? true : false;}
		inline bool full() const {return m_data.size() == m_lookup.capacity() ? true : false;}

		inline bool has(const size_t i) const {return i < m_lookup.capacity() && m_lookup[i] != m_lookup.capacity();}

		inline T* add(const size_t i)
		{
			if(i >= m_lookup.capacity()) return nullptr;
			if(m_lookup[i] == m_lookup.capacity())
			{
				m_lookup[i] = m_data.size();
				return &m_data.push_back();
			}
			else
			{
				T* res = &m_data[m_lookup[i]];
				(*res) = T();
				return res;
			}
		}

		inline void add(const size_t i, T D)
		{
			if(i >= m_lookup.capacity()) return;
			if(m_lookup[i] == m_lookup.capacity())
			{
				m_lookup[i] = m_data.size();
				m_data.push_back(D);
			}
			else m_data[m_lookup[i]] = D;
		}

		inline void remove(const size_t i)
		{
			if(i >= m_lookup.capacity()) return;
			size_t rmv_id = m_lookup[i];
			if(rmv_id == m_lookup.capacity()) return;
			
			if(rmv_id != m_data.size() - 1)
			{
				size_t old_id = m_data.size() - 1;
				m_lookup[m_data[old_id].get_id()] = rmv_id;
			}
			m_data.erase_and_pop_back(rmv_id);
			m_lookup[i] = m_lookup.capacity();
		}

		// NOT free mem
		inline void	clear()
		{
			m_data.clear();
			m_lookup.assign(m_lookup.capacity());
		}

		inline DArray<T>* data() {return &m_data;}
		inline RArray<size_t>* lookupTable() {return &m_lookup;}

		inline size_t capacity() const {return m_lookup.capacity();}

	private:
		DArray<T> m_data;
		RArray<size_t> m_lookup;
	};

}
