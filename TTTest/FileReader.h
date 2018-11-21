#pragma once
#include <stdio.h>
#include "../TT/BaseType.h"
using namespace TT;
class FileReader
{
public:
	explicit FileReader(const char* file)
	: fp(NULL)
	, buffer(NULL)
	, current(0)
	{
		fopen_s(&fp, file, "rb");
		fseek(fp, 0L, SEEK_END);
		length = ftell(fp);
		buffer = new uint8[length];

		fseek(fp, 0, SEEK_SET);
		fread(buffer, length, 1, fp);

		fclose(fp);
		fp = NULL;
	}
	~FileReader()
	{
		if (buffer != NULL)
			delete buffer;
	}
	uint8 ReadInt8()
	{
		_ASSERT((current + 1) <= length);

		int8 data = *((int8*)&buffer[current]);
		current++;
		return data;
	}

	uint8 ReadUint8()
	{
		_ASSERT((current + 1) <= length);

		return buffer[current++];
	}

	uint16 ReadInt16()
	{
		_ASSERT((current + 2) <= length);

		uint16 data = *((uint16*)&buffer[current]);
		current+=2;
		return data;
	}

	int32 ReadInt32()
	{
		_ASSERT((current + 4) <= length);

		int32 data = *((int32*)&buffer[current]);
		current += 4;
		return data;
	}

	uint32 ReadUint32()
	{
		_ASSERT((current + 4) <= length);

		uint32 data = *((uint32*)&buffer[current]);
		current += 4;
		return data;
	}

	const uint8* ReadBuffer(uint32 size)
	{
		_ASSERT((current + size) <= length);

		uint8* pData = buffer + current;

		current += size;
		
		return pData;
	}

private:
	FILE* fp;
	uint32 length;
	uint8* buffer;

	uint32 current;
};