#pragma once
#include "BaseType.h"
namespace TT
{
	class ETCBlock
	{
	};


	void TranscodeETC1_to_RGB8(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
	void TranscodeETC1_to_ARGB8();
	void TranscodeETC2_to_RGB8();
	void TranscodeETC2_to_ARGB8();
	void TranscodeETC2_EAC_to_ARGB8();
	void TranscodeETC2_EAC_to_ARGB4();


	void TranscodeETC1_to_BC1();
	void TranscodeETC2_to_BC1();
	void TranscodeETC2_EAC_to_BC3();
}