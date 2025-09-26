#include <Meta/Meta.h>

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

	const ClassMetaBase* getClassMeta(const std::type_index& index)
	{
		for (auto* c : getGlobalMeta().getAllClasses())
			if (c->getTypeIndex() == index)
				return c;

		return nullptr;
	}

	const ClassMetaBase* getClassMeta(const std::string& name)
	{
		for (auto* c : getGlobalMeta().getAllClasses())
			if (c->name == name)
				return c;

		return nullptr;
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
}