#pragma once
#include "BaseType.h"

namespace TT
{
	extern "C" {
		void TranscodeETC2_to_RGBA8(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
		void TranscodeETC2_EAC_to_RGBA8(const uint8* source, uint8* dest, const uint32 width, const uint32 height);
		void TranscodeETC2_EAC_to_RGBA81(const uint8* source, uint8* dest, const uint32 width, const uint32 height);

		//void TranscodeETC2_EAC_to_RGBA4();

		//void TranscodeETC2_to_BC1();
		//void TranscodeETC2_EAC_to_BC3();

		//void TranscodeETC2_to_PVRTC();
	}
}