#pragma once
#include "Suora/Physics/PhysicsWorld.h"
#include "JoltPhysicsWorld.generated.h"

namespace JPH
{
	class PhysicsSystem;
	class TempAllocator;
	class JobSystemThreadPool;

	class Body;
}

namespace Suora::Physics
{

	class JoltPhysicsWorld : public PhysicsWorld
	{
		SUORA_CLASS(8795643283);
	public:
		JoltPhysicsWorld();
		~JoltPhysicsWorld();
		virtual void CreateShapeNode(ShapeNode* node) override;
		virtual void DestroyShapeNode(ShapeNode* node) override;
		virtual void TickShapeNode(ShapeNode* node) override;

		virtual void Step(double timeStep) override;

		void Inititalize();

	private:
		Ref<JPH::PhysicsSystem> m_PhysicsSystem;
		Ref<JPH::TempAllocator> m_TempAllocator;
		Ref<JPH::JobSystemThreadPool> m_JobSystem;
		Ref<class BPLayerInterfaceImpl> m_BPLayerInterface;

		Ref<class ObjectVsBroadPhaseLayerFilter> m_ObjectVsBroadPhaseLayerFilter;
		Ref<class ObjectLayerPairFilter> m_ObjectLayerPairFilter;

		std::unordered_map<JPH::Body*, Suora::ShapeNode*> m_Body_Rigidbody;
		std::unordered_map<Suora::ShapeNode*, JPH::Body*> m_Rigidbody_Body;
	};

}