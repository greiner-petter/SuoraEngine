#include "CSharpScriptEngine.h"
#include "CSharpProjectGenerator.h"
#include "Suora/Platform/Platform.h"
#include "Suora/Assets/AssetManager.h"
#include "Suora/Core/NativeInput.h"

#include "HostInstance.hpp"
#include "Attribute.hpp"

namespace Suora
{
    static bool s_SDK_Found = false;
    static LogCategory s_CSharpLog = LogCategory::None;
#define CSHARP_TRACE(...) SUORA_LOG(s_CSharpLog, LogLevel::Trace, __VA_ARGS__)
#define CSHARP_INFO(...)  SUORA_LOG(s_CSharpLog, LogLevel::Info,  __VA_ARGS__)
#define CSHARP_WARN(...)  SUORA_LOG(s_CSharpLog, LogLevel::Warn,  __VA_ARGS__)
#define CSHARP_ERROR(...) SUORA_LOG(s_CSharpLog, LogLevel::Error, __VA_ARGS__)

    void ExceptionCallback(std::string_view InMessage)
    {
        CSHARP_ERROR("Unhandled native exception: {0}", InMessage);
    }
    void MessageCallback(std::string_view InMessage, Coral::MessageLevel InLevel)
    {
        switch (InLevel)
        {
        case Coral::MessageLevel::Info:
            CSHARP_INFO(InMessage);
            break;
        case Coral::MessageLevel::Warning:
            CSHARP_WARN(InMessage);
            break;
        case Coral::MessageLevel::Error:
            CSHARP_ERROR(InMessage);
            break;
        default:
            CSHARP_TRACE(InMessage);
            break;
        }

    }

    bool CSharpScriptEngine::Initialize()
    {
        s_CSharpLog = Log::CustomCategory("C#");
        CSHARP_INFO("Initializing C# ScriptEngine");

        s_SDK_Found = IsDotNetSDKPresent();
        if (!s_SDK_Found)
        {
            CSHARP_ERROR(".NET SDK 8 not found on System. C# Script Engine cannot be used!");
            return false;
        }

        CSHARP_INFO("Compiling Coral.Managed.dll");
        CompileCSProj(AssetManager::GetEngineAssetPath() + "/../Code/Modules/CSharpScripting/ThirdParty/Coral/Coral.Managed/Coral.Managed.csproj");
        Platform::CopyDirectory(AssetManager::GetEngineAssetPath() + "/../Code/Modules/CSharpScripting/ThirdParty/Coral/Coral.Managed/Coral.Managed.runtimeconfig.json",
                                AssetManager::GetEngineAssetPath() + "/../Build/CSharp/Release");

        CSHARP_INFO("Setting up .NET Core Host");
        auto coralDir = AssetManager::GetEngineAssetPath() + "/../Build/CSharp/Release";
        Coral::HostSettings settings =
        {
            .CoralDirectory = coralDir,
            .MessageCallback = MessageCallback,
            .ExceptionCallback = ExceptionCallback
        };
        m_HostInstance = CreateRef<Coral::HostInstance>();
        m_HostInstance->Initialize(settings);

        CSHARP_INFO("Initialized C# ScriptEngine");

        ReloadAssemblies();

        return true;
    }
    void CSharpScriptEngine::Shutdown()
    {
        CSHARP_INFO("Shutdown C# ScriptEngine"); 

        if (!s_SDK_Found)
        {
            return;
        }
    }

    void CSharpScriptEngine::Tick(float deltaTime)
    {
        if (!s_SDK_Found)
        {
            return;
        }

        static bool s_WasProjectCSCodeCompiledOnce = false;
        if (!s_WasProjectCSCodeCompiledOnce)
        {
            if (AssetManager::GetProjectAssetPath() != "")
            {
                BuildAndReloadAllCSProjects();
                s_WasProjectCSCodeCompiledOnce = true;
            }
        }

        if (NativeInput::GetKeyDown(Key::F3))
        {
            BuildAndReloadAllCSProjects();
        }
    }

    bool CSharpScriptEngine::IsDotNetSDKPresent()
    {
        return std::filesystem::exists(Coral::HostInstance::GetHostFXRPath());
    }

