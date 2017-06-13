#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.System.File.h>

namespace XEngine::FBXImporter
{
	class BufferedFileReader : XLib::NonCopyable
	{
	private:
		static constexpr uint16 bufferSize = 512;

		XLib::File file;
		char buffer[bufferSize];
		uint16 position = 0;
		uint16 limit = 0;
		bool lastBlock = false;

		uint32 currentBlockIndex = uint32(-1);

		void readBlock()
		{
			uint32 readSize = 0;
			file.read(buffer, bufferSize, readSize);

			position = 0;
			limit = uint16(readSize);
			lastBlock = limit < bufferSize;

			currentBlockIndex++;
		}

	public:
		bool initialize(const char *filename)
		{
			if (!file.open(filename, XLib::FileAccessMode::Read))
				return false;

			readBlock();
			return true;
		}

		inline char getCharUnsafe()
		{
			char result = buffer[position];
			skipCharUnsafe();

			return result;
		}
		inline char peekCharUnsafe()
		{
			return buffer[position];
		}
		inline void skipCharUnsafe()
		{
			position++;
			if (position < limit)
				return;
			if (lastBlock)
				return;

			readBlock();
		}

		inline char getChar()
		{
			return position < limit ? getCharUnsafe() : 0;
		}
		inline char peekChar()
		{
			return position < limit ? peekCharUnsafe() : 0;
		}
		inline void skipChar()
		{
			if (position < limit)
				skipCharUnsafe();
		}

		inline bool hasNotEnded() { return position < limit; }
		inline bool hasEnded() { return position >= limit; }

		inline uint32 getFilePosition() { return currentBlockIndex * bufferSize + position; }
		inline uint32 getFileSize() { return uint32(file.getSize()); }
	};
}