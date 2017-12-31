#include <stdio.h>
#include <fbxsdk.h>

#include <XLib.Types.h>
#include <XLib.Util.h>
#include <XLib.Heap.h>
#include <XLib.System.File.h>
#include <XLib.Crypto.CRC.h>
#include <XLib.Algorithm.QuickSort.h>
#include <XLib.Containers.Vector.h>
#include <XLib.Math.Matrix3x4.h>

#include <XEngine.Render.Vertices.h>
#include <XEngine.Formats.XEGeometry.h>

using namespace XLib;
using namespace XEngine;
using namespace XEngine::Formats;

struct SkeletonJoint
{
	FbxNode *fbxNode;
	uint16 parentId;
};

struct ControlPointSkinningInfo
{
	uint16 indices[4];
	float32 weights[4];
	uint8 normalizedWeights[4];

	inline bool pushIndexWeightPair(uint16 index, float32 weight)
	{
		if (weight <= 0.0f)
		{
			printf("invalid weight value\n");
			return false;
		}

		uint16 minWeightComponentIndex = 0;
		float32 minWeight = weights[0];
		for (uint32 i = 1; i < 4; i++)
		{
			if (minWeight > weights[i])
			{
				minWeight = weights[i];
				minWeightComponentIndex = i;
			}
		}

		indices[minWeightComponentIndex] = index;
		weights[minWeightComponentIndex] = weight;

		return true;
	}
	inline bool normalize()
	{
		for (uint32 i = 0; i < 4; i++)
		{
			for (uint32 j = i + 1; j < 4; j++)
			{
				if (weights[i] > weights[j])
				{
					swap(weights[i], weights[j]);
					swap(indices[i], indices[j]);
				}
			}
		}

		if (weights[0] == 0.0f)
		{
			printf("control point has no cluster associated\n");
			return false;
		}

		float32 coef = 255.0f / (weights[0] + weights[1] + weights[2] + weights[3]);
		for (uint32 i = 0; i < 4; i++)
			normalizedWeights[i] = uint8(weights[i] * coef + 0.5f);

		return true;
	}
};

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

template <>
inline Matrix3x4 FBXConvertValue<Matrix3x4, FbxAMatrix>(const FbxAMatrix& value)
{
	Matrix3x4 result;
	for (uint32 i = 0; i < 3; i++)
	{
		for (uint32 j = 0; j < 4; j++)
			result.data[i][j] = value.mData[j][i];
	}

	return result;
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
static uint32 RemoveVertexDuplicatesAndFillIndexBuffer(VertexType* vertexBuffer,
	uint32* indexBuffer, uint32 totalVertexCount)
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

static bool GenerateControlPointsSkinningInformation(FbxSkin* fbxSkin,
	ControlPointSkinningInfo* controlPoints, uint32 controlPointCount,
	const SkeletonJoint* joints, uint16 jointCount)
{
	for (sint32 clusterIndex = 0; clusterIndex < fbxSkin->GetClusterCount(); clusterIndex++)
	{
		FbxCluster *fbxCluster = fbxSkin->GetCluster(clusterIndex);

		uint16 jointId = uint16(-1);
		FbxNode *fbxLinkNode = fbxCluster->GetLink();
		for (uint16 i = 0; i < jointCount; i++)
		{
			if (joints[i].fbxNode == fbxLinkNode)
			{
				jointId = i;
				break;
			}
		}

		if (jointId == uint16(-1))
		{
			printf("cluster link node not found in skeleton\n");
			return false;
		}

		uint32 indexCount = fbxCluster->GetControlPointIndicesCount();
		int *indices = fbxCluster->GetControlPointIndices();
		double *weights = fbxCluster->GetControlPointWeights();
		for (uint32 i = 0; i < indexCount; i++)
		{
			uint32 index = indices[i];
			if (index >= controlPointCount)
			{
				printf("invalid cluster control point index\n");
				return false;
			}

			if (!controlPoints[i].pushIndexWeightPair(index, float32(weights[i])))
				return false;
		}
	}

	return true;
}

static void GenerateSkeletonJointArrayFromSceneHierarchy(FbxNode* fbxNode,
	Vector<SkeletonJoint>& joints, uint16 parentJointId = uint16(-1))
{
	uint16 thisJointId = uint16(-1);

	if (fbxNode->GetSkeleton())
	{
		SkeletonJoint joint;
		joint.fbxNode = fbxNode;
		joint.parentId = parentJointId;
		thisJointId = uint16(joints.getSize());
		joints.pushBack(joint);
	}
	else if (parentJointId != uint16(-1))
	{
		// non skeleton node in skeleton
		// just skip subtree
		return;
	}

	for (sint32 i = 0; i < fbxNode->GetChildCount(); i++)
		GenerateSkeletonJointArrayFromSceneHierarchy(fbxNode->GetChild(i), joints, thisJointId);
}

static void SaveMesh(FbxMesh* fbxMesh, SkeletonJoint* joints, uint16 jointCount)
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

	FbxSkin *fbxSkin = to<FbxSkin*>(fbxMesh->GetDeformer(0, FbxDeformer::EDeformerType::eSkin));

	if (joints && fbxSkin)
	{
		uint32 controlPointCount = fbxMesh->GetControlPointsCount();
		HeapPtr<ControlPointSkinningInfo> controlPointSkinningInfoArray(controlPointCount);
		GenerateControlPointsSkinningInformation(fbxSkin, controlPointSkinningInfoArray,
			controlPointCount, joints, jointCount);

		for (uint32 i = 0; i < polygonVertexCount; i++)
		{
			vertexBuffer[i].position = FBXConvertValue<float32x3>(fbxPositions[fbxIndices[i]]);
			vertexBuffer[i].
				// resume here
		}
	}
	else
	{
		for (uint32 i = 0; i < polygonVertexCount; i++)
			vertexBuffer[i].position = FBXConvertValue<float32x3>(fbxPositions[fbxIndices[i]]);
	}

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

	if (!compactVertexCount)
	{
		printf("error compacting vertex buffer\n");
		return;
	}

	printf("vertex count = %u'%03u, index count = %u'%03u\n",
		compactVertexCount / 1000, compactVertexCount % 1000,
		polygonVertexCount / 1000, polygonVertexCount % 1000);

	File file;
	if (!file.open("output.xegeometry", FileAccessMode::Write, FileOpenMode::Override))
	{
		printf("error writing file\n");
		return;
	}

	XEGeometryFile::Header header;
	header.magic = XEGeometryFile::Magic;
	header.version = XEGeometryFile::SupportedVersion;
	header.vertexType = XEGeometryFile::VertexType::Textured;
	header.vertexCount = compactVertexCount;
	header.indexCount = polygonVertexCount;
	file.write(header);
	file.write(vertexBuffer, sizeof(VertexTexture) * compactVertexCount);
	file.write(indexBuffer, sizeof(uint32) * polygonVertexCount);
	file.close();

	printf("file converted\n");
}

