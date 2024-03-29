//modified from Angle's loadimage_etc
//https://github.com/google/angle/blob/master/src/image_util/loadimage_etc.cpp
//
// Copyright (c) 2013-2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "ETC.h"
#include "ColorBlock.h"
#include "Math.h"
namespace TT
{
	// Table 3.17.2 sorted according to table 3.17.3
	static const int32 intensityModifierDefault[8][4] =
	{
		{ 2,   8,  -2,   -8 },
		{ 5,  17,  -5,  -17 },
		{ 9,  29,  -9,  -29 },
		{ 13,  42, -13,  -42 },
		{ 18,  60, -18,  -60 },
		{ 24,  80, -24,  -80 },
		{ 33, 106, -33, -106 },
		{ 47, 183, -47, -183 },
	};

	// Table C.12, intensity modifier for non opaque punchthrough alpha
	static const int32 intensityModifierNonOpaque[8][4] =
	{
		{ 0,   8, 0,   -8 },
		{ 0,  17, 0,  -17 },
		{ 0,  29, 0,  -29 },
		{ 0,  42, 0,  -42 },
		{ 0,  60, 0,  -60 },
		{ 0,  80, 0,  -80 },
		{ 0, 106, 0, -106 },
		{ 0, 183, 0, -183 },
	};

/*
https://www.khronos.org/registry/OpenGL/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt
ETC1_RGB8_OES:

a) bit layout in bits 63 through 32 if diffbit = 0

 63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48
 -----------------------------------------------
| base col1 | base col2 | base col1 | base col2 |
| R1 (4bits)| R2 (4bits)| G1 (4bits)| G2 (4bits)|
 -----------------------------------------------

 47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32
 ---------------------------------------------------
| base col1 | base col2 | table  | table  |diff|flip|
| B1 (4bits)| B2 (4bits)| cw 1   | cw 2   |bit |bit |
 ---------------------------------------------------


b) bit layout in bits 63 through 32 if diffbit = 1

 63 62 61 60 59 58 57 56 55 54 53 52 51 50 49 48
 -----------------------------------------------
| base col1    | dcol 2 | base col1    | dcol 2 |
| R1' (5 bits) | dR2    | G1' (5 bits) | dG2    |
 -----------------------------------------------

 47 46 45 44 43 42 41 40 39 38 37 36 35 34  33  32
 ---------------------------------------------------
| base col 1   | dcol 2 | table  | table  |diff|flip|
| B1' (5 bits) | dB2    | cw 1   | cw 2   |bit |bit |
 ---------------------------------------------------


c) bit layout in bits 31 through 0 (in both cases)

 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
 -----------------------------------------------
|       most significant pixel index bits       |
| p| o| n| m| l| k| j| i| h| g| f| e| d| c| b| a|
 -----------------------------------------------

 15 14 13 12 11 10  9  8  7  6  5  4  3   2   1  0
 --------------------------------------------------
|         least significant pixel index bits       |
| p| o| n| m| l| k| j| i| h| g| f| e| d| c | b | a |
 --------------------------------------------------
*/
//https://www.khronos.org/registry/OpenGL/specs/es/3.0/es_spec_3.0.pdf page293

