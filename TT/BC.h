#pragma once
#include "BaseType.h"

namespace TT
{
	extern "C" {
		class BC1Block
		{
		};


		void TranscodeBC1_to_RGBA8(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
		void TranscodeBC3_to_RGB8A(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
		//void TranscodeBC3_to_RGBA4(const uint8* source, uint8* dest, const uint32 width, const uint32 height);


		//https://www.khronos.org/registry/OpenGL/extensions/AMD/AMD_compressed_ATC_texture.txt
		//ATC_RGB_AMD                        0x8C92
		void TranscodeBC1_to_ATC_RGB(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
		//ATC_RGBA_INTERPOLATED_ALPHA_AMD    0x87EE
		void TranscodeBC3_to_ATC_RGBA(const uint8* source, uint8* dest, const uint32 width, const uint32 height);


		//void TranscodeBC1_to_ETC1(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
		//void TranscodeBC3_to_ETC2_EAC(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
	}
}