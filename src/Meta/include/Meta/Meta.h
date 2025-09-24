#pragma once

#include <Meta/MetaExport.h>

#include <any>
#include <vector>
#include <string_view>
#include <assert.h>
#include <functional>
#include <type_traits>
#include <typeindex>

#include <Logger/Logger.h>

// TODO: Document with comments
// TODO: Add accessors for getting class meta by string name
// TODO: Add accessors for getting props by name

#define DECLARE_META_OBJECT(classname) \
	private: \
		friend Meta::Impl::MetaInitializer<classname>; \
		static Meta::Impl::MetaInitializer<classname> s_metaInit; \
		static void initMeta(Meta::Impl::MetaInitializer<classname>& w); \

#define IMPLEMENT_META_OBJECT(classname) \
	Meta::Impl::MetaInitializer<classname> classname::s_metaInit = Meta::Impl::MetaInitializer<classname>(#classname); \
	void classname::initMeta(Meta::Impl::MetaInitializer<classname>& w)

//#define META_PROPERTY_CREATE()

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

		virtual std::any createDefaultAsAny() const = 0;
		virtual std::any getAsAny(void* obj) const = 0;
		virtual void setFromAny(void* obj, const std::any& val) = 0;
		virtual std::type_index getTypeIndex() const = 0;
	};

	template<class C, typename T>
	struct MemberProperty : MemberPropertyBase
	{
		using ClassType = C;
		using MemberType = T;

		MemberProperty(std::string_view name, MemberType ClassType::* ptr)
			: MemberPropertyBase(name)
			, member(ptr)
		{
		}

		MemberType ClassType::*member;

		std::any createDefaultAsAny() const { return std::any(MemberType()); };
		const MemberType& get(const ClassType& obj) const { return obj.*member; };
		void set(ClassType& obj, const MemberType& val) { obj.*member = val; };
		std::type_index getTypeIndex() const { return std::type_index(typeid(MemberType)); };

		virtual std::any getAsAny(void* obj) const
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
		// TODO: Props should not be exposed like this.
		// These pointers can be modified an that is no bueno
		std::vector<MemberPropertyBase*> props;

		ClassMetaBase(std::string_view name)
			: name(name)
			, props{}
		{
		}

		virtual std::any createDefaultAsAny() const = 0;
		virtual std::type_index getTypeIndex() const = 0;
	};

	template<class C>
	struct ClassMeta : ClassMetaBase
	{
		using ClassType = C;

		ClassMeta(std::string_view name)
			: ClassMetaBase(name)
		{
		}

		std::any createDefaultAsAny() const
		{
			return std::any(ClassType());
		}

		std::type_index getTypeIndex() const
		{
			return std::type_index(typeid(ClassType));
		}
	};

	META_EXPORT void initializeMetaInfo();
	META_EXPORT const ClassMetaBase* getClassMeta(const std::type_index& index);

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

			template<auto member>
			void addProperty(std::string_view name)
			{
				using V = std::remove_cvref_t<decltype(std::declval<T>().*member)>;
				Meta::MemberPropertyBase* prop = new MemberProperty<T, V>{ name, member };
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
}
