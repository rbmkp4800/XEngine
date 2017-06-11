#pragma once

#include <XLib.Types.h>

namespace XEngine::FBXImporter
{
	struct String
	{
		static constexpr uint32 lengthLimit = 24;

		char data[lengthLimit];

		inline char& operator [] (uint32 index) { return data[index]; }

		inline bool equals(const char* thatStr) const
		{
			const char *thisStr = data;
			const char *thisStrLimit = thisStr + lengthLimit;

			while (thisStr < thisStrLimit && *thatStr != 0)
			{
				if (*thisStr != *thatStr)
					return false;

				thisStr++;
				thatStr++;
			}

			return true;
		}
	};

	enum class TokenCode : uint8
	{
		None = 0,

		Inentifier,
		Int,
		Float,
		String,
	};

	struct Token
	{
		static constexpr uint32 stringLength = 24;

		union
		{
			float32 floatValue;
			sint32 intValue;
			String string;
		};

		TokenCode code;
	};
}