	class ETC2Block
	{
	private:
		union {
			// Individual, differential, H and T modes
			struct
			{
				union {
					// Individual and differential modes
					struct
					{
						union {
							struct  // Individual colors
							{
								uint8 R2 : 4;
								uint8 R1 : 4;
								uint8 G2 : 4;
								uint8 G1 : 4;
								uint8 B2 : 4;
								uint8 B1 : 4;
							} indiv;
							struct  // Differential colors
							{
								int8 dR : 3;
								uint8 R : 5;
								int8 dG : 3;
								uint8 G : 5;
								int8 dB : 3;
								uint8 B : 5;
							} diff;
						} colors;
						bool flipbit : 1;
						bool diffbit : 1;
						uint8 cw2 : 3;
						uint8 cw1 : 3;
					} idm;
					// T mode
					struct
					{
						// Byte 1
						uint8 TR1b : 2;
						uint8 TdummyB : 1;
						uint8 TR1a : 2;
						uint8 TdummyA : 3;
						// Byte 2
						uint8 TB1 : 4;
						uint8 TG1 : 4;
						// Byte 3
						uint8 TG2 : 4;
						uint8 TR2 : 4;
						// Byte 4
						uint8 Tdb : 1;
						bool Tflipbit : 1;
						uint8 Tda : 2;
						uint8 TB2 : 4;
					} tm;
					// H mode
					struct
					{
						// Byte 1
						uint8 HG1a : 3;
						uint8 HR1 : 4;
						uint8 HdummyA : 1;
						// Byte 2
						uint8 HB1b : 2;
						uint8 HdummyC : 1;
						uint8 HB1a : 1;
						uint8 HG1b : 1;
						uint8 HdummyB : 3;
						// Byte 3
						uint8 HG2a : 3;
						uint8 HR2 : 4;
						uint8 HB1c : 1;
						// Byte 4
						uint8 Hdb : 1;
						bool Hflipbit : 1;
						uint8 Hda : 1;
						uint8 HB2 : 4;
						uint8 HG2b : 1;
					} hm;
				} mode;
				uint8 pixelIndexMSB[2];
				uint8 pixelIndexLSB[2];
			} idht;
			// planar mode
			struct
			{
				// Byte 1
				uint8 GO1 : 1;
				uint8 RO : 6;
				uint8 PdummyA : 1;
				// Byte 2
				uint8 BO1 : 1;
				uint8 GO2 : 6;
				uint8 PdummyB : 1;
				// Byte 3
				uint8 BO3a : 2;
				uint8 PdummyD : 1;
				uint8 BO2 : 2;
				uint8 PdummyC : 3;
				// Byte 4
				uint8 RH2 : 1;
				bool Pflipbit : 1;
				uint8 RH1 : 5;
				uint8 BO3b : 1;
				// Byte 5
				uint8 BHa : 1;
				uint8 GH : 7;
				// Byte 6
				uint8 RVa : 3;
				uint8 BHb : 5;
				// Byte 7
				uint8 GVa : 5;
				uint8 RVb : 3;
				// Byte 8
				uint8 BV : 6;
				uint8 GVb : 2;
			} pblk;
			struct
			{
				uint32 part0;
				uint32 part1;
			};
		} u;

	public:
		void Decode1(uint8* dest, uint32 destRowPitch) const
		{
			// Select mode
			//uint32 diff = (u.part0 >> 24) & 0x2;
			//if (diff)
			if(u.idht.mode.idm.diffbit)
			{
				//int32  R = (u.part0 >> 3) & 0x1F;
				//int32 dR = ((u.part0 & 0x7) << 29) >> 29;
				//int32  G = (u.part0 >> 11) & 0x1F;
				//int32 dG = (((u.part0 >> 8) & 0x7) << 29) >> 29;
				//int32  B = (u.part0 >> 19) & 0x1F;
				//int32 dB = (((u.part0 >> 16) & 0x7) << 29) >> 29;
				//int32 r = (R + dR);
				//int32 g = (G + dG);
				//int32 b = (B + dB);
				const auto &diff = u.idht.mode.idm.colors.diff;
				int r = (diff.R + diff.dR);
				int g = (diff.G + diff.dG);
				int b = (diff.B + diff.dB);

				if (r < 0 || r > 31)
				{
					//DecodeTMode(dest, destRowPitch);
				}
				else if (g < 0 || g > 31)
				{
					//DecodeHMode(dest, destRowPitch);
				}
				else if (b < 0 || b > 31)
				{
					//DecodePlanarMode(dest, destRowPitch);
				}
				else
				{
					DecodeDifferentialMode(dest, destRowPitch, diff.R, diff.G, diff.B, r, g, b);
				}
			}
			else
			{
				//DecodeIndividualMode(dest, destRowPitch);
			}
		}

		void Decode(uint8* dest, uint32 destRowPitch) const
		{
			// Select mode
			uint32 diff = (u.part0 >> 24) & 0x2;
			if (diff)
			{
				int32  R = (u.part0 >> 3) & 0x1F;
				int32 dR = ((int)(u.part0 & 0x7) << 29) >> 29;
				int32  G = (u.part0 >> 11) & 0x1F;
				int32 dG = ((int)((u.part0 >> 8) & 0x7) << 29) >> 29;
				int32  B = (u.part0 >> 19) & 0x1F;
				int32 dB = ((int)((u.part0 >> 16) & 0x7) << 29) >> 29;
				int32 r = (R + dR);
				int32 g = (G + dG);
				int32 b = (B + dB);

				if (r < 0 || r > 31)
				{
					DecodeTMode(dest, destRowPitch);
				}
				else if (g < 0 || g > 31)
				{
					DecodeHMode(dest, destRowPitch);
				}
				else if (b < 0 || b > 31)
				{
					DecodePlanarMode(dest, destRowPitch);
				}
				else
				{
					DecodeDifferentialMode(dest, destRowPitch, R, G, B, r, g, b);
				}
			}
			else
			{
				DecodeIndividualMode(dest, destRowPitch);
			}
		}

