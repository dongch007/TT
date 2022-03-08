#include "stdafx.h"
#include <chrono>
#include <iostream>
#include "svpng.h"
#include "../TT/ColorBlock.h"
#include "../TT//ETC.h"
#include "FileReader.h"
using namespace TT;

void saveAsPNG(const char* file, uint8* data, uint32 width, uint32 height)
{
	FILE* fp = NULL;
	fopen_s(&fp, file, "wb");
	if (fp != nullptr)
	{
		svpng(fp, width, height, data, 1);
		fclose(fp);
	}
}


int main()
{
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

	auto start = std::chrono::steady_clock::now();

	for (int i = 0; i < 100; i++)
		TranscodeETC2_EAC_to_RGBA8(compressedData, unCompressedData, pixelWidth, pixelHeight);

	auto end = std::chrono::steady_clock::now();

	//auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	using milliseconds = std::chrono::duration<double, std::milli>;
	auto time = std::chrono::duration_cast<milliseconds>(end - start);
	std::cout << time.count() << "ms\n";

	saveAsPNG("ground.png", unCompressedData, pixelWidth, pixelHeight);

	printf("!!!!!!!!!!!!!!!!!!");
	//getchar();
	return 0;
}