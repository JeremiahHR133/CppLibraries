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
	namespace Impl
	{
		template<typename T, typename C>
		T member_value_deducer(T C::*);
		template<auto MV>
		using member_value_type_t = decltype(member_value_deducer(MV));

		template<typename T, typename C, typename... Args>
		T member_function_return_deducer(T (C::*)(Args...));
		template<typename T, typename C, typename... Args>
		T member_function_return_deducer(T (C::*)(Args...) const);
		template<auto MV>
		using member_function_return_type_t = decltype(member_function_return_deducer(MV));

		template <typename T, typename C, typename func>
		struct is_member_setter_function_pointer : std::false_type {};
		template <typename T, typename C>
		struct is_member_setter_function_pointer<T, C, void (C::*)(T)> : std::true_type {};

		template <typename T, typename C, typename func>
		struct is_member_getter_function_pointer : std::false_type {};
		template <typename T, typename C>
		struct is_member_getter_function_pointer<T, C, T (C::*)(void) const> : std::true_type {};

	}

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

	template<class ClassType, typename MemberType>
	class TemplateMemberPropertyBase : public MemberPropertyBase
	{
	public:
		TemplateMemberPropertyBase(const std::string& name)
			: MemberPropertyBase(name)
		{
		}

		virtual MemberType get(const ClassType& obj) const = 0;
		virtual void set(ClassType& obj, MemberType val) const = 0;

		std::any createDefaultAsAny() const override { return std::any(MemberType()); };
		std::type_index getTypeIndex() const override { return std::type_index(typeid(MemberType)); };

		std::any getAsAny(const MetaObject& obj) const override
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

		void setFromAny(MetaObject& obj, const std::any& val) const override
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

	template<class ClassType, typename MemberType>
	class MemberProperty : public TemplateMemberPropertyBase<ClassType, MemberType>
	{
		friend Impl::MetaInitializer<ClassType>;

		// Creation is limited to the initializer class
		MemberProperty(const std::string& name, MemberType ClassType::* ptr)
			: TemplateMemberPropertyBase<ClassType, MemberType>(name)
			, member(ptr)
		{
		}

		MemberType ClassType::*member;

	public:

		MemberType get(const ClassType& obj) const override { return obj.*member; };
		void set(ClassType& obj, MemberType val) const override { obj.*member = val; };
	};

	template<class ClassType, typename MemberType>
	class MemberPropertyFunctional : public TemplateMemberPropertyBase<ClassType, MemberType>
	{
		friend Impl::MetaInitializer<ClassType>;

		// Creation is limited to the initializer class
		MemberPropertyFunctional(const std::string& name, void (ClassType::*setter)(MemberType), MemberType (ClassType::*getter)(void) const)
			: TemplateMemberPropertyBase<ClassType, MemberType>(name)
			, setter(setter)
			, getter(getter)
		{
		}

		void (ClassType::* setter)(MemberType);
		MemberType(ClassType::* getter)(void) const;

	public:

		MemberType get(const ClassType& obj) const override { return (obj.*getter)(); };
		void set(ClassType& obj, MemberType val) const override { (obj.*setter)(val); };
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

		template<typename ClassType>
		class MetaInitializer
		{
		public:
			MetaInitializer(const std::string& name)
				: m_classPtr(nullptr)
			{
				addDelayInitialize([name, this]()
					{
						m_classPtr = new ClassMeta<ClassType>(name);
						assert(m_classPtr);
						if (m_classPtr)
						{
							addClass(m_classPtr);
							ClassType::initMeta(*this);
						}
						else
							Log::Critical().log("Unable to allocate new ClassMeta!");
					}
				);
			}
			~MetaInitializer() = default;

			template<auto member>
				requires std::is_member_object_pointer_v<decltype(member)>
			void addProperty(const std::string& name)
			{
				using V = member_value_type_t<member>;
				internalAddMemberProperty(new MemberProperty<ClassType, V>{ name, member });
			}

			template<auto setter, auto getter>
			requires std::is_member_function_pointer_v<decltype(setter)>
				  && std::is_member_function_pointer_v<decltype(getter)>
			void addProperty(const std::string& name)
			{
				// I find the static asserts provide better feedback on errors as opposed to just having these as a requires clause
				static_assert(is_member_setter_function_pointer<std::remove_cvref_t<member_function_return_type_t<getter>>, ClassType, decltype(setter)>::value
					, "Setter must match this signature where T matches the return type of the getter: void(T)");
				static_assert(is_member_getter_function_pointer<std::remove_cvref_t<member_function_return_type_t<getter>>, ClassType, decltype(getter)>::value
					, "Getter must match this signature where T matches the input type to the setter: T(void) const");
				using V = std::remove_reference_t<std::remove_const_t<member_function_return_type_t<getter>>>;
				internalAddMemberProperty(new MemberPropertyFunctional<ClassType, V>{ name, setter, getter });
			}

		private:
			void internalAddMemberProperty(MemberPropertyBase* prop)
			{
				assert(prop);
				if (!prop)
				{
					Log::Critical().log("Unable to allocate memory for new property!");
					return;
				}

				auto foundProp = std::find_if(m_classPtr->props.begin(), m_classPtr->props.end(), [prop](const MemberPropertyBase* comp) {return comp->getName() == prop->getName(); });
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

			ClassMetaBase* m_classPtr;
		};
	}
}
