#include <stdio.h>

#include <XLib.Util.h>

#include "XEngine.FBXImporter.FileTokenizer.h"

using namespace XLib;
using namespace XEngine::FBXImporter;

inline bool isSeparator(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
inline bool isDigit(char c) { return c >= '0' && c <= '9'; }
inline bool isAlpabet(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
inline bool isNewLine(char c) { return c == '\n' || c == '\r'; }

inline bool canStartNumber(char c) { return isDigit(c) || c == '-'; }
inline bool canStartIdentifier(char c) { return isAlpabet(c); }
inline bool canContinueIdentidier(char c) { return isAlpabet(c) || isDigit(c); }

bool FileTokenizer::initialize(const char *filename)
{
	return reader.initialize(filename);
}

TokenizerResult FileTokenizer::nextToken(Token& token)
{
	token.code = TokenCode::None;

	char firstChar = 0;
	for (;;)
	{
		while (isSeparator(reader.peekChar()))
		{
			if (reader.peekCharUnsafe() == '\n')
				lineCounter++;
			reader.skipCharUnsafe();
		}

		if (reader.hasEnded())
			return TokenizerResult::Success;

		firstChar = reader.getCharUnsafe();

		if (firstChar != ';')
			break;

		for (;;)
		{
			if (reader.hasEnded())
				TokenizerResult::Success;

			char c = reader.getCharUnsafe();
			if (c == '\n')
			{
				lineCounter++;
				break;
			}
		}
	}

	if (firstChar == 0)
		return TokenizerResult::Success;

	if (canStartIdentifier(firstChar))
	{
		token.string[0] = firstChar;
		for (uint32 i = 1;; i++)
		{
			char c = reader.peekChar();
			if (reader.hasNotEnded() && canContinueIdentidier(c))
				reader.skipCharUnsafe();
			else
			{
				if (i == 1)
					token.code = TokenCode(token.string[0]);
				else
				{
					token.code = TokenCode::Inentifier;
					for (uint32 j = i; j < token.string.lengthLimit; j++)
						token.string[j] = 0;
				}
				break;
			}

			if (i < token.string.lengthLimit)
				token.string[i] = c;
		}
	}
	else if (firstChar == '"')
	{
		for (uint32 i = 0;; i++)
		{
			if (reader.hasEnded())
				return TokenizerResult::QuotationMarksDoNotMatch;

			char c = reader.getCharUnsafe();
			if (c == '"')
			{
				for (uint32 j = i; j < token.string.lengthLimit; j++)
					token.string[j] = 0;
				break;
			}

			if (i < token.string.lengthLimit)
				token.string[i] = c;
		}

		token.code = TokenCode::String;
	}
	else if (canStartNumber(firstChar))
	{
		char buffer[32];
		buffer[0] = firstChar;

		bool isFloat = false;
		for (uint32 i = 1;; i++)
		{
			char c = reader.peekChar();
			if (isDigit(c) || c == '.'  || c == 'e' || c == '+' || c == '-')
				reader.skipCharUnsafe();
			else
			{
				buffer[i] = 0;
				break;
			}

			if (c == '.')
			{
				if (isFloat)
					return TokenizerResult::BrokenNumber;
				isFloat = true;
			}

			buffer[i] = c;

			if (i >= countof(buffer))
				return TokenizerResult::BrokenNumber;
		}

		if (isFloat)
		{
			float32 value = 0.0f;
			sscanf_s(buffer, "%f", &value);

			token.code = TokenCode::Float;
			token.floatValue = value;
		}
		else
		{
			sint32 value = 0;
			sscanf_s(buffer, "%d", &value);

			token.code = TokenCode::Int;
			token.intValue = value;
		}
	}
	else if (firstChar >= ' ')
		token.code = TokenCode(firstChar);
	else
		return TokenizerResult::InvalidCharacter;

	return TokenizerResult::Success;
}