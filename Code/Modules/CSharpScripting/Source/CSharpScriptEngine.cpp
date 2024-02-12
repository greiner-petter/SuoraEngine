#include "CSharpScriptEngine.h"
#include "CSharpProjectGenerator.h"
#include "CSharpManagedObject.h"
#include "Suora/Platform/Platform.h"
#include "Suora/Assets/AssetManager.h"
#include "Suora/Core/NativeInput.h"
#include "Suora/Core/Application.h"
#include "Suora/NodeScript/NodeScriptObject.h"

#include "HostInstance.hpp"
#include "Attribute.hpp"
#include "TypeCache.hpp"
#include "GC.hpp"

#include "Suora/GameFramework/Node.h"

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

    // Internal Calls
    void DebugLogInfo(Coral::String InString)  { CSHARP_INFO((String)InString);  }
    void DebugLogWarn(Coral::String InString)  { CSHARP_WARN((String)InString);  }
    void DebugLogError(Coral::String InString) { CSHARP_ERROR((String)InString); }

    static Array<String> s_CSharpClasses;
    struct ClassWrapper
    {
        Class cls = Class::None;
    };
    static Map<String, ClassWrapper> s_CSharpParentClasses;
    static Map<String, Coral::Type*> s_CSharpManagedTypes;
    static Map<String, String> s_CSharpAssemblyQualifiedNames;

    bool CSharpScriptEngine::Initialize()
    {
        s_ScriptEngines.Add(this);

        s_CSharpLog = Log::CustomCategory("C#");
        CSHARP_INFO("Initializing C# ScriptEngine");

        s_SDK_Found = IsDotNetSDKPresent();
        if (!s_SDK_Found)
        {
            CSHARP_ERROR(".NET SDK 8 not found on System. C# Script Engine cannot be used!");
            return false;
        }

        // Compile Coral.Managed.dll
        CSHARP_INFO("Compiling Coral.Managed.dll");
        CompileCSProj(AssetManager::GetEngineAssetPath() + "/../Code/Modules/CSharpScripting/ThirdParty/Coral/Coral.Managed/Coral.Managed.csproj");

        // Copy over runtimeconfig.json
        Platform::CopyDirectory(AssetManager::GetEngineAssetPath() + "/../Code/Modules/CSharpScripting/ThirdParty/Coral/Coral.Managed/Coral.Managed.runtimeconfig.json",
                                AssetManager::GetEngineAssetPath() + "/../Build/CSharp/Release");

        // Setup .NET Host
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

        // Done.
        CSHARP_INFO("Initialized C# ScriptEngine");

        return true;
    }
    void CSharpScriptEngine::Shutdown()
    {
        CSHARP_INFO("Shutdown C# ScriptEngine"); 

        s_ScriptEngines.Remove(this);

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

    String CSharpScriptEngine::GetScriptClassDomain() const
    {
        return "CSharp";
    }

    Array<Class> CSharpScriptEngine::GetAllScriptClasses()
    {
        Array<Class> out;

        for (String It : s_CSharpClasses)
        {
            out.Add(Class("CSharp$" + It));
        }

        return out;
    }

    Class CSharpScriptEngine::GetScriptParentClass(String scriptClass)
    {
        if (s_CSharpParentClasses.ContainsKey(scriptClass))
        {
            return s_CSharpParentClasses.At(scriptClass).cls;
        }

        return Class::None;
    }

    Object* CSharpScriptEngine::CreateScriptClassInstance(const String& scriptClass, bool isRootNode)
    {
        if (!s_CSharpClasses.Contains(scriptClass))
        {
            return nullptr;
        }

        Class nativeParentClass = s_CSharpParentClasses.At(scriptClass).cls;
        while (nativeParentClass.IsScriptClass())
        {
            nativeParentClass = nativeParentClass.GetParentClass();
        }

        Object* obj = New(nativeParentClass);
        SuoraVerify(obj);

        // If needed apply NodeGraph here !
        obj->Implement<INodeScriptObject>();
        INodeScriptObject* interface_ = obj->GetInterface<INodeScriptObject>();
        interface_->m_Class = Class("CSharp$" + scriptClass);

        obj->Implement<ICSharpManagedObject>();
        CreateManagedObject(obj, scriptClass);

        return obj;
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

        // Reset
        s_CSharpClasses.Clear();
        s_CSharpAssemblyQualifiedNames.Clear();
        s_CSharpParentClasses.Clear();
        s_CSharpManagedTypes.Clear();

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

    static Coral::Type SuoraObjectType;
    static Coral::Type SuoraClassType;
    static Coral::Type NativeSuoraClassType;
    void CSharpScriptEngine::ProcessReloadedSuoraAssembly(Coral::ManagedAssembly& assembly)
    {
        SuoraObjectType      = assembly.GetType("Suora.SuoraObject");
        // Get a reference to the SuoraClass Attribute type
        SuoraClassType       = assembly.GetType("Suora.SuoraClass");
        NativeSuoraClassType = assembly.GetType("Suora.NativeSuoraClass");

        assembly.AddInternalCall("Suora.Debug", "LogInfo",  reinterpret_cast<void*>(&DebugLogInfo));
        assembly.AddInternalCall("Suora.Debug", "LogWarn",  reinterpret_cast<void*>(&DebugLogWarn));
        assembly.AddInternalCall("Suora.Debug", "LogError", reinterpret_cast<void*>(&DebugLogError));
        assembly.UploadInternalCalls();
    }

    static NativeClassID GetCoralTypeNativeClassID(Coral::Type type)
    {
        auto attribs = type.GetAttributes();
        for (auto& attribute : attribs)
        {
            if (attribute.GetType() == NativeSuoraClassType)
            {
                return attribute.GetFieldValue<int64_t>("NativeID");
            }
        }

        return 0;
    }

    void CSharpScriptEngine::ProcessReloadedAssembly(Coral::ManagedAssembly& assembly)
    {
        auto allTypes = assembly.GetTypes();

        for (auto type : allTypes)
        {
            auto attribs = type->GetAttributes();
            for (auto& attribute : attribs)
            {
                if (attribute.GetType() == SuoraClassType)
                {
                    CSHARP_WARN("SuoraClass: '{0}' with Parent '{1}'", (String)(type->GetFullName()), (String)(type->GetBaseType().GetFullName()));
                    s_CSharpClasses.Add((String)(type->GetFullName()));
                    s_CSharpAssemblyQualifiedNames[(String)(type->GetFullName())] = (String)(type->GetAssemblyQualifiedName());

                    Coral::Type parentClass = type->GetBaseType();
                    NativeClassID nativeClassID = GetCoralTypeNativeClassID(parentClass);
                    s_CSharpParentClasses[(String)(type->GetFullName())] = ClassWrapper(nativeClassID != 0 ? Class(nativeClassID) : Class("CSharp$" + (String)(parentClass.GetFullName())));

                    s_CSharpManagedTypes[(String)(type->GetFullName())] = type;
                }
            }
        }
    }

    bool CSharpScriptEngine::IsEditor() const
    {
        return Application::Get().IsEditor();
    }

    void CSharpScriptEngine::CreateManagedObject(Object* obj, const String& managedType)
    {
        SuoraVerify(obj);

        String typeStr = (managedType != "") ? managedType : "Suora." + obj->GetNativeClass().GetClassName();

        auto str = Coral::String::New(s_CSharpAssemblyQualifiedNames[typeStr]);
        SuoraObjectType.InvokeStaticMethod("CreateSuoraObject", str, (void*)obj);
        Coral::String::Free(str);
    }

    void CSharpScriptEngine::DestroyManagedObject(Object* obj)
    {
        SuoraVerify(obj);

        SuoraObjectType.InvokeStaticMethod("DestroySuoraObject", (void*)obj);
        Coral::GC::Collect();
    }

    CSharpScriptEngine* CSharpScriptEngine::Get()
    {
        return ScriptEngine::GetScriptEngineByDomain("CSharp")->As<CSharpScriptEngine>();
    }

}