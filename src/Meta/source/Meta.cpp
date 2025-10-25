#include <Meta/Meta.h>

#include <mutex>

namespace
{
	class MetaInfoRepo
	{
	public:
		MetaInfoRepo() = default;
		~MetaInfoRepo() = default;

		void addClass(const Meta::ClassMetaBase* newClass);
		const std::vector<const Meta::ClassMetaBase*>& getAllClasses()
		{
			return m_allClasses;
		}

	private:
		std::vector<const Meta::ClassMetaBase*> m_allClasses;
	};

	void MetaInfoRepo::addClass(const Meta::ClassMetaBase* newClass)
	{
		if (!newClass)
		{
			Log::Error().log("Rejected new class which was nullptr!");
			assert(false && "New class was nullptr");
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
			Log::Error().log("Class already registered! Name: {}", newClass->getName());
			assert(false && "Duplicate class registered!");
		}
	}

	MetaInfoRepo& getGlobalMeta()
	{
		static MetaInfoRepo s_metaInfo;
		return s_metaInfo;
	}

	std::vector<std::function<void()>> g_addClassCallbacks;
	std::mutex g_addClassVecMutex;

	std::vector<std::function<void()>> g_parentInitCallbacks;
	std::mutex g_parentInitVecMutex;

	std::vector<std::function<void()>> g_metaInitCallbacks;
	std::mutex g_metaInitVecMutex;
}

namespace Meta
{
	bool MetaObject::isOrIsDerivedFrom(std::type_index idx) const
	{
		const ClassMetaBase* ptr = Meta::getClassMeta(getTypeName());
		while (ptr)
		{
			if (ptr->getTypeIndex() == idx)
				return true;

			ptr = ptr->getParent();
		}

		return false;
	}

	void initializeMetaInfo()
	{
		// Register all classes
		{
			std::lock_guard<std::mutex> lock(g_addClassVecMutex);
			for (auto& func : g_addClassCallbacks)
			{
				func();
			}
		}

		// Initialize all meta info
		{
			std::lock_guard<std::mutex> lock(g_metaInitVecMutex);
			for (auto& func : g_metaInitCallbacks)
			{
				func();
			}
		}

		// Update meta info based on parent meta
		{
			std::lock_guard<std::mutex> lock(g_parentInitVecMutex);
			for (auto& func : g_parentInitCallbacks)
			{
				func();
			}
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
		for (const auto* p : meta.getMemberProps())
			if (p->getName() == name)
				return p;

		return nullptr;
	}

	const MemberPropertyBase* ClassMetaBase::getMemberProp(const std::string& name) const
	{
		for (const auto* p : getMemberProps())
			if (p->getName() == name)
				return p;

		return nullptr;
	}


	const MemberNonConstFunctionPropBase* ClassMetaBase::getNonConstFunc(const std::string& name) const
	{
		for (const auto* f : getNonConstFuncs())
			if (f->getName() == name)
				return f;

		return nullptr;
	}

	const MemberConstFunctionPropBase* ClassMetaBase::getConstFunc(const std::string& name) const
	{
		for (const auto* f : getConstFuncs())
			if (f->getName() == name)
				return f;

		return nullptr;
	}

	namespace Impl
	{
		void addClass(const ClassMetaBase* c)
		{
			getGlobalMeta().addClass(c);
		}

		void addDelayClass(std::function<void()> call)
		{
			std::lock_guard<std::mutex> lock(g_addClassVecMutex);
			g_addClassCallbacks.push_back(call);
		}

		void addDelayParentInitialize(std::function<void()> call)
		{
			std::lock_guard<std::mutex> lock(g_parentInitVecMutex);
			g_parentInitCallbacks.push_back(call);
		}

		void addDelayMetaInitialize(std::function<void()> call)
		{
			std::lock_guard<std::mutex> lock(g_metaInitVecMutex);
			g_metaInitCallbacks.push_back(call);
		}
	}
}