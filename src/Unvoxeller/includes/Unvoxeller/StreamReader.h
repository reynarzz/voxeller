#pragma once
#include <Unvoxeller/Types.h>


class StreamReader 
{
public:
	StreamReader() = default;
	StreamReader(const u8* buffer, u64 size);

	StreamReader(StreamReader&) = delete;
	StreamReader operator=(StreamReader&) = delete;

	template<typename T>
	bool Read(T t, u64 size) 
	{
		
	}

	u64 Seekg();

private:
	u64 _cursor = 0;
};