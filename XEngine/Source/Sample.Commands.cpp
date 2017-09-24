#include <XLib.Memory.h>
#include <XLib.Strings.h>

#include "Sample.h"

using namespace XLib;
using namespace XEngine;

namespace
{
	struct Word
	{
		const char* string;
		uint32 length;

		inline bool compareTo(const char* that)
		{
			for (uint32 i = 0; i < length; i++)
			{
				if (string[i] != that[i])
					return false;
			}

			return that[length] == 0;
		}

		inline uint64 toUInt64()
		{
			uint64 result = 0;
			Memory::Copy(&result, string, min<uint32>(sizeof(result), length));
			return result;
		}
	};

	class CommandStringParser
	{
	private:
		const char* current;

		inline void skipSeparators()
		{
			while (*current == ' ' || *current == '\t')
				current++;
		}

	public:
		inline CommandStringParser(const char* string) : current(string) {}

		inline bool parseWord(Word& word)
		{
			skipSeparators();

			if (*current == '\n' || *current == '\0')
				return false;

			word.string = current;
			const char *begin = current;
			for (;;)
			{
				char c = *current;
				if (c == ' ' || c == '\t' || c == '\n' || c == '\0')
					break;

				current++;
			}
			word.length = uint32(current - begin);

			return true;
		}

		// TODO: refactor to parse<Type>(Type& value);
		inline bool parseSInt32(sint32& value)
		{
			skipSeparators();
			if (!Strings::Parse<sint32>(current, value, &current))
				return false;

			char c = *current;
			return c == ' ' || c == '\t' || c == '\n' || c == '\0';
		}
		inline bool parseFloat32(float32& value)
		{
			skipSeparators();
			if (!Strings::Parse<float32>(current, value, &current))
				return false;

			char c = *current;
			return c == ' ' || c == '\t' || c == '\n' || c == '\0';
		}
		/*inline bool parseFloat32(float32& value)
		{

		}*/
	};
}

void SampleWindow::onConsoleCommand(const char* commandString)
{
	CommandStringParser parser(commandString);

	Word commandName;
	if (!parser.parseWord(commandName))
		return;

	if (commandName.compareTo("quit"))
	{
		destroy();
	}
	else if (commandName.compareTo("scene.clear"))
	{
		
	}
	else if (commandName.compareTo("camera.setPosition"))
	{
		float32x3 position;
		if (!parser.parseFloat32(position.x) ||
			!parser.parseFloat32(position.y) ||
			!parser.parseFloat32(position.z))
		{
			// TODO: error
			return;
		}

		xerCamera.position = position;
	}
	else if (commandName.compareTo("geometry.generate"))
	{
		Word templateName;
		Word geometryIdentifier;
		if (!parser.parseWord(templateName) ||
			!parser.parseWord(geometryIdentifier))
		{
			// TODO: error
			return;
		}

		GeometryBaseEntry& entry = geometryBase[geometryBaseEntryCount];
		geometryBaseEntryCount++;

		if (templateName.compareTo("cube"))
			XERGeometryGenerator::Cube(&xerDevice, &entry.xerGeometry);
		else if (templateName.compareTo("sphere"))
			XERGeometryGenerator::Sphere(5, &xerDevice, &entry.xerGeometry);

		entry.id = geometryIdentifier.toUInt64();
	}
	else if (commandName.compareTo("scene.createObject"))
	{
		Word geometryIdentifier;
		float32x3 position;

		if (!parser.parseWord(geometryIdentifier) ||
			!parser.parseFloat32(position.x) ||
			!parser.parseFloat32(position.y) ||
			!parser.parseFloat32(position.z))
		{
			// TODO: error
			return;
		}

		float32x3 scale(1.0f, 1.0f, 1.0f);
		if (parser.parseFloat32(scale.x))
		{
			if (!parser.parseFloat32(scale.y) ||
				!parser.parseFloat32(scale.z))
			{
				// TODO: error
				return;
			}
		}

		uint64 id = geometryIdentifier.toUInt64();

		XERGeometry *xerGeometry = nullptr;

		for each (GeometryBaseEntry& entry in geometryBase)
		{
			if (entry.id == id)
			{
				xerGeometry = &entry.xerGeometry;
				break;
			}
		}

		if (!xerGeometry)
		{
			// TODO: error
			return;
		}

		xerScene.createGeometryInstance(xerGeometry, &xerPlainEffect,
			Matrix3x4::Translation(position) * Matrix3x4::Scale(scale));
	}
	else
		xerConsole.write(XECONSOLE_SETCOLOR_RED_LITERAL "unknown command\n");
}