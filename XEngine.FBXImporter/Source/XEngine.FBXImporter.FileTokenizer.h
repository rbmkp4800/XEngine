#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>

#include "XEngine.FBXImporter.Token.h"
#include "XEngine.FBXImporter.BufferedFileReader.h"

namespace XEngine::FBXImporter
{
	enum class TokenizerResult
	{
		Success = 0,

		QuotationMarksDoNotMatch,
		BrokenNumber,
		InvalidCharacter,
	};

	class FileTokenizer : public XLib::NonCopyable
	{
	private:
		BufferedFileReader reader;
		uint32 lineCounter = 0;

	public:
		bool initialize(const char *filename);
		TokenizerResult nextToken(Token& token);

		inline uint32 getCurrentLine() { return lineCounter + 1; }
	};
}