		void DecodeIndividualMode(uint8* dest, uint32 destRowPitch) const
		{
			const auto &indiv = u.idht.mode.idm.colors.indiv;
			int r1 = extend_4to8bits(indiv.R1);
			int g1 = extend_4to8bits(indiv.G1);
			int b1 = extend_4to8bits(indiv.B1);
			int r2 = extend_4to8bits(indiv.R2);
			int g2 = extend_4to8bits(indiv.G2);
			int b2 = extend_4to8bits(indiv.B2);
			DecodeIndividualOrDifferentialMode(dest, destRowPitch, r1, g1, b1, r2, g2, b2);
		}

		//void DecodeIndividualMode1(uint8* dest, uint32 destRowPitch) const
		//{
		//	int r1 = extend_4to8bits((u.part0 >> 4) & 0xF);
		//	int r2 = extend_4to8bits(u.part0 & 0xF);
		//	int g1 = extend_4to8bits((u.part0 >> 12) & 0xF);
		//	int g2 = extend_4to8bits((u.part0 >> 8) & 0xF);
		//	int b1 = extend_4to8bits((u.part0 >> 20) & 0xF);
		//	int b2 = extend_4to8bits((u.part0 >> 16) & 0xF);
		//	DecodeIndividualOrDifferentialMode(dest, destRowPitch, r1, g1, b1, r2, g2, b2);
		//}

		inline void DecodeDifferentialMode(uint8* dest, uint32 destRowPitch, int R, int G, int B, int r, int g, int b) const
		{
			int r1 = extend_5to8bits(R);
			int g1 = extend_5to8bits(G);
			int b1 = extend_5to8bits(B);
			int r2 = extend_5to8bits(r);
			int g2 = extend_5to8bits(g);
			int b2 = extend_5to8bits(b);
			DecodeIndividualOrDifferentialMode(dest, destRowPitch, r1, g1, b1, r2, g2, b2);
		}

		//uint32 getIndex(uint32 x, uint32 y) const
		//{
		//	uint32 bitIndex = x * 4 + y;
		//	uint32 bitOffset = bitIndex & 7;
		//	uint32 lsb = (u.idht.pixelIndexLSB[1 - (bitIndex >> 3)] >> bitOffset) & 1;
		//	uint32 msb = (u.idht.pixelIndexMSB[1 - (bitIndex >> 3)] >> bitOffset) & 1;
		//	return (msb << 1) | lsb;
		//}
		inline uint32 getIndex(uint32 x, uint32 y) const
		{
			uint32 bitIndex = x * 4 + y;
			uint32 bitOffset = bitIndex & 7;
#ifdef __EMSCRIPTEN__
			uint32 lsb = ((u.part1 >> (16 + 8 * (1 - (bitIndex >> 3)))) >> bitOffset) & 0x1;
			uint32 msb = ((u.part1 >> (8 * (1 - (bitIndex >> 3)))) >> bitOffset) & 0x1;
			return (msb << 1) | lsb;
#else
			uint32 lsb = (u.idht.pixelIndexLSB[1 - (bitIndex >> 3)] >> bitOffset) & 1;
			uint32 msb = (u.idht.pixelIndexMSB[1 - (bitIndex >> 3)] >> bitOffset) & 1;
			return (msb << 1) | lsb;
#endif
		}

		//void DecodeIndividualOrDifferentialMode1(uint8* dest, uint32 destRowPitch, int r1, int g1, int b1, int r2, int g2, int b2) const
		//{
		//	static ColorRGBA8 subblockColors0[4];
		//	static ColorRGBA8 subblockColors1[4];
		//	for (uint32 modifierIdx = 0; modifierIdx < 4; modifierIdx++)
		//	{
		//		const int32 i1 = intensityModifierDefault[u.idht.mode.idm.cw1][modifierIdx];
		//		ColorRGBA8& c0 = subblockColors0[modifierIdx];
		//		c0.r = ClampUint8(r1 + i1);
		//		c0.g = ClampUint8(g1 + i1);
		//		c0.b = ClampUint8(b1 + i1);

