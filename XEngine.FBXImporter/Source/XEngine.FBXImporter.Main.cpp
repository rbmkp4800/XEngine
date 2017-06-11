#include <stdio.h>

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Containers.Vector.h>
#include <XLib.Program.h>

#include "XEngine.FBXImporter.Token.h"
#include "XEngine.FBXImporter.FileTokenizer.h"
#include "XEngine.FBXImporter.Parser.h"

using namespace XLib;
using namespace XEngine::FBXImporter;

void Program::Run()
{
	printf("FBX parser by RBMKP4800\n");

	FileTokenizer tokenizer;
	//if (!tokenizer.initialize("D:\\rbmkp4800\\Downloads\\prom.txt.fbx.txt"))
	if (!tokenizer.initialize("example.fbx.txt"))
	{
		printf("can't open file\n");
		return;
	}

	Parser parser;
	while (!parser.isFinalized())
	{
		Token token;
		TokenizerResult tokenizerResult = tokenizer.nextToken(token);
		if (tokenizerResult != TokenizerResult::Success)
		{
			printf("tokenizer error %d at %d\n", tokenizerResult, tokenizer.getCurrentLine());
			return;
		}
		ParserResult parserResult = parser.pushToken(token);
		if (parserResult != ParserResult::Success)
		{
			printf("parser error %d at %d\n", parserResult, tokenizer.getCurrentLine());
			return;
		}
	}

	Vector<MeshData> meshes = parser.takeMeshes();
	for each (MeshData& mesh in meshes)
	{
		printf("mesh %s %d %d\n", mesh.name.data, mesh.vertexCount, mesh.indexCount);

		//for (uint32 i = 0; i < mesh.vertexCount; i++)
		//	printf("%f ", mesh.vertices[i]);
		//printf("\n\n");
	}
}