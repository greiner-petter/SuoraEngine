#pragma once
#include <unordered_map>
#include "Suora/Core/Base.h"
#include "Suora/Common/VectorUtils.h"
#include "Suora/Common/Array.h"
#include "Suora/Common/Math.h"
#include "Suora/Core/Object/Pointer.h"
#include "Suora/Core/EngineSubSystem.h"
#include "PhysicsEngine.generated.h"

namespace Suora::Physics
{

	class PhysicsEngine : public EngineSubSystem
	{
		SUORA_CLASS(547893438543);
	public:
		static Ref<PhysicsEngine> Create();

		virtual void Initialize()
		{
		}

		virtual Ref<class PhysicsWorld> CreatePhysicsWorld();

	public:
		inline static Class s_PhysicsEngineClass = PhysicsEngine::StaticClass();

	};

}