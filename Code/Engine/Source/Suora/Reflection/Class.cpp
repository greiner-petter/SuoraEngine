#include "Precompiled.h"
#include "Class.h"
#include "ClassReflector.h"
#include "Suora/Assets/AssetManager.h"
#include "Suora/Assets/Blueprint.h"
#include "Suora/Assets/ScriptClass.h"
#include "Suora/Common/Array.h"
#include "Suora/Common/Random.h"

namespace Suora
{
	const Class Class::None = (NativeClassID) 0;

	Class Class::GetParentClass() const
	{
		switch (GetClassType())
		{
		case ClassType::Native:
			return ClassReflector::GetByClass(*this).m_NativeParentClass;
		case ClassType::BlueprintClass:
			return m_BlueprintClass->GetNodeParentClass();
		case ClassType::ScriptClass:
			return m_ScriptClass->GetScriptParentClass();
		case ClassType::None:
		default:
			SuoraError("Class::GetParentClass() implementation missing!");
			return Class::None;
		}
		return Class::None;
	}

	ClassType Class::GetClassType() const
	{
		if (IsNative() && m_NativeClassID != 0) return ClassType::Native;
		if (IsScriptClass()) return ClassType::ScriptClass;
		if (IsBlueprintClass()) return ClassType::BlueprintClass;

		return ClassType::None;
	}

	void Class::GenerateNativeClassReflector(const Class& cls)
	{
		ClassReflector::GetByClass(cls);
	}

	Array<Class> Class::GetInheritanceTree()
	{
		Array<Class> out;
		Class level = *this;

		while (level != Class::None)
		{
			out.Insert(0, level);
			level = level.GetParentClass();
		}

		return out;
	}

	Array<Class> Class::GetAllClasses()
	{
		Array<Class> classes = s_NativeClasses;
		classes.Add(Object::StaticClass());

		const Array<Blueprint*> blueprints = AssetManager::GetAssets<Blueprint>();
		for (Blueprint* bp : blueprints)
		{
			classes.Add(bp);
		}

		const Array<ScriptClass*> sclasses = AssetManager::GetAssets<ScriptClass>();
		for (ScriptClass* sclass : sclasses)
		{
			classes.Add(sclass);
		}

		return classes;
	}

	Array<Class> Class::GetSubclassesOf(const Class& base)
	{
		Array<Class> classes = GetAllClasses();
		Array<Class> subClasses;

		int i = 0;
		for (Class& cls : classes)
		{
			if (cls != Class::None && cls.Inherits(base))
			{
				subClasses.Add(cls);
			}
		}

		return subClasses;
	}

	std::string Class::GetNativeClassName() const
	{
		return ClassReflector::GetClassName(*this);
	}

	std::string Class::GetClassName() const
	{
		switch (GetClassType())
		{
		case ClassType::Native:
			return GetNativeClassName();
		case ClassType::BlueprintClass:
			return m_BlueprintClass->GetAssetName();
		case ClassType::ScriptClass:
		case ClassType::None:
			return "None";
		default:
			SuoraError("Class::GetParentClass() implementation missing!");
			return "";
		}
		return "";
	}

	const ClassReflector& Class::GetClassReflector() const
	{
		return ClassReflector::GetByClass(*this);
	}

	std::string Class::ToString() const
	{
		std::string str = IsNative() ? "Native$" : "Node$";

		if (IsNative())
		{
			str.append(std::to_string(GetNativeClassID()));
		}
		else
		{
			str.append(GetBlueprintClass()->m_UUID.GetString());
		}

		return str;
	}

	Class Class::FromString(const std::string& str)
	{
		if (str.find("Native$") != std::string::npos)
		{
			const std::string substr = str.substr(7, str.size() - 7);
			NativeClassID id = std::stoll(substr);
			return id;
		}
		if (str.find("Node$") != std::string::npos)
		{
			std::string uid = str.substr(5, str.size() - 5);
			return AssetManager::GetAsset<Blueprint>(uid);
		}

		return Class::None;
	}

	Object* Class::GetClassDefaultObject(bool clear) const
	{
		static std::unordered_map<Class, Ref<Object>, std::hash<Suora::Class>> s_ClassDefaultObjects;
		if (clear) s_ClassDefaultObjects.clear();

		if (*this == Class::None) return nullptr;

		if (s_ClassDefaultObjects.find(*this) == s_ClassDefaultObjects.end())
		{
			Ref<Object> obj = Ref<Object>(New(*this));

			s_ClassDefaultObjects[*this] = obj;

			return s_ClassDefaultObjects[*this].get();
		}
		else
		{
			return s_ClassDefaultObjects[*this].get();
		}
	}
}