		//		const int32 i2 = intensityModifierDefault[u.idht.mode.idm.cw2][modifierIdx];
		//		ColorRGBA8& c1 = subblockColors1[modifierIdx];
		//		c1.r = ClampUint8(r2 + i2);
		//		c1.g = ClampUint8(g2 + i2);
		//		c1.b = ClampUint8(b2 + i2);
		//	}

		//	if (u.idht.mode.idm.flipbit)
		//	{
		//		//Two 4x2-pixel subblocks on top of each other
		//		for (uint32 j = 0; j < 2; j++)
		//		{
		//			for (uint32 i = 0; i < 4; i++)
		//			{
		//				uint32 index = j * destRowPitch + i * 4;
		//				*(uint32*)(dest + index) = subblockColors0[getIndex(i, j)].dwColor;
		//			}
		//		}
		//		for (uint32 j = 2; j < 4; j++)
		//		{
		//			for (uint32 i = 0; i < 4; i++)
		//			{
		//				uint32 index = j * destRowPitch + i * 4;
		//				*(uint32*)(dest + index) = subblockColors0[getIndex(i, j)].dwColor;
		//			}
		//		}
		//	}
		//	else
		//	{
		//		//Two 2x4-pixel subblocks side-by-side
		//		for (uint32 j = 0; j < 4; j++)
		//		{
		//			for (uint32 i = 0; i < 2; i++)
		//			{
		//				uint32 index = j * destRowPitch + i * 4;
		//				*(uint32*)(dest + index) = subblockColors0[getIndex(i, j)].dwColor;
		//			}
		//		}
		//		for (uint32 j = 0; j < 4; j++)
		//		{
		//			for (uint32 i = 2; i < 4; i++)
		//			{
		//				uint32 index = j * destRowPitch + i * 4;
		//				*(uint32*)(dest + index) = subblockColors0[getIndex(i, j)].dwColor;
		//			}
		//		}
		//	}
		//}

		void DecodeIndividualOrDifferentialMode(uint8* dest, uint32 destRowPitch, int r1, int g1, int b1, int r2, int g2, int b2) const
		{
			//ColorRGBA8 subblockColors0[4];
			//ColorRGBA8 subblockColors1[4];
			//const auto tableIdx1 = u.idht.mode.idm.cw1;
			//const auto tableIdx2 = u.idht.mode.idm.cw2;

			uint32 subblockColors0[4];
			uint32 subblockColors1[4];
			uint32 tableIdx1 = (u.part0 >> 29) & 0x7;
			uint32 tableIdx2 = (u.part0 >> 26) & 0x7;

			for (uint32 modifierIdx = 0; modifierIdx < 4; modifierIdx++)
			{
				const int32 i1 = intensityModifierDefault[tableIdx1][modifierIdx];
				//ColorRGBA8& c0 = subblockColors0[modifierIdx];
				//c0.r = ClampUint8(r1 + i1);
				//c0.g = ClampUint8(g1 + i1);
				//c0.b = ClampUint8(b1 + i1);
				//c0.dwColor = ClampUint8(r1 + i1) | (ClampUint8(g1 + i1) << 8) | (ClampUint8(b1 + i1) << 16) | 0xFF000000;
				subblockColors0[modifierIdx] = ClampUint8(r1 + i1) | (ClampUint8(g1 + i1) << 8) | (ClampUint8(b1 + i1) << 16) | 0xFF000000;

				const int32 i2 = intensityModifierDefault[tableIdx2][modifierIdx];
				//ColorRGBA8& c1 = subblockColors1[modifierIdx];
				//c1.r = ClampUint8(r2 + i2);
				//c1.g = ClampUint8(g2 + i2);
				//c1.b = ClampUint8(b2 + i2);
				//c1.dwColor = ClampUint8(r2 + i2) | (ClampUint8(g2 + i2) << 8) | (ClampUint8(b2 + i2) << 16) | 0xFF000000;
				subblockColors1[modifierIdx] = ClampUint8(r2 + i2) | (ClampUint8(g2 + i2) << 8) | (ClampUint8(b2 + i2) << 16) | 0xFF000000;
			}

			uint32 flip = (u.part0 >> 24) & 0x1;
			if (flip)
			{
				//Two 4x2-pixel subblocks on top of each other
				for (uint32 j = 0; j < 2; j++)
				{
					for (uint32 i = 0; i < 4; i++)
					{
						const uint32 destIndex = j * destRowPitch + i * 4;
						*(uint32*)(dest + destIndex) = subblockColors0[getIndex(i, j)];
					}
				}
				for (uint32 j = 2; j < 4; j++)
				{
					for (uint32 i = 0; i < 4; i++)
					{
						const uint32 destIndex = j * destRowPitch + i * 4;
						*(uint32*)(dest + destIndex) = subblockColors1[getIndex(i, j)];
					}
				}
			}
			else
			{
				//Two 2x4-pixel subblocks side-by-side
				for (uint32 j = 0; j < 4; j++)
				{
					for (uint32 i = 0; i < 2; i++)
					{
						const uint32 destIndex = j * destRowPitch + i * 4;
						*(uint32*)(dest + destIndex) = subblockColors0[getIndex(i, j)];
					}
				}
				for (uint32 j = 0; j < 4; j++)
				{
					for (uint32 i = 2; i < 4; i++)
					{
						const uint32 destIndex = j * destRowPitch + i * 4;
						*(uint32*)(dest + destIndex) = subblockColors1[getIndex(i, j)];
					}
				}
			}
		}

