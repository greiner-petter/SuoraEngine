#pragma once

#include "NodeGraph.h"

namespace Suora
{

	struct ShaderGraphCompiler
	{
		static std::string CompileShaderNode(VisualNode& node, VisualNodePin& pin, bool vertex, bool& error);
		static std::string CompilePin(VisualNodePin& pin, bool vertex, bool& error);
	};

	struct ShaderNodeGraph : VisualNodeGraph
	{
		ShaderNodeGraph();

		void UpdateSupportedNodes() override;

		static ShaderGraphDataType StringToShaderGraphDataType(std::string str);
		static std::string ShaderGraphDataTypeToLabel(ShaderGraphDataType type);
		static std::string ShaderGraphDataTypeToString(ShaderGraphDataType type);
		static glm::vec4 GetShaderDataTypeColor(ShaderGraphDataType type);
	};

}