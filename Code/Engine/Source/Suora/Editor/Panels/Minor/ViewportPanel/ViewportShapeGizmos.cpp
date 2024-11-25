#include "Precompiled.h"
#include "ViewportModules.h"

#include "Suora/GameFramework/World.h"
#include "Suora/GameFramework/Nodes/CameraNode.h"
#include "Suora/GameFramework/Nodes/ShapeNodes.h"
#include "Suora/GameFramework/Nodes/CharacterNode.h"
#include "Suora/Renderer/Renderer3D.h"
#include "Suora/Renderer/RenderCommand.h"
#include "Suora/Assets/AssetManager.h"
#include "Suora/Assets/Mesh.h"
#include "Suora/Assets/Texture2D.h"
#include "Suora/Assets/Material.h"

namespace Suora
{
	static Color s_ShapeColor   = Color(1, 0, 0, 1);
	static Color s_TriggerColor = Color(0, 1, 0, 1);

	static void Draw3DBox(CameraNode* InCamera, BoxShapeNode* InNode)
	{
		Array<std::pair<Vec3, Vec3>> lines;

		lines.Add({ Vec3(-1, -1, -1), Vec3(-1, -1, +1) });
		lines.Add({ Vec3(-1, +1, -1), Vec3(-1, +1, +1) });
		lines.Add({ Vec3(-1, -1, -1), Vec3(-1, +1, -1) });
		lines.Add({ Vec3(-1, -1, +1), Vec3(-1, +1, +1) });

		lines.Add({ Vec3(+1, -1, -1), Vec3(+1, -1, +1) });
		lines.Add({ Vec3(+1, +1, -1), Vec3(+1, +1, +1) });
		lines.Add({ Vec3(+1, -1, -1), Vec3(+1, +1, -1) });
		lines.Add({ Vec3(+1, -1, +1), Vec3(+1, +1, +1) });

		lines.Add({ Vec3(-1, -1, -1), Vec3(+1, -1, -1) });
		lines.Add({ Vec3(-1, +1, -1), Vec3(+1, +1, -1) });
		lines.Add({ Vec3(-1, -1, +1), Vec3(+1, -1, +1) });
		lines.Add({ Vec3(-1, +1, +1), Vec3(+1, +1, +1) });

		Node3D proxy;
		proxy.SetPosition(InNode->GetPosition());
		proxy.SetRotation(InNode->GetRotation());
		proxy.SetScale(InNode->GetBoxExtends());

		for (const std::pair<Vec3, Vec3>& line : lines)
		{
			const Vec4 a = proxy.GetTransformMatrix() * Vec4(line.first, 1);
			const Vec4 b = proxy.GetTransformMatrix() * Vec4(line.second, 1);
			Renderer3D::DrawLine3D(InCamera, a, b, InNode->IsTrigger() ? s_TriggerColor : s_ShapeColor);
		}
	}