static bool SaveSkeleton(const char* filename, SkeletonJoint* joints, uint16 jointCount)
{
	File file;
	if (!file.open(filename, FileAccessMode::Write, FileOpenMode::Override))
	{
		printf("error opening file\n");
		return;
	}

	XESkeletonFile::Header header;
	header.magic = XESkeletonFile::Magic;
	header.version = XESkeletonFile::SupportedVersion;
	header.jointCount = jointCount;
	file.write(header);

	for (uint16 i = 0; i < jointCount; i++)
	{
		FbxAMatrix fbxInverseBindPoseTransform = joints[i].fbxNode->EvaluateGlobalTransform().Inverse();
		file.write(FBXConvertValue<Matrix3x4>(fbxInverseBindPoseTransform));
	}

	file.close();

	return true;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("no input file\n");
		return 0;
	}

	const char* filename = argv[1];

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
	if (!fbxImporter->Import(fbxScene))
	{
		printf("error importing file");
		return 0;
	}
	fbxImporter->Destroy();

	FbxNode *fbxRoot = fbxScene->GetRootNode();
	if (!fbxRoot)
	{
		printf("root node is null\n");
		return 0;
	}

	Vector<SkeletonJoint> skeletonJoints;
	GenerateSkeletonJointArrayFromSceneHierarchy(fbxRoot, skeletonJoints);

	if (!skeletonJoints.isEmpty())
	{
		for (auto joint = skeletonJoints.begin() + 1; joint < skeletonJoints.end(); joint++)
		{
			if (joint->parentId == uint16(-1))
			{
				printf("multiple skeleton roots\n");
				return 0;
			}
		}
	}

	printf("contents:\n");
	if (!skeletonJoints.isEmpty())
		printf("\tskeleton \"%s\"\n", skeletonJoints[0].fbxNode->GetName());

	for (sint32 i = 0; i < fbxRoot->GetChildCount(); i++)
	{
		FbxNode *fbxNode = fbxRoot->GetChild(i);
		if (fbxNode->GetMesh())
			printf("\tmesh %d \"%s\"\n", i, fbxNode->GetName());
	}

	//FbxAnimStack *fbxAnimStack = fbxScene->GetSrcObject<FbxAnimStack>(0);
	//printf("%s\n", fbxAnimStack->GetName());
	//FbxTakeInfo* fbxTakeInfo = fbxScene->GetTakeInfo(fbxAnimStack->GetName());

	return 0;
}