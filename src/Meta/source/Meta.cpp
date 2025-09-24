#include <Meta/Meta.h>

#include <Converter/Converter.h>

#include <mutex>

namespace
{
	class MetaInfoRepo
	{
	public:
		MetaInfoRepo() = default;
		~MetaInfoRepo() = default;

		void addClass(Meta::ClassMetaBase* newClass);
		const std::vector<const Meta::ClassMetaBase*>& getAllClasses()
		{
			return m_allClasses;
		}

	private:
		std::vector<const Meta::ClassMetaBase*> m_allClasses;
	};

	void MetaInfoRepo::addClass(Meta::ClassMetaBase* newClass)
	{
		if (!newClass)
		{
			assert(false && "New class was nullptr");
			Log::Error().log("Rejected adding new class which was nullptr!");
			return;
		}

		auto foundClass = std::find_if(m_allClasses.begin(), m_allClasses.end(), [newClass](const Meta::ClassMetaBase* comp) {return comp->name == newClass->name; });
		if (foundClass == m_allClasses.end())
		{
			Log::Debug().log("Registered new class: {}", newClass->name);
			m_allClasses.push_back(newClass);
		}
		else
		{
			assert(false && "Duplicate class registered!");
			Log::Error().log("Class already registered! Name: {}", newClass->name);
		}
	}

	MetaInfoRepo& getGlobalMeta()
	{
		static MetaInfoRepo s_metaInfo;
		return s_metaInfo;
	}

	std::vector<std::function<void()>> g_globalMetaCallback;
	std::mutex g_metaVecMutex;
}

namespace Meta
{
	void initializeMetaInfo()
	{
		std::lock_guard<std::mutex> lock(g_metaVecMutex);
		for (auto& func : g_globalMetaCallback)
		{
			func();
		}
	}

	namespace Impl
	{
		void addClass(ClassMetaBase* c)
		{
			getGlobalMeta().addClass(c);
		}

		void addDelayInitialize(std::function<void()> call)
		{
			std::lock_guard<std::mutex> lock(g_metaVecMutex);
			g_globalMetaCallback.push_back(call);
		}
	}
	class ExampleStruct
	{
		DECLARE_META_OBJECT(ExampleStruct)
	public:
		ExampleStruct()
			: one(0)
			, two(false)
		{
		}
		ExampleStruct(int i, bool b)
			: one(i)
			, two(b)
		{
		}
	private:
		int one;
		bool two;
	};

	IMPLEMENT_META_OBJECT(ExampleStruct)
	{
		w.addProperty(new MemberProperty<ExampleStruct, int>{ "one", &ExampleStruct::one, "int" });
		w.addProperty(new MemberProperty<ExampleStruct, bool>{ "two", &ExampleStruct::two, "bool" });
	}

	void runTests()
	{
		ExampleStruct obj{1, false};

		auto* objMeta = getGlobalMeta().getAllClasses()[0];
		for (const auto& prop : objMeta->props)
		{
			Log::Info().log("Property: Name = {}, Value = {}", prop->name, Converter::getStringFromAny(prop->getTypeName(), prop->getAsAny(&obj)));
		}
	}
}