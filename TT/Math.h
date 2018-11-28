#pragma once
#include "BaseType.h"
namespace TT
{
	inline uint32 Max(uint32 a, uint32 b)
	{
		return a > b ? a : b;
	}

	inline uint8 ClampUint8(int32 n)
	{
		if (n < 0)
			return 0;

		if (n > 255)
			return 255;

		return n;

		//after test above is faster......
		//https://codereview.stackexchange.com/questions/6502/fastest-way-to-clamp-an-integer-to-the-range-0-255
		//n &= -(n >= 0);
		//return n | ((255 - n) >> 31);
	}

	inline uint8 ClampUint8Left(int32 n)
	{
		if (n < 0)
			return 0;

		return n;
	}

	inline uint8 ClampUint8Right(int32 n)
	{
		if (n > 255)
			return 255;

		return n;
	}

	inline int32 Clamp(int32 n, int32 min, int32 max)
	{
		if (n < min)
			n = min;
		if (n > max)
			n = max;

		return n;
	}

	inline int32 extend_4to8bits(int32 n) { return (n << 4) | n; }
	inline int32 extend_5to8bits(int32 n) { return (n << 3) | (n >> 2); }
	inline int32 extend_6to8bits(int32 n) { return (n << 2) | (n >> 4); }
	inline int32 extend_7to8bits(int32 n) { return (n << 1) | (n >> 6); }
}