	static Array<std::pair<Vec3, Vec3>> GenerateSphereTop(float InScale)
	{
		Array<std::pair<Vec3, Vec3>> lines;

		lines.Add({ Vec3(-1.f, 0, 0),     Vec3(-.75f, .25f, 0) });
		lines.Add({ Vec3(-.75f, .25f, 0), Vec3(-.5f, .5f, 0) });
		lines.Add({ Vec3(-.5f, .5f, 0),   Vec3(-.25f, .75f, 0) });
		lines.Add({ Vec3(-.25f, .75f, 0), Vec3(0, 1.f, 0) });

		lines.Add({ Vec3(+1.f, 0, 0),     Vec3(+.75f, .25f, 0) });
		lines.Add({ Vec3(+.75f, .25f, 0), Vec3(+.5f, .5f, 0) });
		lines.Add({ Vec3(+.5f, .5f, 0),   Vec3(+.25f, .75f, 0) });
		lines.Add({ Vec3(+.25f, .75f, 0), Vec3(0, 1.f, 0) });

		lines.Add({ Vec3(0, 0, -1.f),     Vec3(0, .25f, -.75f) });
		lines.Add({ Vec3(0, .25f, -.75f), Vec3(0, .5f, -.5f) });
		lines.Add({ Vec3(0, .5f, -.5f),   Vec3(0, .75f, -.25f) });
		lines.Add({ Vec3(0, .75f, -.25f), Vec3(0, 1.f, 0) });

		lines.Add({ Vec3(0, 0, +1.f),     Vec3(0, .25f, +.75f) });
		lines.Add({ Vec3(0, .25f, +.75f), Vec3(0, .5f, +.5f) });
		lines.Add({ Vec3(0, .5f, +.5f),   Vec3(0, .75f, +.25f) });
		lines.Add({ Vec3(0, .75f, +.25f), Vec3(0, 1.f, 0) });


		lines.Add({ Vec3(-1.f, 0, 0),     Vec3(-.75f, 0, +.25f) });
		lines.Add({ Vec3(-.75f, 0, +.25f),Vec3(-.5f, 0, +.5f) });
		lines.Add({ Vec3(-.5f, 0, +.5f),  Vec3(-.25f, 0, +.75f) });
		lines.Add({ Vec3(-.25f, 0, +.75f),Vec3(0, 0, +1.f) });

		lines.Add({ Vec3(0, 0, +1.f),     Vec3(+.25f, 0, +.75f) });
		lines.Add({ Vec3(+.25f, 0, +.75f),Vec3(+.5f, 0, +.5f) });
		lines.Add({ Vec3(+.5f, 0, +.5f),  Vec3(+.75f, 0, +.25f) });
		lines.Add({ Vec3(+.75f, 0, +.25f),Vec3(+1.f, 0, 0) });

		lines.Add({ Vec3(+1.f, 0, 0),     Vec3(+.75f, 0, -.25f) });
		lines.Add({ Vec3(+.75f, 0, -.25f),Vec3(+.5f, 0, -.5f) });
		lines.Add({ Vec3(+.5f, 0, -.5f),  Vec3(+.25f, 0, -.75f) });
		lines.Add({ Vec3(+.25f, 0, -.75f),Vec3(0, 0, -1.f) });

		lines.Add({ Vec3(0, 0, -1.f),Vec3(-.25f, 0, -.75f) });
		lines.Add({ Vec3(-.25f, 0, -.75f),Vec3(-.5f, 0, -.5f) });
		lines.Add({ Vec3(-.5f, 0, -.5f),Vec3(-.75f, 0, -.25f) });
		lines.Add({ Vec3(-.75f, 0, -.25f),Vec3(-1.f, 0, 0) });


		for (auto& line : lines)
		{
			line.first = glm::normalize(line.first) * InScale;
			line.second = glm::normalize(line.second) * InScale;
		}

		return lines;
	}
	static void Draw3DSphere(CameraNode* InCamera, SphereShapeNode* InNode)
	{
		Array<std::pair<Vec3, Vec3>> lines = GenerateSphereTop(1.0f);
		Array<std::pair<Vec3, Vec3>> lines2 = GenerateSphereTop(-1.0f);
		lines += lines2;

		Node3D proxy;
		proxy.SetPosition(InNode->GetPosition());
		proxy.SetRotation(InNode->GetRotation());
		proxy.SetScale(Vec::One * InNode->GetSphereRadius());

		for (const std::pair<Vec3, Vec3>& line : lines)
		{
			const Vec4 a = proxy.GetTransformMatrix() * Vec4(line.first, 1);
			const Vec4 b = proxy.GetTransformMatrix() * Vec4(line.second, 1);
			Renderer3D::DrawLine3D(InCamera, a, b, InNode->IsTrigger() ? s_TriggerColor : s_ShapeColor);
		}
	}