		void DecodeTMode(uint8* dest, uint32 destRowPitch) const
		{
			// Table C.8, distance index for T and H modes
			const auto &tm = u.idht.mode.tm;

			int r1 = extend_4to8bits(tm.TR1a << 2 | tm.TR1b);
			int g1 = extend_4to8bits(tm.TG1);
			int b1 = extend_4to8bits(tm.TB1);
			int r2 = extend_4to8bits(tm.TR2);
			int g2 = extend_4to8bits(tm.TG2);
			int b2 = extend_4to8bits(tm.TB2);

			static int distance[8] = { 3, 6, 11, 16, 23, 32, 41, 64 };
			const int d = distance[tm.Tda << 1 | tm.Tdb];

#ifdef __EMSCRIPTEN__
			 uint32 paintColors[4] = {
				r1 | g1<<8 | b1<<16 | 0xFF000000,
				ClampUint8Right(r2 + d) | ClampUint8Right(g2 + d) << 8 | ClampUint8Right(b2 + d) << 16 | 0xFF000000,
				r2 | g2 << 8 | b2 << 16 | 0xFF000000,
				ClampUint8Left(r2 - d) | ClampUint8Left(g2 - d) << 8 | ClampUint8Left(b2 - d) << 16 | 0xFF000000,
			};


			for (uint32 j = 0; j < 4; j++)
			{
				for (uint32 i = 0; i < 4; i++)
				{
					uint32 destIndex = j * destRowPitch + i * 4;
					*(uint32*)(dest+ destIndex) = paintColors[getIndex(i, j)];
				}
			}
#else
			 ColorRGBA8 paintColors[4] = {
				ColorRGBA8(r1, g1, b1, 255),
				ColorRGBA8(ClampUint8Right(r2 + d), ClampUint8Right(g2 + d), ClampUint8Right(b2 + d), 255),
				ColorRGBA8(r2, g2, b2, 255),
				ColorRGBA8(ClampUint8Left(r2 - d), ClampUint8Left(g2 - d), ClampUint8Left(b2 - d), 255),
			};

			for (uint32 j = 0; j < 4; j++)
			{
				for (uint32 i = 0; i < 4; i++)
				{
					uint32 destIndex = j * destRowPitch + i * 4;
					*(uint32*)(dest+ destIndex) = paintColors[getIndex(i, j)].dwColor;
				}
			}
#endif
		}

