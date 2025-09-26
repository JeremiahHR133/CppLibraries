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

#define DECLARE_META_OBJECT(classname) \
	private: \
		friend Meta::Impl::MetaInitializer<classname>; \
		static Meta::Impl::MetaInitializer<classname> s_metaInit; \
		static void initMeta(Meta::Impl::MetaInitializer<classname>& w); \
	public: \
		std::type_index getTypeIndex() const override;

#define IMPLEMENT_META_OBJECT(classname) \
	static_assert(std::is_base_of_v<Meta::MetaObject, classname>, "Meta objects must subclass the Meta::MetaObject class!"); \
	static_assert(std::is_default_constructible_v<classname>, "Class must be default constructible to use meta!"); \
	Meta::Impl::MetaInitializer<classname> classname::s_metaInit = Meta::Impl::MetaInitializer<classname>(#classname); \
	std::type_index classname::getTypeIndex() const { return std::type_index(typeid(classname)); } \
	void classname::initMeta(Meta::Impl::MetaInitializer<classname>& w)

//#define META_PROPERTY_CREATE()

namespace Meta
{
	class MetaObject
	{
	public:
		MetaObject() = default;
		virtual ~MetaObject() = default;

		virtual std::type_index getTypeIndex() const = 0;
	};

	class MemberPropertyBase
	{
	public:
		MemberPropertyBase(const std::string& str)
			: name(str)
		{
		}
		virtual ~MemberPropertyBase() = default;

		virtual std::any createDefaultAsAny() const = 0;
		virtual std::any getAsAny(const MetaObject& obj) const = 0;
		virtual void setFromAny(MetaObject& obj, const std::any& val) const = 0;
		virtual std::type_index getTypeIndex() const = 0;

		const std::string& getName() const { return name; }

	private:
		std::string name;
	};

	namespace Impl
	{
		template<typename T>
		class MetaInitializer;
	}

	template<class C, typename T>
	class MemberProperty : public MemberPropertyBase
	{

		using ClassType = C;
		using MemberType = T;

		friend Impl::MetaInitializer<C>;

		// Creation is limited to the initializer class
		MemberProperty(const std::string& name, MemberType ClassType::* ptr)
			: MemberPropertyBase(name)
			, member(ptr)
		{
		}
		MemberType ClassType::*member;

	public:

		std::any createDefaultAsAny() const { return std::any(MemberType()); };
		const MemberType& get(const ClassType& obj) const { return obj.*member; };
		void set(ClassType& obj, const MemberType& val) const { obj.*member = val; };
		std::type_index getTypeIndex() const { return std::type_index(typeid(MemberType)); };

		virtual std::any getAsAny(const MetaObject& obj) const
		{
			if (obj.getTypeIndex() == std::type_index(typeid(ClassType)))
			{
				return std::any(get(static_cast<const ClassType&>(obj)));
			}
			else
			{
				assert(false && "Property does not belong to given object!");
				Log::Error().log("Failed to getAsAny for prop! Property does not belong to given object!");
			}
			return std::any();
		}

		virtual void setFromAny(MetaObject& obj, const std::any& val) const
		{
			if (obj.getTypeIndex() == std::type_index(typeid(ClassType)))
			{
				try
				{
					set(static_cast<ClassType&>(obj), std::any_cast<MemberType>(val));
				}
				catch (const std::bad_any_cast& e)
				{
					assert(false && "Attempted to set property with 'any' of wrong type!");
					Log::Error().log("Failed to set property! Given 'any' is the wrong type!");
				}
			}
			else
			{
				assert(false && "Property does not belong to given object!");
				Log::Error().log("Failed to setFromAny for prop! Property does not belong to given object!");
			}
		}
	};

	class META_EXPORT ClassMetaBase
	{
	public:
		ClassMetaBase(const std::string& name)
			: name(name)
			, props{}
		{
		}
		virtual ~ClassMetaBase() = default;

		virtual std::any createDefaultAsAny() const = 0;
		virtual std::type_index getTypeIndex() const = 0;

		const std::string& getName() const { return name; }
		const std::vector<const MemberPropertyBase*> getProps() const { return props; }
		const MemberPropertyBase* getProp(const std::string& name) const;

	private:
		std::string name;
		std::vector<const MemberPropertyBase*> props;

		template<typename T> friend class Impl::MetaInitializer;
	};

	template<class C>
	class ClassMeta : public ClassMetaBase
	{
	public:
		using ClassType = C;

		ClassMeta(const std::string& name)
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
	META_EXPORT const ClassMetaBase* getClassMeta(const std::string& name);

	template<typename T>
	const ClassMetaBase* getClassMeta()
	{
		return getClassMeta(std::type_index(typeid(T)));
	}

	namespace Impl
	{
		META_EXPORT void addClass(ClassMetaBase* c);
		META_EXPORT void addDelayInitialize(std::function<void()> call);

		template<typename T>
		class MetaInitializer
		{
		public:
			MetaInitializer(const std::string& name)
				: m_classPtr(nullptr)
			{
				addDelayInitialize([name, this]()
					{
						m_classPtr = new ClassMeta<T>(name);
						assert(m_classPtr);
						if (m_classPtr)
						{
							addClass(m_classPtr);
							T::initMeta(*this);
						}
						else
							Log::Critical().log("Unable to allocate new ClassMeta!");
					}
				);
			}
			~MetaInitializer() = default;

			template<auto member>
			void addProperty(const std::string& name)
			{
				// TODO: This doesn't allow members to be: const, volatile, or references
				// Determine if that is a valid restriction...
				using V = std::remove_cvref_t<decltype(std::declval<T>().*member)>;
				Meta::MemberPropertyBase* prop = new MemberProperty<T, V>{ name, member };
				auto foundProp = std::find_if(m_classPtr->props.begin(), m_classPtr->props.end(), [prop](const Meta::MemberPropertyBase* comp) {return comp->getName() == prop->getName(); });
				if (foundProp == m_classPtr->props.end())
				{
					Log::Debug().log("Registered new property: \"{}\" to class: \"{}\"", prop->getName(), m_classPtr->getName());
					m_classPtr->props.push_back(prop);
				}
				else
				{
					assert(false && "Property already added for class");
					Log::Error().log("Failed to add property! Property already exists: \"{}\"", prop->getName());
				}
			}

		private:
			ClassMetaBase* m_classPtr;
		};
	}
}
