#pragma once

#include <XLib.Types.h>
#include <XLib.NonCopyable.h>
#include <XLib.Delegate.h>
#include <XLib.Heap.h>
#include <XLib.Containers.Vector.h>

#include "XEngine.FBXImporter.Token.h"

namespace XEngine::FBXImporter
{
	enum class ParserResult
	{
		Success = 0,

		BracesDoNotMatch,
		NodeNameExpected,
		ColonExpected,
		PropertyExpected,
	};

	struct MeshData
	{
		String name;
		uint32 vertexCount;
		uint32 indexCount;
		uint32 normalCount;
		XLib::HeapPtr<float32> vertices;
		XLib::HeapPtr<sint32> indices;
		XLib::HeapPtr<float32> normals;
	};

	class Parser : public XLib::NonCopyable
	{
	private:
		class DataProducer
		{
		private:
			enum class NodeType : uint8;

			uint32 propertyIndex = 0;
			XLib::Vector<NodeType> nodeStack;

			String modelName;
			XLib::Vector<float32> vertices;
			XLib::Vector<sint32> indices;

			XLib::Vector<MeshData> meshes;

		public:
			DataProducer();

			inline void pushNode(const String& name);
			inline void pushProperty(const Token& token);
			inline void pushSubnodesLevel();
			inline bool popSubnodesLevel();
			inline bool checkRootNode();

			inline XLib::Vector<MeshData> takeMeshes() { return move(meshes); }
		};

		using StateHandler = ParserResult(Parser::*)(const Token& token);

		ParserResult stateHandler_nodeNameExpected(const Token& token);
		ParserResult stateHandler_colonExpected(const Token& token);
		ParserResult stateHandler_colonConsumed(const Token& token);
		ParserResult stateHandler_propertyExpected(const Token& token);
		ParserResult stateHandler_propertyConsumed(const Token& token);
		StateHandler currentStateHandler = &Parser::stateHandler_nodeNameExpected;

		DataProducer dataProducer;

	public:
		inline ParserResult pushToken(const Token& token) { return (*this.*currentStateHandler)(token); }
		inline bool isFinalized() { return currentStateHandler == nullptr; }

		inline XLib::Vector<MeshData> takeMeshes() { return dataProducer.takeMeshes(); }
	};
}