		void DecodeHMode(uint8* dest, uint32 destRowPitch) const
		{
			// Table C.8, distance index for T and H modes
			const auto &hm = u.idht.mode.hm;

			int r1 = extend_4to8bits(hm.HR1);
			int g1 = extend_4to8bits(hm.HG1a << 1 | hm.HG1b);
			int b1 = extend_4to8bits(hm.HB1a << 3 | hm.HB1b << 1 | hm.HB1c);
			int r2 = extend_4to8bits(hm.HR2);
			int g2 = extend_4to8bits(hm.HG2a << 1 | hm.HG2b);
			int b2 = extend_4to8bits(hm.HB2);

			static const int distance[8] = { 3, 6, 11, 16, 23, 32, 41, 64 };
			const int orderingTrickBit =
				((r1 << 16 | g1 << 8 | b1) >= (r2 << 16 | g2 << 8 | b2) ? 1 : 0);
			const int d = distance[(hm.Hda << 2) | (hm.Hdb << 1) | orderingTrickBit];

#ifdef __EMSCRIPTEN__
			uint32 paintColors[4] = {
				ClampUint8Right(r1 + d) | ClampUint8Right(g1 + d) << 8 | ClampUint8Right(b1 + d) << 16 | 0xFF000000,
				ClampUint8Left(r1 - d) | ClampUint8Left(g1 - d) << 8 | ClampUint8Left(b1 - d) << 16 | 0xFF000000,
				ClampUint8Right(r2 + d) | ClampUint8Right(g2 + d) << 8 | ClampUint8Right(b2 + d) << 16 | 0xFF000000,
				ClampUint8Left(r2 - d) | ClampUint8Left(g2 - d) << 8 | ClampUint8Left(b2 - d) << 16 | 0xFF000000,
			};

			for (uint32 j = 0; j < 4; j++)
			{
				for (uint32 i = 0; i < 4; i++)
				{
					uint32 destIndex = j * destRowPitch + i * 4;
					*(uint32*)(dest + destIndex) = paintColors[getIndex(i, j)];
				}
			}
#else
			const ColorRGBA8 paintColors[4] = {
				ColorRGBA8(ClampUint8Right(r1 + d), ClampUint8Right(g1 + d), ClampUint8Right(b1 + d), 255),
				ColorRGBA8(ClampUint8Left(r1 - d), ClampUint8Left(g1 - d), ClampUint8Left(b1 - d), 255),
				ColorRGBA8(ClampUint8Right(r2 + d), ClampUint8Right(g2 + d), ClampUint8Right(b2 + d), 255),
				ColorRGBA8(ClampUint8Left(r2 - d), ClampUint8Left(g2 - d), ClampUint8Left(b2 - d), 255),
			};

			for (int j = 0; j < 4; j++)
			{
				for (int i = 0; i < 4; i++)
				{
					int index = j * destRowPitch + i * 4;
					*(uint32*)(dest + index) = paintColors[getIndex(i, j)].dwColor;
				}
			}
#endif
		}

