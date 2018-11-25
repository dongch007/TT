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
			uint64 data;
		} u;

	public:
		void Decode(ColorBlock& block) const
		{
			// Select mode
			if (u.idht.mode.idm.diffbit)
			{
				const auto &diff = u.idht.mode.idm.colors.diff;
				int r = (diff.R + diff.dR);
				int g = (diff.G + diff.dG);
				int b = (diff.B + diff.dB);
				if (r < 0 || r > 31)
				{
					DecodeTMode(block);
				}
				else if (g < 0 || g > 31)
				{
					DecodeHMode(block);
				}
				else if (b < 0 || b > 31)
				{
					DecodePlanarMode(block);
				}
				else
				{
					DecodeDifferentialMode(block);
				}
			}
			else
			{
				DecodeIndividualMode(block);
			}
		}

		void DecodeIndividualMode(ColorBlock& block) const
		{
			const auto &indiv = u.idht.mode.idm.colors.indiv;
			int r1 = extend_4to8bits(indiv.R1);
			int g1 = extend_4to8bits(indiv.G1);
			int b1 = extend_4to8bits(indiv.B1);
			int r2 = extend_4to8bits(indiv.R2);
			int g2 = extend_4to8bits(indiv.G2);
			int b2 = extend_4to8bits(indiv.B2);
			DecodeIndividualOrDifferentialMode(block, r1, g1, b1, r2, g2, b2);
		}

		void DecodeDifferentialMode(ColorBlock& block) const
		{
			const auto &diff = u.idht.mode.idm.colors.diff;
			int r1 = extend_5to8bits(diff.R);
			int g1 = extend_5to8bits(diff.G);
			int b1 = extend_5to8bits(diff.B);
			int r2 = extend_5to8bits(diff.R + diff.dR);
			int g2 = extend_5to8bits(diff.G + diff.dG);
			int b2 = extend_5to8bits(diff.B + diff.dB);
			DecodeIndividualOrDifferentialMode(block, r1, g1, b1, r2, g2, b2);
		}

		uint32 getIndex(uint32 x, uint32 y) const
		{
			uint32 bitIndex = x * 4 + y;
			uint32 bitOffset = bitIndex & 7;
			uint32 lsb = (u.idht.pixelIndexLSB[1 - (bitIndex >> 3)] >> bitOffset) & 1;
			uint32 msb = (u.idht.pixelIndexMSB[1 - (bitIndex >> 3)] >> bitOffset) & 1;
			return (msb << 1) | lsb;
		}

		void DecodeIndividualOrDifferentialMode(ColorBlock& block, int r1, int g1, int b1, int r2, int g2, int b2) const
		{
			ColorRGBA8 subblockColors0[4];
			ColorRGBA8 subblockColors1[4];
			for (int modifierIdx = 0; modifierIdx < 4; modifierIdx++)
			{
				const int i1 = intensityModifierDefault[u.idht.mode.idm.cw1][modifierIdx];
				ColorRGBA8& c0 = subblockColors0[modifierIdx];
				c0.r = ClampUint8(r1 + i1);
				c0.g = ClampUint8(g1 + i1);
				c0.b = ClampUint8(b1 + i1);

				const int i2 = intensityModifierDefault[u.idht.mode.idm.cw2][modifierIdx];
				ColorRGBA8& c1 = subblockColors1[modifierIdx];
				c1.r = ClampUint8(r2 + i2);
				c1.g = ClampUint8(g2 + i2);
				c1.b = ClampUint8(b2 + i2);
			}

			if (u.idht.mode.idm.flipbit)
			{
				//Two 4x2-pixel subblocks on top of each other
				for (int j = 0; j < 2; j++)
				{
					for (int i = 0; i < 4; i++)
					{
						block.color(i, j) = subblockColors0[getIndex(i, j)];
					}
				}
				for (int j = 2; j < 4; j++)
				{
					for (int i = 0; i < 4; i++)
					{
						block.color(i, j) = subblockColors1[getIndex(i, j)];
					}
				}
			}
			else
			{
				//Two 2x4-pixel subblocks side-by-side
				for (int j = 0; j < 4; j++)
				{
					for (int i = 0; i < 2; i++)
					{
						block.color(i, j) = subblockColors0[getIndex(i, j)];
					}
				}
				for (int j = 0; j < 4; j++)
				{
					for (int i = 2; i < 4; i++)
					{
						block.color(i, j) = subblockColors1[getIndex(i, j)];
					}
				}
			}
		}

		void DecodeTMode(ColorBlock& block) const
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

			 ColorRGBA8 paintColors[4] = {
				ColorRGBA8(r1, g1, b1, 255),
				ColorRGBA8(ClampUint8Right(r2 + d), ClampUint8Right(g2 + d), ClampUint8Right(b2 + d), 255),
				ColorRGBA8(r2, g2, b2, 255),
				ColorRGBA8(ClampUint8Left(r2 - d), ClampUint8Left(g2 - d), ClampUint8Left(b2 - d), 255),
			};


			for (int j = 0; j < 4; j++)
			{
				for (int i = 0; i < 4; i++)
				{
					block.color(i, j) = paintColors[getIndex(i, j)];
				}
			}
		}

		void DecodeHMode(ColorBlock& block) const
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
					block.color(i, j) = paintColors[getIndex(i, j)];
				}
			}
		}

		void DecodePlanarMode(ColorBlock& block) const
		{
			int ro = extend_6to8bits(u.pblk.RO);
			int go = extend_7to8bits(u.pblk.GO1 << 6 | u.pblk.GO2);
			int bo = extend_6to8bits(u.pblk.BO1 << 5 | u.pblk.BO2 << 3 | u.pblk.BO3a << 1 | u.pblk.BO3b);
			int rh = extend_6to8bits(u.pblk.RH1 << 1 | u.pblk.RH2);
			int gh = extend_7to8bits(u.pblk.GH);
			int bh = extend_6to8bits(u.pblk.BHa << 5 | u.pblk.BHb);
			int rv = extend_6to8bits(u.pblk.RVa << 3 | u.pblk.RVb);
			int gv = extend_7to8bits(u.pblk.GVa << 2 | u.pblk.GVb);
			int bv = extend_6to8bits(u.pblk.BV);

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
					ColorRGBA8& color = block.color(i, j);
					//color.r = ClampUint8((i*(rh - ro) + j * (rv - ro) + 2) >> 2 + ro);
					//color.g = ClampUint8((i*(gh - go) + j * (gv - go) + 2) >> 2 + ro);
					//color.b = ClampUint8((i*(bh - bo) + j * (bv - ro) + 2) >> 2 + ro);
					color.r = ClampUint8(((i*rho + ry) >> 2) + ro);
					color.g = ClampUint8(((i*gho + gy) >> 2) + go);
					color.b = ClampUint8(((i*bho + by) >> 2) + bo);
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
		union 
		{
			struct
			{
				union {
					uint8 us;
					int8 s;
				} base_codeword;
				uint8 table_index : 4;
				uint8 multiplier : 4;
				uint8 mc1 : 2;
				uint8 mb : 3;
				uint8 ma : 3;
				uint8 mf1 : 1;
				uint8 me : 3;
				uint8 md : 3;
				uint8 mc2 : 1;
				uint8 mh : 3;
				uint8 mg : 3;
				uint8 mf2 : 2;
				uint8 mk1 : 2;
				uint8 mj : 3;
				uint8 mi : 3;
				uint8 mn1 : 1;
				uint8 mm : 3;
				uint8 ml : 3;
				uint8 mk2 : 1;
				uint8 mp : 3;
				uint8 mo : 3;
				uint8 mn2 : 2;
			};
			/*struct
			{
				uint64 base_codeword : 4;
				uint64 table_index : 4;
				uint64 multiplier : 4;
				uint64 a : 3;
				uint64 b : 3;
				uint64 c : 3;
				uint64 d : 3;
				uint64 e : 3;
				uint64 f : 3;
				uint64 g : 3;
				uint64 h : 3;
				uint64 i : 3;
				uint64 j : 3;
				uint64 k : 2;
				uint64 l : 3;
				uint64 m : 3;
				uint64 n : 3;
				uint64 o : 3;
				uint64 p : 3;
			};*/
		};
	public:
		void Decode(ColorBlock& block) const
		{
			uint8 index_array[16];
			index_array[0]  = ma;
			index_array[1]  = mb;
			index_array[2]  = mc1 << 1 | mc2;
			index_array[3]  = md;
			index_array[4]  = me;
			index_array[5]  = mf1 << 2 | mf2;
			index_array[6]  = mg;
			index_array[7]  = mh;
			index_array[8]  = mi;
			index_array[9]  = mj;
			index_array[10] = mk1 << 1 | mk2;
			index_array[11] = ml;
			index_array[12] = mm;
			index_array[13] = mn1 << 2 | mn2;
			index_array[14] = mo;
			index_array[15] = mp;

			//index_array[0] = a;
			//index_array[1] = b;
			//index_array[2] = c;
			//index_array[3] = d;
			//index_array[4] = e;
			//index_array[5] = f;
			//index_array[6] = g;
			//index_array[7] = h;
			//index_array[8] = i;
			//index_array[9] = j;
			//index_array[10] = k;
			//index_array[11] = l;
			//index_array[12] = m;
			//index_array[13] = n;
			//index_array[14] = o;
			//index_array[15] = p;
			
			for (int j = 0; j < 4; j++)
			{
				for (int i = 0; i < 4; i++)
				{
					ColorRGBA8& col = block.color(i, j);
					//col.a = multiplier;
					//col.a = base_codeword.us;
					col.a = ClampUint8(base_codeword.us + multiplier * intensityModifierAlpha[table_index][index_array[i*4+j]]);
					//col.a = ClampUint8(base_codeword.us + intensityModifierAlpha[i][index_array[j]]);
					//col.a = index_array[j];
					//col.a = ClampUint8(base_codeword + multiplier * intensityModifierAlpha[table_index][index_array[i * 4 + j]]);
				}
			}
		}
	};

	void TranscodeETC2_to_RGB8(const uint8* source, uint8* dest, const uint32 width, const uint32 height)
	{
		const uint32 bw = (width + 3) / 4;  //block width
		const uint32 bh = (height + 3) / 4; //block height

		ColorBlock colorBlock;
		for (uint32 by = 0; by < bh; ++by)
		{
			for (uint32 bx = 0; bx < bw; ++bx)
			{
				const ETC2Block* pETC2Block = (ETC2Block*)source;
				pETC2Block->Decode(colorBlock);

				for (uint32 y = 0; y < 4; ++y)
				{
					for (uint32 x = 0; x < 4; ++x)
					{
						uint32 index = (4 * by + y)*width + (4 * bx + x);
						memcpy(&dest[index * 3], &colorBlock.color(x, y), 24); //only copy rgb(24 bytes)
					}
				}

				source += 8;
			}
		}
	}

	void TranscodeETC2_to_RGBA8(const uint8* source, uint8* dest, const uint32 width, const uint32 height)
	{
		const uint32 bw = (width + 3) / 4;  //block width
		const uint32 bh = (height + 3) / 4; //block height

		ColorBlock colorBlock;
		for (uint32 by = 0; by < bh; ++by)
		{
			for (uint32 bx = 0; bx < bw; ++bx)
			{
				const ETC2Block* pETC2Block = (ETC2Block*)source;
				pETC2Block->Decode(colorBlock);

				for (uint32 y = 0; y < 4; ++y)
				{
					for (uint32 x = 0; x < 4; ++x)
					{
						uint32 index = (4 * by + y)*width + (4 * bx + x);
						memcpy(&dest[index * 4], &colorBlock.color(x, y), sizeof(ColorRGBA8));
					}
				}

				source += 8;
			}
		}
	}

	void TranscodeETC2_EAC_to_RGBA8(const uint8* source, uint8* dest, const uint32 width, const uint32 height)
	{
		const uint32 bw = (width + 3) / 4;  //block width
		const uint32 bh = (height + 3) / 4; //block height

		ColorBlock colorBlock;
		for (uint32 by = 0; by < bh; ++by)
		{
			for (uint32 bx = 0; bx < bw; ++bx)
			{
				const ETC2Block* pETC2Block = (ETC2Block*)(source+8);
				pETC2Block->Decode(colorBlock);

				//ETC2Block.Decode will cover alpha channel with 255, so call EACBlock.Decode after ETC2Block.Decode
				const EACBlock* pEACBlock = (EACBlock*)source;
				pEACBlock->Decode(colorBlock);

				for (uint32 y = 0; y < 4; ++y)
				{
					for (uint32 x = 0; x < 4; ++x)
					{
						uint32 index = (4 * by + y)*width + (4 * bx + x);
						memcpy(&dest[index * 4], &colorBlock.color(x, y), sizeof(ColorRGBA8));
						//ColorRGBA8& col = colorBlock.color(y * 4 + x);
						//dest[index] = colorBlock.color(y*4+x).dwColor;
						//dest[index] = col.r;
						//dest[index+1] = col.g;
						//dest[index+2] = col.b;
						//dest[index+3] = col.a;
					}
				}

				source += 16;
			}
		}
	}
}