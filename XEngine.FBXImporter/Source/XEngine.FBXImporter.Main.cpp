#include <stdio.h>

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Containers.Vector.h>
#include <XLib.Program.h>

#include "XEngine.Render.Vertices.h"
#include "XEngine.Formats.XEGeometry.h"

#include "XEngine.FBXImporter.Token.h"
#include "XEngine.FBXImporter.FileTokenizer.h"
#include "XEngine.FBXImporter.Parser.h"

using namespace XLib;
using namespace XEngine;
using namespace XEngine::FBXImporter;
using namespace XEngine::Formats::XEGeometry;

void Program::Run()
{
	printf("FBX parser by RBMKP4800\n");

	FileTokenizer tokenizer;
	if (!tokenizer.initialize("D:\\rbmkp4800\\Downloads\\prom.fbx"))
	//if (!tokenizer.initialize("example.txt"))
	{
		printf("can't open file\n");
		return;
	}

	printf("reading...\n");

	uint32 fileSizeKib = tokenizer.getFileSize() / 1024;
	uint32 progressUpdateCounter = 0;

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

		progressUpdateCounter++;
		if (progressUpdateCounter > 100000)
		{
			progressUpdateCounter = 0;
			printf("\r%4d%%", tokenizer.getFilePosition() / 1024 * 100 / fileSizeKib);
		}
	}
	printf("\r      \n");

	Vector<MeshData> meshes = parser.takeMeshes();
	for (uint32 i = 0; i < meshes.getSize(); i++)
	{
		printf("mesh %d: \"%s\" %d %d %d\n", i, meshes[i].name.data,
			meshes[i].vertexCount, meshes[i].indexCount, meshes[i].normalCount);

		//for (uint32 i = 0; i < mesh.vertexCount; i++)
		//	printf("%f ", mesh.vertices[i]);
		//printf("\n\n");
	}

	uint32 meshIndex = 0;
	for (;;)
	{
		printf("select mesh to import (0-%d): ", meshes.getSize() - 1);

		fseek(stdin, 0, SEEK_END);

		if (scanf("%d", &meshIndex) != 1)
		{
			printf("invalid input\n");
			continue;
		}

		if (meshIndex >= meshes.getSize())
		{
			printf("invalid index\n");
			continue;
		}

		break;
	}

	MeshData& mesh = meshes[meshIndex];
	
	if (mesh.normalCount != mesh.indexCount * 3)
		printf("not supported number of normals");

	{
		File file;
		file.open("output.xegeometry", FileAccessMode::Write, FileOpenMode::Override);

		{
			FileHeader header;
			header.signature = FileSignature;
			header.version = SupportedFileVersion;
			header.vertexStride = sizeof(VertexBase);
			header.vertexCount = mesh.indexCount;
			header.indexCount = mesh.indexCount;

			file.write(header);
		}

		{
			HeapPtr<VertexBase> vertices(mesh.indexCount);
			for (uint32 i = 0; i < mesh.indexCount; i++)
			{
				sint32 index = mesh.indices[i];
				if (i % 3 == 2)
				{
					index++;
					index = -index;
				}

				if (uint32(index) >= mesh.vertexCount)
				{
					printf("invalid index\n");
					return;
				}

				VertexBase &vertex = vertices[i];
				vertex.position.x = mesh.vertices[index * 3 + 0];
				vertex.position.y = mesh.vertices[index * 3 + 1];
				vertex.position.z = mesh.vertices[index * 3 + 2];
				vertex.normal.x = mesh.normals[i * 3 + 0];
				vertex.normal.y = mesh.normals[i * 3 + 1];
				vertex.normal.z = mesh.normals[i * 3 + 2];
			}

			file.write(vertices, sizeof(VertexBase) * mesh.indexCount);
		}

		{
			HeapPtr<uint32> indices(mesh.indexCount);
			for (uint32 i = 0; i < mesh.indexCount; i++)
				indices[i] = i;

			file.write(indices, sizeof(uint32) * mesh.indexCount);
		}

		file.close();
	}
}