		void DecodePlanarMode(uint8* dest, uint32 destRowPitch) const
		{
#ifdef __EMSCRIPTEN__
			//need optimize
			uint32 byte0 = u.part0 & 0xFF;
			uint32 byte1 = (u.part0 >> 8) & 0xFF;
			uint32 byte2 = (u.part0 >> 16) & 0xFF;
			uint32 byte3 = (u.part0 >> 24) & 0xFF;
			uint32 byte4 = u.part1 & 0xFF;
			uint32 byte5 = (u.part1 >> 8) & 0xFF;
			uint32 byte6 = (u.part1 >> 16) & 0xFF;
			uint32 byte7 = (u.part1 >> 24) & 0xFF;
			int ro = extend_6to8bits((byte0 >> 1) & 0x3F);
			int gO1 = byte0 & 0x1;
			int gO2 = (byte1 >> 1) & 0x3F;
			int go = extend_7to8bits((gO1 << 6) | gO2);
			int bO1 = byte1 & 0x1;
			int bO2 = (byte2 >> 3) & 0x3;
			int bO3 = ((byte2 & 0x3) << 1) | (byte3 >> 7);
			int bo = extend_6to8bits(bO1 << 5 | bO2 << 3 | bO3);

			int rH1 = (byte3 >> 2) & 0x1F;
			int rH2 = byte3 & 0x1;
			int rh = extend_6to8bits(rH1 << 1 | rH2);
			int gh = extend_7to8bits(byte4 >> 1);
			int bh = extend_6to8bits(((byte4 & 0x1) << 5) | (byte5 >> 3));

			int rv = extend_6to8bits(((byte5 & 0x7) << 3) | (byte6 >> 5));
			int gv = extend_7to8bits(((byte6 & 0x1F) << 2) | (byte7 >> 6));
			int bv = extend_6to8bits(byte7 & 0x3F);
#else
			int ro = extend_6to8bits(u.pblk.RO);
			int go = extend_7to8bits(u.pblk.GO1 << 6 | u.pblk.GO2);
			int bo = extend_6to8bits(u.pblk.BO1 << 5 | u.pblk.BO2 << 3 | u.pblk.BO3a << 1 | u.pblk.BO3b);
			int rh = extend_6to8bits(u.pblk.RH1 << 1 | u.pblk.RH2);
			int gh = extend_7to8bits(u.pblk.GH);
			int bh = extend_6to8bits(u.pblk.BHa << 5 | u.pblk.BHb);
			int rv = extend_6to8bits(u.pblk.RVa << 3 | u.pblk.RVb);
			int gv = extend_7to8bits(u.pblk.GVa << 2 | u.pblk.GVb);
			int bv = extend_6to8bits(u.pblk.BV);
#endif

			int rvo = rv - ro;
			int gvo = gv - go;
			int bvo = bv - bo;

			int rho = rh - ro;
			int gho = gh - go;
			int bho = bh - bo;

			for (int j = 0; j < 4; j++)
			{
				int ry = j * rvo + 2;
				int gy = j * gvo + 2;
				int by = j * bvo + 2;

				for (int i = 0; i < 4; i++)
				{
					int destIndex = j * destRowPitch + i * 4;
#ifdef __EMSCRIPTEN__
					uint32 r = ClampUint8(((i*rho + ry) >> 2) + ro);
					uint32 g = ClampUint8(((i*gho + gy) >> 2) + go);
					uint32 b = ClampUint8(((i*bho + by) >> 2) + bo);
					*(uint32*)(dest + destIndex) = r | (g << 8) | (b << 16) | 0xFF000000;
#else
					ColorRGBA8& color = *(ColorRGBA8*)(dest + destIndex);
					//color.r = ClampUint8((i*(rh - ro) + j * (rv - ro) + 2) >> 2 + ro);
					//color.g = ClampUint8((i*(gh - go) + j * (gv - go) + 2) >> 2 + ro);
					//color.b = ClampUint8((i*(bh - bo) + j * (bv - ro) + 2) >> 2 + ro);
					color.r = ClampUint8(((i*rho + ry) >> 2) + ro);
					color.g = ClampUint8(((i*gho + gy) >> 2) + go);
					color.b = ClampUint8(((i*bho + by) >> 2) + bo);
					color.a = 255;
#endif
				}
			}
		}
	};

	//Table C.10: Intensity modifier sets for alpha component.
	static const int32 intensityModifierAlpha[16][8] =
	{
		{ -3, -6,  -9, -15, 2, 5, 8, 14 },
		{ -3, -7, -10, -13, 2, 6, 9, 12 },
		{ -2, -5,  -8, -13, 1, 4, 7, 12 },
		{ -2, -4,  -6, -13, 1, 3, 5, 12 },
		{ -3, -6,  -8, -12, 2, 5, 7, 11 },
		{ -3, -7,  -9, -11, 2, 6, 8, 10 },
		{ -4, -7,  -8, -11, 3, 6, 7, 10 },
		{ -3, -5,  -8, -11, 2, 4, 7, 10 },
		{ -2, -6,  -8, -10, 1, 5, 7,  9 },
		{ -2, -5,  -8, -10, 1, 4, 7,  9 },
		{ -2, -4,  -8, -10, 1, 3, 7,  9 },
		{ -2, -5,  -7, -10, 1, 4, 6,  9 },
		{ -3, -4,  -7, -10, 2, 3, 6,  9 },
		{ -1, -2,  -3, -10, 0, 1, 2,  9 },
		{ -4, -6,  -8,  -9, 3, 5, 7,  8 },
		{ -3, -5,  -7,  -9, 2, 4, 6,  8 }
	};