	static void Draw3DCapsule(CameraNode* InCamera, CapsuleShapeNode* InNode)
	{
		const float HalfHeight = InNode->GetCapsuleHeight() / 2.0f;
		Array<std::pair<Vec3, Vec3>> lines = GenerateSphereTop(InNode->GetCapsuleRadius());
		for (auto& line : lines)
		{
			line.first.y  += HalfHeight;
			line.second.y += HalfHeight;
		}
		Array<std::pair<Vec3, Vec3>> lines2 = GenerateSphereTop(InNode->GetCapsuleRadius() * -1.0f);
		for (auto& line : lines2)
		{
			line.first.y  -= HalfHeight;
			line.second.y -= HalfHeight;
		}
		lines += lines2;

		// Connecting both half spheres
		lines.Add({ Vec3(-InNode->GetCapsuleRadius(), -HalfHeight, 0), Vec3(-InNode->GetCapsuleRadius(), +HalfHeight, 0) });
		lines.Add({ Vec3(+InNode->GetCapsuleRadius(), -HalfHeight, 0), Vec3(+InNode->GetCapsuleRadius(), +HalfHeight, 0) });
		lines.Add({ Vec3(0, -HalfHeight, -InNode->GetCapsuleRadius()), Vec3(0, +HalfHeight, -InNode->GetCapsuleRadius()) });
		lines.Add({ Vec3(0, -HalfHeight, +InNode->GetCapsuleRadius()), Vec3(0, +HalfHeight, +InNode->GetCapsuleRadius()) });

		Node3D proxy;
		proxy.SetPosition(InNode->GetPosition());
		proxy.SetRotation(InNode->GetRotation());
		proxy.SetScale(Vec::One);

		for (const std::pair<Vec3, Vec3>& line : lines)
		{
			const Vec4 a = proxy.GetTransformMatrix() * Vec4(line.first, 1);
			const Vec4 b = proxy.GetTransformMatrix() * Vec4(line.second, 1);
			Renderer3D::DrawLine3D(InCamera, a, b, InNode->IsTrigger() ? s_TriggerColor : s_ShapeColor);
		}
	}

	static void Draw3DCharacter(CameraNode* InCamera, CharacterNode* InNode)
	{
		const float HalfHeight = InNode->GetCapsuleHeight() / 2.0f;
		Array<std::pair<Vec3, Vec3>> lines = GenerateSphereTop(InNode->GetCapsuleRadius());
		for (auto& line : lines)
		{
			line.first.y += HalfHeight;
			line.second.y += HalfHeight;
		}
		Array<std::pair<Vec3, Vec3>> lines2 = GenerateSphereTop(InNode->GetCapsuleRadius() * -1.0f);
		for (auto& line : lines2)
		{
			line.first.y -= HalfHeight;
			line.second.y -= HalfHeight;
		}
		lines += lines2;

		// Connecting both half spheres
		lines.Add({ Vec3(-InNode->GetCapsuleRadius(), -HalfHeight, 0), Vec3(-InNode->GetCapsuleRadius(), +HalfHeight, 0) });
		lines.Add({ Vec3(+InNode->GetCapsuleRadius(), -HalfHeight, 0), Vec3(+InNode->GetCapsuleRadius(), +HalfHeight, 0) });
		lines.Add({ Vec3(0, -HalfHeight, -InNode->GetCapsuleRadius()), Vec3(0, +HalfHeight, -InNode->GetCapsuleRadius()) });
		lines.Add({ Vec3(0, -HalfHeight, +InNode->GetCapsuleRadius()), Vec3(0, +HalfHeight, +InNode->GetCapsuleRadius()) });

		Node3D proxy;
		proxy.SetPosition(InNode->GetPosition());
		proxy.SetRotation(InNode->GetRotation());
		proxy.SetScale(Vec::One);

		for (const std::pair<Vec3, Vec3>& line : lines)
		{
			const Vec4 a = proxy.GetTransformMatrix() * Vec4(line.first, 1);
			const Vec4 b = proxy.GetTransformMatrix() * Vec4(line.second, 1);
			Renderer3D::DrawLine3D(InCamera, a, b, Color(1, 0, 0.5f, 1));
		}
	}

	void ViewportShapeGizmos::DrawDebugGizmos(World* world, CameraNode* camera, int* pickingID, Map<int, Node*>* pickingMap, bool isHandlingMousePick)
	{
		Array<BoxShapeNode*> boxes = world->FindNodesByClass<BoxShapeNode>();
		for (BoxShapeNode* box : boxes)
		{
			Draw3DBox(camera, box);
		}
		Array<SphereShapeNode*> spheres = world->FindNodesByClass<SphereShapeNode>();
		for (SphereShapeNode* sphere : spheres)
		{
			Draw3DSphere(camera, sphere);
		}
		Array<CapsuleShapeNode*> capsules = world->FindNodesByClass<CapsuleShapeNode>();
		for (CapsuleShapeNode* capsule : capsules)
		{
			Draw3DCapsule(camera, capsule);
		}

		Array<CharacterNode*> characters = world->FindNodesByClass<CharacterNode>();
		for (CharacterNode* character : characters)
		{
			Draw3DCharacter(camera, character);
		}
	}

}