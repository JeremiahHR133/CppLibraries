#pragma once

#include <Meta/MetaExport.h>

#include <any>
#include <vector>
#include <string_view>
#include <assert.h>
#include <functional>
#include <type_traits>

#include <Logger/Logger.h>

#define DECLARE_META_OBJECT(classname) \
	private: \
		friend Impl::MetaInitializer<classname>; \
		static Impl::MetaInitializer<classname> s_metaInit; \
		static void initMeta(Impl::MetaInitializer<classname>& w); \

#define IMPLEMENT_META_OBJECT(classname) \
	Impl::MetaInitializer<classname> classname::s_metaInit = Impl::MetaInitializer<classname>(#classname); \
	void classname::initMeta(Impl::MetaInitializer<classname>& w)

namespace Meta
{
	struct MemberPropertyBase
	{
		std::string_view name;

		MemberPropertyBase(std::string_view str)
			: name(str)
		{
		}
		virtual ~MemberPropertyBase() = default;

		virtual std::any createDefaultAsAny() = 0;
		virtual std::any getAsAny(void* obj) = 0;
		virtual void setFromAny(void* obj, const std::any& val) = 0;
		virtual std::string_view getTypeName() = 0;
	};

	template<class C, typename T>
	struct MemberProperty : MemberPropertyBase
	{
		using ClassType = C;
		using MemberType = T;

		MemberProperty(std::string_view name, MemberType ClassType::* ptr, std::string_view tName)
			: MemberPropertyBase(name)
			, member(ptr)
			, typeName(tName)
		{
		}

		MemberType ClassType::*member;
		std::string_view typeName;

		std::any createDefaultAsAny() { return std::any(MemberType()); };
		const MemberType& get(const ClassType& obj) { return obj.*member; };
		void set(ClassType& obj, const MemberType& val) { obj.*member = val; };
		std::string_view getTypeName() { return typeName; };

		virtual std::any getAsAny(void* obj)
		{
			return std::any(get(*(static_cast<ClassType*>(obj))));
		}

		virtual void setFromAny(void* obj, const std::any& val)
		{
			set(*(static_cast<ClassType*>(obj)), std::any_cast<MemberType>(val));
		}
	};

	struct ClassMetaBase
	{
		std::string_view name;
		std::vector<MemberPropertyBase*> props;

		ClassMetaBase(std::string_view name, const std::vector<MemberPropertyBase*> props)
			: name(name)
			, props(props)
		{
		}

		virtual std::any createDefaultAsAny() = 0;
	};

	template<class C>
	struct ClassMeta : ClassMetaBase
	{
		using ClassType = C;

		ClassMeta(std::string_view name, const std::vector<MemberPropertyBase*> props)
			: ClassMetaBase(name, props)
		{
		}

		ClassMeta(std::string_view name)
			: ClassMetaBase(name, {})
		{
		}

		std::any createDefaultAsAny()
		{
			return std::any(ClassType());
		}
	};

	META_EXPORT void initializeMetaInfo();

	namespace Impl
	{
		META_EXPORT void addClass(ClassMetaBase* c);
		META_EXPORT void addDelayInitialize(std::function<void()> call);

		template<typename T>
		class MetaInitializer
		{
			static_assert(std::is_default_constructible_v<T>, "Class must be default constructible to use meta!");
		public:
			MetaInitializer(std::string_view name)
				: m_classPtr(nullptr)
			{
				addDelayInitialize([name, this]()
					{
						m_classPtr = new ClassMeta<T>(name);
						addClass(m_classPtr);
						T::initMeta(*this);
					}
				);
			}
			~MetaInitializer() = default;

			void addProperty(MemberPropertyBase* prop)
			{
				if (!prop)
				{
					assert(false && "Prop was nullptr");
					Log::Error().log("Failed to add nullptr prop!");
				}

				auto foundProp = std::find_if(m_classPtr->props.begin(), m_classPtr->props.end(), [prop](const Meta::MemberPropertyBase* comp) {return comp->name == prop->name; });
				if (foundProp == m_classPtr->props.end())
				{
					Log::Debug().log("Registered new property: \"{}\" to class: \"{}\"", prop->name, m_classPtr->name);
					m_classPtr->props.push_back(prop);
				}
				else
				{
					assert(false && "Property already added for class");
					Log::Error().log("Failed to add property! Property already exists: \"{}\"", prop->name);
				}
			}

		private:
			ClassMetaBase* m_classPtr;
		};
	}

	META_EXPORT void runTests();
}