    void CSharpScriptEngine::BuildAllCSProjects()
    {
        if (!s_SDK_Found)
        {
            return;
        }

        std::filesystem::path path = AssetManager::GetProjectAssetPath() + "/../Code/CSharp";
        if (std::filesystem::exists(path))
        {
            for (auto dir : std::filesystem::directory_iterator(path))
            {
                if (dir.is_directory())
                {
                    CompileCSProj(dir.path() / (dir.path().filename().string() + ".csproj"));
                }
            }
        }

    }

    void CSharpScriptEngine::CompileCSProj(const std::filesystem::path& csproj)
    {
        if (!s_SDK_Found)
        {
            return;
        }
        CSHARP_INFO("Compiling {0}", csproj.string());
        Platform::CommandLine("dotnet build " + csproj.string() + " -c Release");
    }

    void CSharpScriptEngine::BuildAndReloadAllCSProjects()
    {
        if (!s_SDK_Found)
        {
            return;
        }
        auto begin = std::chrono::high_resolution_clock::now();

        // Generate
        CSharpProjectGenerator::GenerateCSProjectFiles();

        // Build
        BuildAllCSProjects();

        // Reload
        ReloadAssemblies();

        auto end = std::chrono::high_resolution_clock::now();
        CSHARP_INFO("It took {0}ms to build and reload all C# Projects.", std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
    }

    void CSharpScriptEngine::ReloadAssemblies()
    {
        if (!s_SDK_Found)
        {
            return;
        }
        CSHARP_INFO("Reloading C# Script Assemblies...");

        if (m_AssemblyLoadContext)
        {
            m_HostInstance->UnloadAssemblyLoadContext(*m_AssemblyLoadContext);
        }

        auto loadContext = m_HostInstance->CreateAssemblyLoadContext("MainAssembly");
        m_AssemblyLoadContext = CreateRef<Coral::AssemblyLoadContext>();
        *m_AssemblyLoadContext = loadContext;

        std::filesystem::path assemblyDir;
        if (IsEditor())
        {
            assemblyDir = AssetManager::GetProjectAssetPath() + "/../Build/CSharp/Release";
        }
        else
        {
            assemblyDir = AssetManager::GetProjectAssetPath() + "/../Binaries/CSharp";
        }

        if (std::filesystem::exists(assemblyDir))
        {
            std::filesystem::path suoraGeneratedDll = assemblyDir / "Suora.Generated.dll";
            if (!std::filesystem::exists(suoraGeneratedDll))
            {
                CSHARP_ERROR("Suora.Generated.dll was not found. Could not reload Assemblies!");
                return;
            }
            auto& generatedAssembly = m_AssemblyLoadContext->LoadAssembly(suoraGeneratedDll.string());
            ProcessReloadedSuoraAssembly(generatedAssembly);

            for (const auto& file : std::filesystem::directory_iterator(assemblyDir))
            {
                if (file.is_directory())
                    continue;

                std::string filename = file.path().filename().string();
                if (filename == "Coral.Managed.dll" || filename == "Suora.Generated.dll")
                    continue;

                if (filename.ends_with(".dll"))
                {
                    auto& assembly = m_AssemblyLoadContext->LoadAssembly(file.path().string());
                    ProcessReloadedAssembly(assembly);
                }
                
            }
        }

        CSHARP_INFO("Reloaded C# Script Assemblies!");
    }

    static Coral::Type SuoraClassType;
    void CSharpScriptEngine::ProcessReloadedSuoraAssembly(const Coral::ManagedAssembly& assembly)
    {
        // Get a reference to the SuoraClass Attribute type
        SuoraClassType = assembly.GetType("Suora.SuoraClass");
    }

    void CSharpScriptEngine::ProcessReloadedAssembly(const Coral::ManagedAssembly& assembly)
    {
        auto allTypes = assembly.GetTypes();

        for (auto type : allTypes)
        {
            auto attribs = type->GetAttributes();
            for (auto& attribute : attribs)
            {
                if (attribute.GetType() == SuoraClassType)
                {
                    CSHARP_WARN("SuoraClass: '{0}' with Parent '{1}'", (std::string)(type->GetFullName()), (std::string)(type->GetBaseType().GetFullName()));
                }
            }
        }
    }

    bool CSharpScriptEngine::IsEditor()
    {
        return true;
    }

}