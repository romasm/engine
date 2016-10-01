#pragma once

#define _DELETE(p)		{ if(p){delete (p); (p)=nullptr;} }
#define _DELETE_ARRAY(p)	{ if(p){delete[] (p); (p)=nullptr;} }
#define _RELEASE(p)		{ if(p){(p)->Release(); (p)=nullptr;} }
#define _CLOSE(p)		{ if(p){(p)->Close(); delete (p); (p)=nullptr;} }

#define ALIGNED_ALLOCATION void* operator new(size_t i){return _aligned_malloc(i,16);}\
					void operator delete(void* p){_aligned_free(p);}

