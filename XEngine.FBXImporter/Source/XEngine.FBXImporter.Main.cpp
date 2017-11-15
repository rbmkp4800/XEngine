#include <stdio.h>
#include <fbxsdk.h>

#include <XLib.Types.h>
#include <XLib.Util.h>
#include <XLib.Heap.h>
#include <XLib.System.File.h>
#include <XLib.Crypto.CRC.h>
#include <XLib.Algorithm.QuickSort.h>

#include <XEngine.Render.Vertices.h>
#include <XEngine.Formats.XEGeometry.h>

using namespace XLib;
using namespace XEngine;
using namespace XEngine::Formats;

template <typename TargetType, typename SourceType>
inline TargetType FBXConvertValue(const SourceType& value)
{
	static_assert(false, "invalid conversion");
}

template <>
inline float32x3 FBXConvertValue<float32x3, FbxVector4>(const FbxVector4& value)
{
	return float32x3
	(
		float32(value.mData[0]),
		float32(value.mData[1]),
		float32(value.mData[2])
	);
}

template <>
inline float32x2 FBXConvertValue<float32x2, FbxVector2>(const FbxVector2& value)
{
	return float32x2
	(
		float32(value.mData[0]),
		float32(value.mData[1])
	);
}

template <typename TargetComponentType, typename FbxLayerElementType, typename VertexType>
static bool FillVertexBufferComponent(const FbxLayerElementTemplate<FbxLayerElementType>* fbxLayerElement,
	VertexType* vertexBuffer, uint32 polygonVertexCount, uint32 targetComponentOffset)
{
	FbxLayerElement::EMappingMode fbxMappingMode = fbxLayerElement->GetMappingMode();
	FbxLayerElement::EReferenceMode fbxReferenceMode = fbxLayerElement->GetReferenceMode();

	if (fbxMappingMode != FbxLayerElement::EMappingMode::eByPolygonVertex)
	{
		printf("mapping mode not supported (%d)\n", uint32(fbxMappingMode));
		return false;
	}

	if (fbxReferenceMode == FbxLayerElement::EReferenceMode::eDirect)
	{
		FbxLayerElementArrayTemplate<FbxLayerElementType> &fbxDirect = fbxLayerElement->GetDirectArray();

		if (fbxDirect.GetCount() != polygonVertexCount)
		{
			printf("invalid layer element value count\n");
			return false;
		}

		for (uint32 i = 0; i < polygonVertexCount; i++)
		{
			TargetComponentType &target =
				*to<TargetComponentType*>(to<byte*>(vertexBuffer + i) + targetComponentOffset);
			target = FBXConvertValue<TargetComponentType>(fbxDirect.GetAt(i));
		}
	}
	else if (fbxReferenceMode == FbxLayerElement::EReferenceMode::eIndexToDirect)
	{
		FbxLayerElementArrayTemplate<FbxLayerElementType> &fbxDirect = fbxLayerElement->GetDirectArray();
		FbxLayerElementArrayTemplate<int> &fbxIndex = fbxLayerElement->GetIndexArray();

		if (fbxIndex.GetCount() != polygonVertexCount)
		{
			printf("invalid layer element index count\n");
			return false;
		}

		for (uint32 i = 0; i < polygonVertexCount; i++)
		{
			TargetComponentType &target =
				*to<TargetComponentType*>(to<byte*>(vertexBuffer + i) + targetComponentOffset);
			target = FBXConvertValue<TargetComponentType>(fbxDirect.GetAt(fbxIndex.GetAt(i)));
		}
	}
	else
	{
		printf("reference mode not supported (%d)\n", uint32(fbxReferenceMode));
		return false;
	}

	return true;
}

