#pragma once
#include "BaseType.h"
namespace TT
{
	inline uint8 ClampUint8(int32 x)
	{
		if (x < 0)
			x = 0;

		if (x > 255)
			x = 255;

		return x;
	}

	inline int32 Clamp(int32 x, int32 min, int32 max)
	{
		if (x < min)
			x = min;
		if (x > max)
			x = max;

		return x;
	}

	int32 extend_4to8bits(int32 x) { return (x << 4) | x; }
	int32 extend_5to8bits(int32 x) { return (x << 3) | (x >> 2); }
	int32 extend_6to8bits(int32 x) { return (x << 2) | (x >> 4); }
	int32 extend_7to8bits(int32 x) { return (x << 1) | (x >> 6); }
}