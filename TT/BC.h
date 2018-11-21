#pragma once
#include "BaseType.h"

namespace TT
{
	class BC1Block
	{
	};


	void TranscodeBC1_to_RGB8(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
	void TranscodeBC1_to_ARGB8(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
	void TranscodeBC3_to_ARGB8(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
	//void TranscodeBC3_to_ARGB4(const uint8* source, uint8* dest, const uint32 width, const uint32 height);

	void TranscodeBC1_to_ATC_RGB(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
	void TranscodeBC3_to_ATC_RGBA_I(const uint8* source, uint8* dest, const uint32 width, const uint32 height);


	//void TranscodeBC1_to_ETC1(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
	//void TranscodeBC3_to_ETC2_EAC(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
}