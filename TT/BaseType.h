#pragma once
#include <string>
namespace TT
{
	typedef unsigned char       uint8;
	typedef char		        int8;
	typedef unsigned short      uint16;
	typedef short               int16;
	typedef unsigned int        uint32;
	typedef int                 int32;

#ifdef __EMSCRIPTEN__
	typedef unsigned long long	uint64;
	typedef long long			int64;
#else
	typedef unsigned __int64	uint64;
	typedef __int64				int64;
#endif
}