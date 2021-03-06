// TTTest.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
#include "svpng.h"
#include "../TT/ColorBlock.h"
#include "../TT//ETC.h"
#include "FileReader.h"
using namespace TT;
void test_rgba(void) {
	unsigned char rgba[256 * 256 * 4], *p = rgba;
	unsigned x, y;
	FILE* fp = NULL;
	fopen_s(&fp, "rgba.png", "wb");
	for (y = 0; y < 256; y++)
		for (x = 0; x < 256; x++) {
			*p++ = (unsigned char)x;                /* R */
			*p++ = (unsigned char)y;                /* G */
			*p++ = 128;                             /* B */
			*p++ = (unsigned char)((x + y) / 2);    /* A */
		}
	svpng(fp, 256, 256, rgba, 1);
	fclose(fp);
}

void saveAsPNG(const char* file, uint8* data, uint32 width, uint32 height)
{
	FILE* fp = NULL;
	fopen_s(&fp, file, "wb");
	svpng(fp, width, height, data, 1);
	fclose(fp);
}

class ETC2Block
{
public:
	union {
		uint32 data;
		struct
		{
			uint32 d0 : 6;
			//uint8 d1 : 3;
			uint32 d2 : 3;
			uint32 d3 : 3;
			uint32 d4 : 3;
			uint32 d5 : 1;
		};
	};
};

int main()
{
	TT::ColorBlock block;
	TT::ColorRGBA8 col(0xFF000000);
	ETC2Block b;
	b.data = 0x00FF;

	FileReader reader("ground.ktx");

	uint8 KtxIdentifier[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };

	for (int i = 0; i < 12; i++)
	{
		if (reader.ReadUint8() != KtxIdentifier[i])
			_ASSERT(false);
	}
	uint32 endianness = reader.ReadUint32();
	uint32 glType = reader.ReadUint32();
	uint32 glTypeSize = reader.ReadUint32();
	uint32 glFormat = reader.ReadUint32();
	uint32 glInternalFormat = reader.ReadUint32();
	uint32 glBaseInternalFormat = reader.ReadUint32();
	uint32 pixelWidth = reader.ReadUint32();
	uint32 pixelHeight = reader.ReadUint32();
	uint32 pixelDepth = reader.ReadUint32();
	uint32 numberOfArrayElements = reader.ReadUint32();
	uint32 numberOfFaces = reader.ReadUint32();
	uint32 numberOfMipmapLevels = reader.ReadUint32();
	uint32 bytesOfKeyValueData = reader.ReadUint32();

	uint32 imageSize = reader.ReadUint32();
	const uint8* compressedData = reader.ReadBuffer(imageSize);

	//uint8* unCompressedData = new uint8[imageSize * 8];

	//TranscodeETC2_to_RGBA8(compressedData, unCompressedData, pixelWidth, pixelHeight);

	uint8* unCompressedData = new uint8[imageSize * 4];

	LARGE_INTEGER nFreq;

	LARGE_INTEGER nBeginTime;

	LARGE_INTEGER nEndTime;
	QueryPerformanceFrequency(&nFreq);

	QueryPerformanceCounter(&nBeginTime);

	for(int i = 0; i <1000; i++)
	TranscodeETC2_EAC_to_RGBA8(compressedData, unCompressedData, pixelWidth, pixelHeight);

	QueryPerformanceCounter(&nEndTime);

	double time = (double)(nEndTime.QuadPart - nBeginTime.QuadPart) / (double)nFreq.QuadPart;
	printf("%f", time*1000);


	saveAsPNG("ground.png", unCompressedData, pixelWidth, pixelHeight);

	printf("!!!!!!!!!!!!!!!!!!");
	//getchar();
    return 0;
}

