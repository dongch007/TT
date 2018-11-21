#pragma once
#include "Color.h"
namespace TT
{
	class ColorBlock
	{
	public:
		ColorBlock() {}

		ColorRGBA8 color(uint32 i) const
		{
			return m_color[i];
		}

		ColorRGBA8& color(uint32 i)
		{
			return m_color[i];
		}

		ColorRGBA8 color(uint32 x, uint32 y) const
		{
			return m_color[y * 4 + x];
		}

		ColorRGBA8& color(uint32 x, uint32 y)
		{
			return m_color[y * 4 + x];
		}

	private:
		ColorRGBA8 m_color[16];
	};
}