#pragma once
#include "BaseType.h"
namespace TT
{
	class ColorRGBA8
	{
	public:
		union
		{
			uint32	dwColor;
			uint8	byColor[4];
			struct
			{
				uint8 r;		// 0x000000ff
				uint8 g;		// 0x0000ff00
				uint8 b;		// 0x00ff0000
				uint8 a;		// 0xff000000
			};
		};
	public:
		ColorRGBA8(uint32 col = 0xFFFFFFFF)
			: dwColor(col)
		{}
		ColorRGBA8(uint8 inR, uint8 inG, uint8 inB, uint8 inA)
			: r(inR)
			, g(inG)
			, b(inB)
			, a(inA)
		{}

		const ColorRGBA8& operator= (const ColorRGBA8& arg)
		{
			dwColor = arg.dwColor;
			return *this;
		}

		bool operator == (const ColorRGBA8& arg) const
		{
			return dwColor == arg.dwColor;
		}

		bool operator != (const ColorRGBA8& arg) const
		{
			return dwColor != arg.dwColor;
		}
	public:
		static const ColorRGBA8 White;
	};

	class ColorRGBA4
	{
	};


	class ColorRGB565
	{
	};
}