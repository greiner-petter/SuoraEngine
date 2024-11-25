#include "Precompiled.h"
#include "Engine.h"
#include "Application.h"
#include "NativeInput.h"

#include "Suora/Assets/AssetManager.h"
#include "Suora/Assets/SuoraProject.h"
#include "Suora/GameFramework/GameInstance.h"
#include "Suora/Renderer/RenderPipeline.h"
#include "Suora/NodeScript/Scripting/ScriptVM.h"
#include "Suora/Physics/PhysicsEngine.h"
#include "Suora/NodeScript/External/ScriptEngine.h"

namespace Suora
{
	static void LogPreEngineCreateInfo()
	{
		for (int i = 0; i < NativeFunction::s_NativeFunctions.size(); i++)
		{
			NativeFunction* func = NativeFunction::s_NativeFunctions[i];
			SUORA_LOG(LogCategory::Core, LogLevel::Info, "Registered NativeFunction: {0} {1}", func->m_ReturnType, func->m_Label);
		}
	}

	Ref<Engine> Engine::Create()
	{
		LogPreEngineCreateInfo();
		Ref<Engine> engine = Ref<Engine>(new Engine());

		engine->m_RootPath = std::filesystem::current_path(); 
		SUORA_LOG(LogCategory::Core, LogLevel::Info, "Seeking Engine/Project RootPath from: {0}", (engine->m_RootPath /= "Content").string());
		while (!std::filesystem::is_directory(engine->m_RootPath.append("Content")))
		{
			engine->m_RootPath = engine->m_RootPath.parent_path().parent_path();
		}
		engine->m_RootPath = engine->m_RootPath.parent_path();
		SUORA_LOG(LogCategory::Core, LogLevel::Info, "Found Engine/Project RootPath in: {0}", engine->m_RootPath.string());

		AssetManager::Initialize(Path(engine->m_RootPath) / "Content");

		// Dark magic.....
		Class::None.GetClassDefaultObject(true);
		ClassReflector::Create(Object::StaticClass(), [](ClassReflector& desc)
		{
			desc.SetClassName("Object");
			desc.SetClassSize(sizeof(Object)); 
		});

		engine->m_PreviousTime = std::chrono::steady_clock::now();

		engine->m_PhysicsEngine = Physics::PhysicsEngine::Create();
		engine->m_PhysicsEngine->Initialize();

		Array<Class> engineSubsystems = Class::GetSubclassesOf(EngineSubSystem::StaticClass());
		for (const Class& It : engineSubsystems)
		{
			Object* alloc = New(It);
			if (alloc)
			{
				Ref<EngineSubSystem> system = Ref<EngineSubSystem>(alloc->As<EngineSubSystem>());
				engine->m_Subsystems.Add(system);
			}
		}
		for (int32_t i = engine->m_Subsystems.Last(); i >= 0; i--)
		{
			if (!engine->m_Subsystems[i]->Initialize())
			{
				engine->m_Subsystems.RemoveAt(i);
			}
		}

		return engine;
	}

	Engine::~Engine()
	{
		m_GameInstance = nullptr;
	}

	GameInstance* Engine::GetGameInstance() const
	{
		return m_GameInstance.get();
	}

	void Engine::CreateGameInstance()
	{
		m_GameInstance = CreateRef<GameInstance>();
		m_GameInstance->m_Engine = this;
		m_GameInstance->Initialize();
	}
	void Engine::DisposeGameInstance()
	{
		m_GameInstance = nullptr;
	}

	RenderPipeline* Engine::GetRenderPipeline()
	{
		if (!m_RenderPipeline)
		{
			m_RenderPipeline = Ref<RenderPipeline>(New<RenderPipeline>());
			m_RenderPipeline->Initialize();
		}
		return m_RenderPipeline.get();
	}

	Physics::PhysicsEngine* Engine::GetPhysicsEngine() const
	{
		return m_PhysicsEngine.get();
	}

	Array<Ref<EngineSubSystem>> Engine::GetEngineSubsystems() const
	{
		return m_Subsystems;
	}

	String Engine::GetRootPath() const
	{
		return m_RootPath.string();
	}

	float Engine::GetDeltaTime() const
	{
		return m_DeltaTime;
	}

	void Engine::Tick()
	{
		// Updating
		auto currentTime = std::chrono::steady_clock::now();
		auto elapsed = currentTime.time_since_epoch() - m_PreviousTime.time_since_epoch();

		m_DeltaTime = elapsed.count() / 1000000000.0f;

		// Cap the Framerate in Application

		m_PreviousTime = currentTime;
		Update(m_DeltaTime);

	}
	void Engine::Update(float deltaTime)
	{
		m_FramesThisSecond++;
		m_FrameDeltaTimeAccumulator += deltaTime;
		if (m_FrameDeltaTimeAccumulator >= 1.0f)
		{
			m_FPS = m_FramesThisSecond;
			m_FramesThisSecond = 0;
			m_FrameDeltaTimeAccumulator = 0.0f;
		}

		NativeInput::Tick(deltaTime);

		for (const auto It : m_Subsystems)
		{
			It->Tick(deltaTime);
		}
		BlueprintScriptEngine::CleanUp();

		AssetManager::Update(deltaTime);

		if (m_GameInstance)
		{
			m_GameInstance->Update(deltaTime);
		}
	}
	void Engine::Exit()
	{
		Application::Get().Close();
	}
	Engine* Engine::Get()
	{
		return Application::Get().m_Engine.get();
	}
}