template <typename VertexType>
static uint32 RemoveVertexDuplicatesAndFillIndexBuffer(
	VertexType* vertexBuffer, uint32* indexBuffer, uint32 totalVertexCount)
{
	// TODO: refactor, write explanations

	if (totalVertexCount & 0xFF000000)
	{
		printf("vertex limit exceeded\n");
		return 0;
	}

	HeapPtr<uint64> hashedVertices(totalVertexCount);
	HeapPtr<uint32> sparseVertexBufferIndices(totalVertexCount);
	HeapPtr<uint32> vertexBufferCompactionIndices(totalVertexCount);

	for (uint32 i = 0; i < totalVertexCount; i++)
	{
		uint64 hash = uint64(CRC32::Compute(vertexBuffer[i])) << 32;
		hash |= uint32(CRC8::Compute(vertexBuffer[i])) << 24;
		hash |= i;
		hashedVertices[i] = hash;
	}

	QuickSort(to<uint64*>(hashedVertices), totalVertexCount);
	
	for (uint32 i = 0; i < totalVertexCount;)
	{
		uint32 blockBegin = i;
		uint64 hash = hashedVertices[i] & 0xFFFFFFFF'FF000000;
		do
		{
			i++;
		} while ((hashedVertices[i] & 0xFFFFFFFF'FF000000) == hash && i < totalVertexCount);

		uint32 firstVertexIndex = hashedVertices[blockBegin] & 0xFFFFFF;
		sparseVertexBufferIndices[firstVertexIndex] = firstVertexIndex;
		for (uint32 j = blockBegin + 1; j < i; j++)
		{
			uint32 targetIndex = hashedVertices[j] & 0xFFFFFF;
			sparseVertexBufferIndices[targetIndex] = firstVertexIndex;
		}
	}

	uint32 compactVertexCount = 0;
	for (uint32 i = 0; i < totalVertexCount; i++)
	{
		if (sparseVertexBufferIndices[i] == i)
		{
			vertexBuffer[compactVertexCount] = vertexBuffer[i];
			vertexBufferCompactionIndices[i] = compactVertexCount;
			compactVertexCount++;
		}
		indexBuffer[i] = vertexBufferCompactionIndices[sparseVertexBufferIndices[i]];
	}

	return compactVertexCount;
}

static void SaveMesh(FbxMesh *fbxMesh)
{
	if (!fbxMesh->IsTriangleMesh())
	{
		printf("mesh is not triangle\n");
		return;
	}

	printf("converting geometry...\n");

	uint32 polygonVertexCount = fbxMesh->GetPolygonVertexCount();
	HeapPtr<VertexTexture> vertexBuffer(polygonVertexCount);

	FbxVector4 *fbxPositions = fbxMesh->GetControlPoints();
	int *fbxIndices = fbxMesh->GetPolygonVertices();
	for (uint32 i = 0; i < polygonVertexCount; i++)
		vertexBuffer[i].position = FBXConvertValue<float32x3>(fbxPositions[fbxIndices[i]]);

	FbxGeometryElementNormal *fbxNormalElement = fbxMesh->GetElementNormal();
	if (!fbxNormalElement)
	{
		printf("mesh has no normals\n");
		return;
	}

	if (!FillVertexBufferComponent<float32x3, FbxVector4, VertexTexture>(fbxNormalElement,
			vertexBuffer, polygonVertexCount, offsetof(VertexTexture, normal)))
	{
		printf("error converting normals\n");
		return;
	}

	FbxGeometryElementUV *fbxUVElement = fbxMesh->GetElementUV();
	if (!fbxUVElement)
	{
		printf("mesh has no UVs\n");
		return;
	}

	if (!FillVertexBufferComponent<float32x2, FbxVector2, VertexTexture>(fbxUVElement,
			vertexBuffer, polygonVertexCount, offsetof(VertexTexture, texture)))
	{
		printf("error converting UVs\n");
		return;
	}

	printf("compacting vertex buffer...\n");

	HeapPtr<uint32> indexBuffer(polygonVertexCount);
	uint32 compactVertexCount = RemoveVertexDuplicatesAndFillIndexBuffer(
		to<VertexTexture*>(vertexBuffer), to<uint32*>(indexBuffer), polygonVertexCount);

	/*uint32 compactVertexCount = polygonVertexCount;
	for (uint32 i = 0; i < polygonVertexCount; i++)
		indexBuffer[i] = i;*/

	if (!compactVertexCount)
	{
		printf("error compacting vertex buffer\n");
		return;
	}

	printf("vertex count = %u'%03u, index count = %u'%03u\n",
		compactVertexCount / 1000, compactVertexCount % 1000,
		polygonVertexCount / 1000, polygonVertexCount % 1000);

	File outputFile;
	if (!outputFile.open("output.xegeometry", FileAccessMode::Write, FileOpenMode::Override))
	{
		printf("error writing file\n");
		return;
	}

	XEGeometryFile::Header header;
	header.magic = XEGeometryFile::Magic;
	header.version = XEGeometryFile::SupportedVersion;
	header.vertexStride = sizeof(VertexTexture);
	header.vertexCount = compactVertexCount;
	header.indexCount = polygonVertexCount;
	outputFile.write(header);
	outputFile.write(vertexBuffer, sizeof(VertexTexture) * compactVertexCount);
	outputFile.write(indexBuffer, sizeof(uint32) * polygonVertexCount);
	outputFile.close();

	printf("file converted\n");
}

int main(int argc, char** argv)
{
	//const char* lFilename = "F:\\model\\wood_stick_01.FBX";
	//const char* filename = "F:\\prom.FBX";
	const char* filename = "f:\\cube.FBX";
	//const char* filename = "f:\\x18.FBX";

	FbxManager* fbxSDKManager = FbxManager::Create();

	FbxIOSettings *fbxIOSettings = FbxIOSettings::Create(fbxSDKManager, IOSROOT);
	fbxSDKManager->SetIOSettings(fbxIOSettings);

	FbxImporter* fbxImporter = FbxImporter::Create(fbxSDKManager, "");
	if (!fbxImporter->Initialize(filename, -1, fbxSDKManager->GetIOSettings()))
	{
		printf("FbxImporter.Initialize failed: %s\n", fbxImporter->GetStatus().GetErrorString());
		return 0;
	}

	printf("reading file...\n");
	FbxScene *fbxScene = FbxScene::Create(fbxSDKManager, "scene");
	fbxImporter->Import(fbxScene);
	fbxImporter->Destroy();

	FbxNode *fbxRoot = fbxScene->GetRootNode();
	if (!fbxRoot)
	{
		printf("invalid format\n");
		return 0;
	}

	for (int i = 0; i < fbxRoot->GetChildCount(); i++)
	{
		FbxNode *fbxNode = fbxRoot->GetChild(i);
		FbxMesh *fbxMesh = fbxNode->GetMesh();
		if (!fbxMesh)
			continue;

		printf("mesh id %d\t\"%s\"\n", i, fbxNode->GetName());

		SaveMesh(fbxMesh);
		break;
	}

	return 0;
}