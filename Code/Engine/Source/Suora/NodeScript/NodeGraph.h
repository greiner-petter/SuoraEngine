#pragma once

#include <string>
#include <array>
#include <glm/glm.hpp>
#include "inttypes.h"
#include "Suora/Serialization/Yaml.h"
#include <Suora.h>

namespace Suora
{
	class ScriptNode;
	struct VisualNode;

	struct VisualNodePin
	{
		VisualNodePin(VisualNode& node, const std::string& label, const glm::vec4& color, int64_t id, bool receive, float pinHeight = 25.0f)
			: m_Node(&node), Label(label), Color(color), PinID(id), IsReceivingPin(receive), PinHeight(pinHeight) { }
		std::string Label;
		glm::vec4 Color;
		VisualNodePin* Target = nullptr;
		bool IsReceivingPin = false;
		int64_t PinID = 0;
		float PinHeight = 25.0f;
		std::string m_AdditionalData = "";
		std::string Tooltip = "";

		bool operator==(const VisualNodePin& other) const
		{
			return (this == &other);
		}
		VisualNode* GetNode() const { return m_Node; }
		bool HasOtherPin(const Array<Ref<VisualNode>>& nodes);
	private:
		VisualNode* m_Node = nullptr;
		glm::vec2 PinConnectionPoint = glm::vec2(0.0f);
		friend class VisualNodeGraph;
		friend class NodeGraphEditor;
		friend struct VisualNodeSearchOverlay;
	};

	struct VisualNode
	{
		int64_t m_NodeID = 0;
		glm::vec2 m_Position = { 0, 0 };
		glm::vec2 m_Size = { 160, 100 };
		glm::vec4 m_Color = glm::vec4(0.25f, 0.25f, 0.555f, 1);
		glm::vec4 m_BackgroundColor = glm::vec4(0.05f, 0.05f, 0.055f, 0.9f);
		std::string m_Title = "VisualNode";
		Array<VisualNodePin> m_InputPins;
		Array<VisualNodePin> m_OutputPins;
		bool operator==(const VisualNode& other) const
		{
			return (this == &other);
		}
		void AddInputPin(const std::string& label, const glm::vec4& color, int64_t id, bool receive, float pinHeight = 25.0f)
		{
			m_InputPins.Add(VisualNodePin(*this, label, color, id, receive, pinHeight));
		}
		void AddOutputPin(const std::string& label, const glm::vec4& color, int64_t id, bool receive, float pinHeight = 25.0f)
		{
			m_OutputPins.Add(VisualNodePin(*this, label, color, id, receive, pinHeight));
		}
	};

	struct VisualNodeEntry
	{
		VisualNodeEntry(const Ref<VisualNode>& node)
			: m_Node(node) { }
		Ref<VisualNode> m_Node;
		Array<std::string> m_Tags;
	};

	struct VisualNodeGraph
	{
		Array<Ref<VisualNode>> m_Nodes;

		Array<VisualNodeEntry> m_SupportedNodes;
		void AddSupportedNode(const Ref<VisualNode>& node, const Array<std::string>& tags = {});
		virtual void UpdateSupportedNodes();

		virtual void TickAllVisualNodes();

		int IndexOf(VisualNode& node);

		void ClearNodePin(VisualNodePin& pin);
		void FixNodePins();
		void RemoveVisualNode(VisualNode& node);

		void SerializeNodeGraph(Yaml::Node& root);
		void DeserializeNodeGraph(Yaml::Node& root);
		std::string VisualNodePinToString(VisualNodePin& pin);
		void VisualNodePinFromString(const std::string& str, VisualNodePin& a);
	};

}