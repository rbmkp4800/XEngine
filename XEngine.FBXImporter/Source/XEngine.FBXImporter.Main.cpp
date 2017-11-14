#include <stdio.h>
#include <fbxsdk.h>

#include <XLib.Types.h>
#include <XLib.Util.h>
#include <XLib.Heap.h>
#include <XLib.System.File.h>

#include <XEngine.Render.Vertices.h>
#include <XEngine.Formats.XEGeometry.h>

using namespace XLib;
using namespace XEngine;
using namespace XEngine::Formats;

inline float32x3 FBXVector4ToFloat32x3(const FbxVector4& a)
{
	return float32x3
	(
		float32(a.mData[0]),
		float32(a.mData[1]),
		float32(a.mData[2])
	);
}

inline float32x2 FBXVector4ToFloat32x2(const FbxVector4& a)
{
	return float32x2
	(
		float32(a.mData[0]),
		float32(a.mData[1])
	);
}

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

static void SaveMesh(FbxMesh *fbxMesh)
{
	if (!fbxMesh->IsTriangleMesh())
	{
		printf("mesh is not triangle\n");
		return;
	}

	uint32 polygonVertexCount = fbxMesh->GetPolygonVertexCount();
	HeapPtr<VertexTexture> vertexBuffer(polygonVertexCount);

	FbxVector4 *fbxPositions = fbxMesh->GetControlPoints();
	int *fbxIndices = fbxMesh->GetPolygonVertices();
	for (uint32 i = 0; i < polygonVertexCount; i++)
		vertexBuffer[i].position = FBXVector4ToFloat32x3(fbxPositions[fbxIndices[i]]);

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

	File outputFile;
	if (!outputFile.open("output.xegeometry", FileAccessMode::Write, FileOpenMode::Override))
	{
		printf("error writing file\n");
		return;
	}

	HeapPtr<uint32> indexBuffer(polygonVertexCount);
	for (uint32 i = 0; i < polygonVertexCount; i++)
		indexBuffer[i] = i;

	XEGeometryFile::Header header;
	header.magic = XEGeometryFile::Magic;
	header.version = XEGeometryFile::SupportedVersion;
	header.vertexStride = sizeof(VertexTexture);
	header.vertexCount = polygonVertexCount;
	header.indexCount = polygonVertexCount;
	outputFile.write(header);
	outputFile.write(vertexBuffer, polygonVertexCount * sizeof(VertexTexture));
	outputFile.write(indexBuffer, polygonVertexCount * sizeof(uint32));
	outputFile.close();
}

int main(int argc, char** argv)
{
	//const char* lFilename = "F:\\model\\wood_stick_01.FBX";
	//const char* filename = "F:\\prom.FBX";
	const char* filename = "F:\\x18.FBX";
	FbxManager* fbxSDKManager = FbxManager::Create();

	FbxIOSettings *fbxIOSettings = FbxIOSettings::Create(fbxSDKManager, IOSROOT);
	fbxSDKManager->SetIOSettings(fbxIOSettings);

	FbxImporter* fbxImporter = FbxImporter::Create(fbxSDKManager, "");
	if (!fbxImporter->Initialize(filename, -1, fbxSDKManager->GetIOSettings()))
	{
		printf("FbxImporter.Initialize failed: %s\n", fbxImporter->GetStatus().GetErrorString());
		return 0;
	}

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