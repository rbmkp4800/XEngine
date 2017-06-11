#include "XEngine.FBXImporter.Parser.h"

using namespace XEngine::FBXImporter;

// nodes parser =============================================================================//

inline bool isAlpabet(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

inline float32 tokenToFloat(const Token& token)
{
	if (token.code == TokenCode::Float)
		return token.floatValue;
	if (token.code == TokenCode::Int)
		return float32(token.intValue);
	return 0.0f;
}

inline bool canBeProperty(TokenCode code)
{
	return code == TokenCode::Int || code == TokenCode::Float ||
		code == TokenCode::String || isAlpabet(char(code));
}

ParserResult Parser::stateHandler_nodeNameExpected(const Token& token)
{
	if (token.code == TokenCode('}'))
	{
		if (!dataProducer.popSubnodesLevel())
			return ParserResult::BracesDoNotMatch;

		currentStateHandler = &Parser::stateHandler_nodeNameExpected;
		return ParserResult::Success;
	}

	if (token.code == TokenCode::None)
	{
		currentStateHandler = nullptr;
		if (!dataProducer.checkRootNode())
			return ParserResult::BracesDoNotMatch;
		return ParserResult::Success;
	}

	if (token.code != TokenCode::Inentifier)
		return ParserResult::NodeNameExpected;

	dataProducer.pushNode(token.string);

	currentStateHandler = &Parser::stateHandler_colonExpected;
	return ParserResult::Success;
}
ParserResult Parser::stateHandler_colonExpected(const Token& token)
{
	if (token.code != TokenCode(':'))
		return ParserResult::ColonExpected;

	currentStateHandler = &Parser::stateHandler_colonConsumed;
	return ParserResult::Success;
}
ParserResult Parser::stateHandler_colonConsumed(const Token& token)
{
	if (token.code == TokenCode('{'))
	{
		dataProducer.pushSubnodesLevel();
		currentStateHandler = &Parser::stateHandler_nodeNameExpected;
		return ParserResult::Success;
	}

	if (canBeProperty(token.code))
	{
		dataProducer.pushProperty(token);
		currentStateHandler = &Parser::stateHandler_propertyConsumed;
		return ParserResult::Success;
	}

	return stateHandler_nodeNameExpected(token);
}
ParserResult Parser::stateHandler_propertyExpected(const Token& token)
{
	if (!canBeProperty(token.code))
		return ParserResult::PropertyExpected;

	dataProducer.pushProperty(token);
	currentStateHandler = &Parser::stateHandler_propertyConsumed;
	return ParserResult::Success;
}
ParserResult Parser::stateHandler_propertyConsumed(const Token& token)
{
	if (token.code == TokenCode(','))
	{
		currentStateHandler = &Parser::stateHandler_propertyExpected;
		return ParserResult::Success;
	}
	else if (token.code == TokenCode('{'))
	{
		dataProducer.pushSubnodesLevel();
		currentStateHandler = &Parser::stateHandler_nodeNameExpected;
		return ParserResult::Success;
	}

	return stateHandler_nodeNameExpected(token);
}

// data producer ============================================================================//

enum class Parser::DataProducer::NodeType : uint8
{
	None = 0,
	Unknown,
	Objects,
	Model,
	Model_Mesh,
	Vertices,
	Indices,
	LayerElementNormal,
	Normals,
};

Parser::DataProducer::DataProducer()
{
	nodeStack.pushBack(NodeType::Unknown);
}

inline void Parser::DataProducer::pushNode(const String& name)
{
	if (nodeStack.back() == NodeType::Model_Mesh)
	{
		MeshData& mesh = meshes.allocateBack();
		mesh.name = modelName;

		mesh.vertexCount = vertices.getSize();
		mesh.indexCount = indices.getSize();
		mesh.normalCount = normals.getSize();

		mesh.vertices = vertices.takeBuffer();
		mesh.indices = indices.takeBuffer();
		mesh.normals = normals.takeBuffer();
	}
	nodeStack.popBack();

	NodeType type = NodeType::Unknown;
	if (nodeStack.isEmpty())
	{
		if (name.equals("Objects"))
			type = NodeType::Objects;
	}
	else
	{
		NodeType parentType = nodeStack.back();

		if (parentType == NodeType::Objects)
		{
			if (name.equals("Model"))
				type = NodeType::Model;
		}
		else if (parentType == NodeType::Model_Mesh)
		{
			if (name.equals("Vertices"))
			{
				if (vertices.isEmpty())
					type = NodeType::Vertices;
			}
			else if (name.equals("PolygonVertexIndex"))
			{
				if (indices.isEmpty())
					type = NodeType::Indices;
			}
			else if (name.equals("LayerElementNormal"))
				type = NodeType::LayerElementNormal;
		}
		else if (parentType == NodeType::LayerElementNormal)
		{
			if (name.equals("Normals"))
			{
				if (normals.isEmpty())
					type = NodeType::Normals;
			}
		}
	}

	nodeStack.pushBack(type);
	propertyIndex = 0;
}
inline void Parser::DataProducer::pushProperty(const Token& token)
{
	NodeType type = nodeStack.back();
	if (type == NodeType::Model)
	{
		if (propertyIndex == 0 && token.code == TokenCode::String)
			modelName = token.string;
		if (propertyIndex == 1 && token.code == TokenCode::String && token.string.equals("Mesh"))
		{
			nodeStack.back() = NodeType::Model_Mesh;

			vertices.clear();
			indices.clear();
			normals.clear();
		}
	}
	else if (type == NodeType::Vertices)
		vertices.pushBack(tokenToFloat(token));
	else if (type == NodeType::Indices)
		indices.pushBack(token.code == TokenCode::Int ? token.intValue : 0);
	else if (type == NodeType::Normals)
		normals.pushBack(tokenToFloat(token));

	propertyIndex++;
}
inline void Parser::DataProducer::pushSubnodesLevel()
{
	nodeStack.pushBack(NodeType::None);
}
inline bool Parser::DataProducer::popSubnodesLevel()
{
	if (nodeStack.getSize() == 1)
		return false;

	nodeStack.popBack();
	return true;
}
inline bool Parser::DataProducer::checkRootNode()
{
	return nodeStack.getSize() == 1;
}