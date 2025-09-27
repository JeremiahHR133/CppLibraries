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
			Log::Error().log("Rejected new class which was nullptr!");
			return;
		}

		auto foundClass = std::find_if(m_allClasses.begin(), m_allClasses.end(), [newClass](const Meta::ClassMetaBase* comp) {return comp->getName() == newClass->getName(); });
		if (foundClass == m_allClasses.end())
		{
			Log::Debug().log("Registered new class: {}", newClass->getName());
			m_allClasses.push_back(newClass);
		}
		else
		{
			assert(false && "Duplicate class registered!");
			Log::Error().log("Class already registered! Name: {}", newClass->getName());
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
			if (c->getName() == name)
				return c;

		return nullptr;
	}

	const MemberPropertyBase* getPropMeta(const ClassMetaBase& meta, const std::string& name)
	{
		for (const auto* p : meta.getProps())
			if (p->getName() == name)
				return p;

		return nullptr;
	}

	const MemberPropertyBase* ClassMetaBase::getProp(const std::string& name) const
	{
		for (const auto* p : getProps())
			if (p->getName() == name)
				return p;

		return nullptr;
	}

	const MemberFunctionPropBase* ClassMetaBase::getFunc(const std::string& name) const
	{
		for (const auto* f : getFuncs())
			if (f->getName() == name)
				return f;

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