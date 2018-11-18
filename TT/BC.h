#pragma once
namespace TT
{
	class BC1Block
	{
	};


	void TranscodeBC1_to_RGB8();
	void TranscodeBC1_to_ARGB8();
	void TranscodeBC3_to_ARGB8();
	void TranscodeBC3_to_ARGB4();


	void TranscodeBC1_to_ETC1();
	void TranscodeBC3_to_ETC2_EAC();
}