	class EACBlock
	{
	private:
		uint32 part0;
		uint32 part1;
	public:
		void Decode(uint8* dest, uint32 destRowPitch) const
		{
			//	uint8 base_codeword : 8;
			//	uint8 table_index : 4;
			//	uint8 multiplier : 4;
			//	uint8 mc1 : 2; 16
			//	uint8 mb : 3;  18
			//	uint8 ma : 3;  21
			//	uint8 mf1 : 1; 24
			//	uint8 me : 3;  25
			//	uint8 md : 3;  28
			//	uint8 mc2 : 1; 31
			//	uint8 mh : 3;  0
			//	uint8 mg : 3;  3
			//	uint8 mf2 : 2; 6
			//	uint8 mk1 : 2; 8
			//	uint8 mj : 3;  10
			//	uint8 mi : 3;  13
			//	uint8 mn1 : 1; 16
			//	uint8 mm : 3;  17
			//	uint8 ml : 3;  20
			//	uint8 mk2 : 1; 23
			//	uint8 mp : 3;  24
			//	uint8 mo : 3;  27
			//	uint8 mn2 : 2; 30
			int32 base_codeword = part0 & 0xFF;
			uint32 table_index = (part0 >> 8) & 0xF;
			int32 multiplier = (part0 >> 12) & 0xF;

			static uint32 index_array[16];
			index_array[0] = (part0 >> 21) & 0x7;                                 //a
			index_array[1] = (part0 >> 18) & 0x7;                                 //b
			index_array[2] = (((part0 >> 16) & 0x3) << 1) | (part0 >> 31);        //c
			index_array[3] = (part0 >> 28) & 0x7;                                 //d
			index_array[4] = (part0 >> 25) & 0x7;                                 //e
			index_array[5] = (((part0 >> 24) & 0x1) << 2) | ((part1 >> 6) & 0x3); //f
			index_array[6] = (part1 >> 3) & 0x7;                                  //g
			index_array[7] = part1 & 0x7;			                              //h
			index_array[8] = (part1 >> 13) & 0x7;                                 //i
			index_array[9] = (part1 >> 10) & 0x7;                                 //j
			index_array[10] = (((part1 >> 8) & 0x3) << 1) | ((part1 >> 23) & 0x1);//k
			index_array[11] = (part1 >> 20) & 0x7;                                //l
			index_array[12] = (part1 >> 17) & 0x7;                                //m
			index_array[13] = (((part1 >> 16) & 0x1) << 2) | ((part1 >> 30));     //n
			index_array[14] = (part1 >> 27) & 0x7;                                //o
			index_array[15] = (part1 >> 24) & 0x7;                                //p

			for (uint32 j = 0; j < 4; j++)
			{
				for (uint32 i = 0; i < 4; i++)
				{
					const uint32 destIndex = j * destRowPitch + i * 4;
					dest[destIndex + 3] = ClampUint8(base_codeword + multiplier * intensityModifierAlpha[table_index][index_array[i * 4 + j]]);
				}
			}
		}
	};

	void TranscodeETC2_to_RGBA8(const uint8* source, uint8* dest, const uint32 width, const uint32 height)
	{
		const uint32 bw = (width + 3) / 4;  //block width
		const uint32 bh = (height + 3) / 4; //block height

		uint32 destRowPitch = Max(width * 4, 16);
		for (uint32 by = 0; by < bh; ++by)
		{
			for (uint32 bx = 0; bx < bw; ++bx)
			{
				uint32 destOffset = by * 4 * destRowPitch + bx * 16;

				const ETC2Block* pETC2Block = (ETC2Block*)source;
				pETC2Block->Decode(dest+destOffset, destRowPitch);

				source += 8;
			}
		}
	}

	void TranscodeETC2_EAC_to_RGBA8(const uint8* source, uint8* dest, const uint32 width, const uint32 height)
	{
		const uint32 bw = (width + 3) / 4;  //block width
		const uint32 bh = (height + 3) / 4; //block height

		uint32 destRowPitch = Max(width * 4, 16);
		for (uint32 by = 0; by < bh; ++by)
		{
			for (uint32 bx = 0; bx < bw; ++bx)
			{
				uint32 destOffset = by * 4 * destRowPitch + bx * 16;

				const ETC2Block* pETC2Block = (ETC2Block*)(source+8);
				pETC2Block->Decode(dest + destOffset, destRowPitch);


				//ETC2Block.Decode will cover alpha channel with 255, so call EACBlock.Decode after ETC2Block.Decode
				const EACBlock* pEACBlock = (EACBlock*)source;
				pEACBlock->Decode(dest + destOffset, destRowPitch);

				source += 16;
			}
		}
	}
}