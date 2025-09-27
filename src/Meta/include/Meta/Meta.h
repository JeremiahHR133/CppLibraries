#pragma once

#include <Meta/MetaExport.h>

#include <any>
#include <vector>
#include <string_view>
#include <assert.h>
#include <functional>
#include <type_traits>
#include <typeindex>
#include <tuple>

#include <Logger/Logger.h>

// Declare an object as exported to the meta system
// Use this in the class definition, as shown: 
//		class ExampleStruct : public Meta::MetaObject
//		{
//			DECLARE_META_OBJECT(ExampleStruct)
//		public:
//          ...
//		};
#define DECLARE_META_OBJECT(classname) \
	private: \
		friend Meta::Impl::MetaInitializer<classname>; \
		static Meta::Impl::MetaInitializer<classname> s_metaInit; \
		static void initMeta(Meta::Impl::MetaInitializer<classname>& w); \
	public: \
		std::type_index getTypeIndex() const override;

// Implement an object that has been declared as a meta object
// Use this in the class implementation, as shown:
// 		IMPLEMENT_META_OBJECT(ExampleStruct)
// 		{
// 			w.addProperty<&ExampleStruct::one>("one");
// 			w.addProperty<&ExampleStruct::two>("two");
// 			w.addProperty<&ExampleStruct::setThree, &ExampleStruct::getThree>("three");
// 			w.addFunction<&ExampleStruct::exampleRandomFunction>("randomFunction");
// 		}
//		ExampleStruct::ExampleStruct() = default;
#define IMPLEMENT_META_OBJECT(classname) \
	static_assert(std::is_base_of_v<Meta::MetaObject, classname>, "Meta objects must subclass the Meta::MetaObject class!"); \
	static_assert(std::is_default_constructible_v<classname>, "Class must be default constructible to use meta!"); \
	Meta::Impl::MetaInitializer<classname> classname::s_metaInit = Meta::Impl::MetaInitializer<classname>(#classname); \
	std::type_index classname::getTypeIndex() const { return std::type_index(typeid(classname)); } \
	void classname::initMeta(Meta::Impl::MetaInitializer<classname>& w)

// Simple meta system
// Works off of properties added via the imlementation macro
// See the macros at the top of this file for examples
namespace Meta
{
	namespace Impl
	{
		//
		// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
		// Template Meta-Programming Support
		// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
		//

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

		template <typename... Args>
		struct is_const_member_function : std::false_type {};
		template <typename C, typename R, typename... Args>
		struct is_const_member_function<R (C::*)(Args...) const> : std::true_type {};

		template <typename Func>
		struct function_args_deducer
		{
			static_assert(sizeof(Func) == 0, "Template parameter is not a member function pointer");
		};
		template <typename R, typename C, typename... Args>
		struct function_args_deducer<R (C::*)(Args...)>
		{
			using return_type = R;
			using class_type = C;
			template <template <typename, typename, typename...> class Apply>
			using apply_to = Apply<C, R, Args...>;
		};
		template <typename R, typename C, typename... Args>
		struct function_args_deducer<R (C::*)(Args...) const>
		{
			using return_type = R;
			using class_type = C;
			template <template <typename, typename, typename...> class Apply>
			using apply_to = Apply<C, R, Args...>;
		};

		template <typename T, typename C, typename func>
		struct is_member_setter_function_pointer : std::false_type {};
		template <typename T, typename C>
		struct is_member_setter_function_pointer<T, C, void (C::*)(T)> : std::true_type {};

		template <typename T, typename C, typename func>
		struct is_member_getter_function_pointer : std::false_type {};
		template <typename T, typename C>
		struct is_member_getter_function_pointer<T, C, T (C::*)(void) const> : std::true_type {};

	}

	// The Meta Object!
	// The base of all classes exposed to the meta system.
	class MetaObject
	{
	public:
		MetaObject() = default;
		virtual ~MetaObject() = default;

		virtual std::type_index getTypeIndex() const = 0;
	};

	//
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	// Function Properties
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//

	// Base for all free member function properties
	// This is not the base of member properties that use member setters and getters 
	class MemberFunctionPropBase
	{
	public:
		MemberFunctionPropBase(const std::string& name)
			: name(name)
		{
		}
		virtual ~MemberFunctionPropBase() = default;

		virtual std::type_index getTypeIndex() const = 0;
		const std::string& getName() const { return name; }

	private:
		std::string name;
	};

	// Base for non-const member functions
	// This is not the base of member properties that use member setters and getters 
	class MemberNonConstFunctionPropBase : public MemberFunctionPropBase
	{
	public:
		MemberNonConstFunctionPropBase(const std::string& name)
			: MemberFunctionPropBase(name)
		{
		}

		virtual std::any invoke(MetaObject& obj, const std::vector<std::any>& args) const = 0;
	};

	// Non-const "free" (not bound as a setter/getter pair) member function property
	template <typename ClassType, typename ReturnType, typename... Args>
	class MemberNonConstFunctionProp : public MemberNonConstFunctionPropBase
	{
	public:
		MemberNonConstFunctionProp(const std::string& name, ReturnType (ClassType::* func)(Args...))
			: MemberNonConstFunctionPropBase(name)
			, func(func)
		{
		}

		std::any invoke(MetaObject& obj, const std::vector<std::any>& args) const override
		{
			if (args.size() != sizeof...(Args))
			{
				assert(false && "Wrong number of arguments provided to function invocation!");
				Log::Error().log("Wrong number of arguments provided to function invocation!");
				return std::any();
			}

			if (obj.getTypeIndex() == std::type_index(typeid(ClassType)))
			{
				try
				{
					return invoke_impl<Args...>(static_cast<ClassType&>(obj), args, std::index_sequence_for<Args...>{});
				}
				catch (const std::bad_any_cast& e)
				{
					assert(false && "Bad any cast in function arguments!");
					Log::Error().log("At least one function argument is of the wrong type!");
				}
			}
			else
			{
				assert(false && "Property does not belong to given object!");
				Log::Error().log("Failed to invoke for prop! Property does not belong to given object!");
			}
			return std::any();
		}

		std::type_index getTypeIndex() const override
		{
			return std::type_index(typeid(ReturnType));
		}

	private:
		template <typename... Args, std::size_t... Is>
		std::any invoke_impl(ClassType& obj, const std::vector<std::any>& args, std::index_sequence<Is...>) const
		{
			return std::any((obj.*func)(std::any_cast<Args>(args[Is])...));
		}

		ReturnType (ClassType::* func)(Args...);
	};

	// Base for const member functions
	// This is not the base of member properties that use member setters and getters 
	class MemberConstFunctionPropBase : public MemberFunctionPropBase
	{
	public:
		MemberConstFunctionPropBase(const std::string& name)
			: MemberFunctionPropBase(name)
		{
		}

		virtual std::any invoke(const MetaObject& obj, const std::vector<std::any>& args) const = 0;
	};

	// Const "free" (not bound as a setter/getter pair) member function property
	template <typename ClassType, typename ReturnType, typename... Args>
	class MemberConstFunctionProp : public MemberConstFunctionPropBase
	{
	public:
		MemberConstFunctionProp(const std::string& name, ReturnType (ClassType::* func)(Args...) const)
			: MemberConstFunctionPropBase(name)
			, func(func)
		{
		}

		std::any invoke(const MetaObject& obj, const std::vector<std::any>& args) const override
		{
			if (args.size() != sizeof...(Args))
			{
				assert(false && "Wrong number of arguments provided to function invocation!");
				Log::Error().log("Wrong number of arguments provided to function invocation!");
				return std::any();
			}

			if (obj.getTypeIndex() == std::type_index(typeid(ClassType)))
			{
				try
				{
					return invoke_impl<Args...>(static_cast<const ClassType&>(obj), args, std::index_sequence_for<Args...>{});
				}
				catch (const std::bad_any_cast& e)
				{
					assert(false && "Bad any cast in function arguments!");
					Log::Error().log("At least one function argument is of the wrong type!");
				}
			}
			else
			{
				assert(false && "Property does not belong to given object!");
				Log::Error().log("Failed to invoke for prop! Property does not belong to given object!");
			}
			return std::any();
		}

		std::type_index getTypeIndex() const override
		{
			return std::type_index(typeid(ReturnType));
		}

	private:
		template <typename... Args, std::size_t... Is>
		std::any invoke_impl(const ClassType& obj, const std::vector<std::any>& args, std::index_sequence<Is...>) const
		{
			return std::any((obj.*func)(std::any_cast<Args>(args[Is])...));
		}

		ReturnType (ClassType::* func)(Args...) const;
	};

	//
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	// Member Properties
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//

	// The base of all member properties
	// This includes member properties that work off of a setter/getter pair
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

	// The templated base for all member properties
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

	// A simple member property that operates off of a pointer to a class member
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

	// A member property that operates off of a setter/getter member function pair
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

	//
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	// Base Meta Storage Class
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//

	// The primary interface for interacting with meta-data
	// The ClassMetaBase is the class used to store and access the property info
	// Use the global Meta:: functions to get a ClassMetaBase pointer for a registered class
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
		const std::vector<const MemberNonConstFunctionPropBase*> getNonConstFuncs() const { return nonConstFunctions; }
		const MemberNonConstFunctionPropBase* getNonConstFunc(const std::string& name) const;
		const std::vector<const MemberConstFunctionPropBase*> getConstFuncs() const { return constFunctions; }
		const MemberConstFunctionPropBase* getConstFunc(const std::string& name) const;

	private:
		std::string name;
		std::vector<const MemberPropertyBase*> props;
		std::vector<const MemberNonConstFunctionPropBase*> nonConstFunctions;
		std::vector<const MemberConstFunctionPropBase*> constFunctions;

		template<typename T> friend class Impl::MetaInitializer;
	};

	// Small class used to store some information that is otherwise type-erased from the base class
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

	//
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	// Global Functions
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//

	// Call this function once at the start of the program to initialize all meta info
	// from any loaded libraries
	// This function is here to defer meta-initialization until after logging has been
	// initialized and shared libraries have been loaded.
	META_EXPORT void initializeMetaInfo();

	// Get a pointer to a classes meta info
	// Returns nullptr if the class was not found
	META_EXPORT const ClassMetaBase* getClassMeta(const std::type_index& index);
	// Get a pointer to a classes meta info
	// Returns nullptr if the class was not found
	META_EXPORT const ClassMetaBase* getClassMeta(const std::string& name);
	// Get a pointer to a classes meta info
	// Returns nullptr if the class was not found
	template<typename T>
	const ClassMetaBase* getClassMeta()
	{
		return getClassMeta(std::type_index(typeid(T)));
	}

	//
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	// Implementation Details
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//

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

			template<auto function>
				requires std::is_member_function_pointer_v<decltype(function)>
			void addFunction(const std::string& name)
			{
				// UGLY!!! DUPLICATED!!!
				// Probably a smell of a bad design... Think on this some
				if constexpr (is_const_member_function<decltype(function)>::value)
				{
					using ApplyArgs = function_args_deducer<decltype(function)>::template apply_to<MemberConstFunctionProp>;
					auto* func = new ApplyArgs{name, function};
					assert(func);
					if (!func)
					{
						Log::Critical().log("Unable to allocate memory for new function property!");
						return;
					}
					auto foundFunc = std::find_if(m_classPtr->constFunctions.begin(), m_classPtr->constFunctions.end(), [func](const MemberFunctionPropBase* comp) {return comp->getName() == func->getName(); });
					if (foundFunc == m_classPtr->constFunctions.end())
					{
						Log::Debug().log("Registered new const function property: \"{}\" to class: \"{}\"", func->getName(), m_classPtr->getName());
						m_classPtr->constFunctions.push_back(func);
					}
					else
					{
						assert(false && "Const function property already added for class");
						Log::Error().log("Failed to add const function property! Property already exists: \"{}\"", func->getName());
					}
				}
				else
				{
					using ApplyArgs = function_args_deducer<decltype(function)>::template apply_to<MemberNonConstFunctionProp>;
					auto* func = new ApplyArgs{name, function};
					assert(func);
					if (!func)
					{
						Log::Critical().log("Unable to allocate memory for new function property!");
						return;
					}
					auto foundFunc = std::find_if(m_classPtr->nonConstFunctions.begin(), m_classPtr->nonConstFunctions.end(), [func](const MemberFunctionPropBase* comp) {return comp->getName() == func->getName(); });

					if (foundFunc == m_classPtr->nonConstFunctions.end())
					{
						Log::Debug().log("Registered new non-const function property: \"{}\" to class: \"{}\"", func->getName(), m_classPtr->getName());
						m_classPtr->nonConstFunctions.push_back(func);
					}
					else
					{
						assert(false && "Non-const function property already added for class");
						Log::Error().log("Failed to add non-const function property! Property already exists: \"{}\"", func->getName());
					}
				}
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
