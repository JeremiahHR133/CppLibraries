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

// TODO: Make the fuction property base class generic based on a bool template parameter
//       that modifies a using clause to edit the function signature!

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
		std::type_index getTypeIndex() const override; \
		std::string getTypeName() const override;

// Implement an object that has been declared as a meta object
// Use this in the class implementation, as shown:
// 		IMPLEMENT_META_OBJECT(ExampleStruct)
// 		{
// 			w.addMember<&ExampleStruct::one>("one");
// 			w.addMember<&ExampleStruct::two>("two");
// 			w.addMember<&ExampleStruct::setThree, &ExampleStruct::getThree>("three");
// 			w.addFunction<&ExampleStruct::exampleRandomFunction>("randomFunction");
// 		}
//		ExampleStruct::ExampleStruct() = default;
#define IMPLEMENT_META_OBJECT(classname) \
	static_assert(std::is_base_of_v<Meta::MetaObject, classname>, "Meta objects must subclass the Meta::MetaObject class!"); \
	static_assert(std::is_default_constructible_v<classname>, "Class must be default constructible to use meta!"); \
	Meta::Impl::MetaInitializer<classname> classname::s_metaInit = Meta::Impl::MetaInitializer<classname>(#classname); \
	std::type_index classname::getTypeIndex() const { return std::type_index(typeid(classname)); } \
	std::string classname::getTypeName() const { return #classname; } \
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
			// Boy this is crap.... 
			template <bool B, template <bool, typename, typename, typename...> class Apply>
			using apply_to_1 = Apply<B, C, R, Args...>;
			template <bool B, template <bool, typename...> class Apply>
			using apply_to_2 = Apply<B, Args...>;
		};
		template <typename R, typename C, typename... Args>
		struct function_args_deducer<R (C::*)(Args...) const>
		{
			using return_type = R;
			using class_type = C;
			// Boy this is crap.... 
			template <bool B, template <bool, typename, typename, typename...> class Apply>
			using apply_to_1 = Apply<B, C, R, Args...>;
			template <bool B, template <bool, typename...> class Apply>
			using apply_to_2 = Apply<B, Args...>;
		};

		template <typename T, typename C, typename func>
		struct is_member_setter_function_pointer : std::false_type {};
		template <typename T, typename C>
		struct is_member_setter_function_pointer<T, C, void (C::*)(T)> : std::true_type {};

		template <typename T, typename C, typename func>
		struct is_member_getter_function_pointer : std::false_type {};
		template <typename T, typename C>
		struct is_member_getter_function_pointer<T, C, T (C::*)(void) const> : std::true_type {};

		template <typename... Ts>
		std::string pack_as_type_names()
		{
			std::string ret;
			((ret += (ret.empty() ? "" : ", ") + std::string(std::type_index(typeid(Ts)).name())), ...);
			return ret;
		}
	}

	// The Meta Object!
	// The base of all classes exposed to the meta system.
	// Make sure any types exposed to the meta system subclass MetaObject
	class MetaObject
	{
	public:
		MetaObject() = default;
		virtual ~MetaObject() = default;

		virtual std::type_index getTypeIndex() const = 0;
		virtual std::string getTypeName() const = 0;
	};

	// Base class for all properties
	class Prop
	{
	public:
		Prop(const std::string& name, const std::string& className)
			: name(name)
			, className(className)
		{
		}
		virtual ~Prop() = default;

		virtual std::type_index getTypeIndex() const = 0;
		const std::string& getName() const { return name; }
		const std::string& getClassName() const { return className; }
		const std::string& getDescription() const { return description; }
		void setDescription(const std::string& str) { description = str; }

	private:
		std::string name;
		std::string className;
		std::string description;
	};

	//
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	// Function Properties
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//

	// Base for "free" (not a bound setter/getter pair) member function properties
	// This is not the base of member properties that use member setters and getters 
	// The class must be templated to differentiate between const and non-const functions
	template <bool isConst>
	class MemberFunctionPropBase : public Prop
	{
	public:
		using MetaObjSignature = std::conditional_t<isConst, const MetaObject&, MetaObject&>;
		MemberFunctionPropBase(const std::string& name, const std::string& className)
			: Prop(name, className)
		{
		}

		virtual std::any invoke(MetaObjSignature obj, const std::vector<std::any>& args) const = 0;

		void setDefaultArgs(const std::vector<std::any>& args) { defaultArgs = args; };
		// Assuming you won't invoke with default args when the function takes no args...
		std::any invokeDefaultArgs(MetaObjSignature obj) const
		{
			if (defaultArgs.size())
				return invoke(obj, defaultArgs);
			else
			{
				Log::Error().log("Unable to invoke with default args! Property was never given default args!");
				assert(false && "Property was never given default args!");
			}

			return std::any();
		}

	private:
		std::vector<std::any> defaultArgs;
	};

	using MemberConstFunctionPropBase = MemberFunctionPropBase<true>;
	using MemberNonConstFunctionPropBase = MemberFunctionPropBase<false>;

	// "free" (not bound as a setter/getter pair) member function property
	template <bool isConst, typename ClassType, typename ReturnType, typename... Args>
	class MemberFunctionProp : public MemberFunctionPropBase<isConst>
	{
	public:
		using ClassTypeSignature = std::conditional_t<isConst, const ClassType&, ClassType&>;
		using FunctionSignature  = std::conditional_t<isConst, ReturnType (ClassType::*)(Args...) const, ReturnType (ClassType::*)(Args...)>;

		MemberFunctionProp(const std::string& name, const std::string& className, FunctionSignature func)
			: MemberFunctionPropBase<isConst>(name, className)
			, func(func)
		{
		}

		std::any invoke(MemberFunctionPropBase<isConst>::MetaObjSignature obj, const std::vector<std::any>& args) const override
		{
			if (args.size() != sizeof...(Args))
			{
				Log::Error().log("Wrong number of arguments provided to function invocation!");
				Log::Error(1).log("Expected: {}({})", Prop::getName(), Impl::pack_as_type_names<Args...>());
				Log::Error(1).log("Given {} args!", args.size());
				assert(false && "Wrong number of arguments provided to function invocation!");
				return std::any();
			}

			if (obj.getTypeIndex() == std::type_index(typeid(ClassType)))
			{
				try
				{
					return invoke_impl<Args...>(static_cast<ClassTypeSignature>(obj), args, std::index_sequence_for<Args...>{});
				}
				catch (const std::bad_any_cast& e)
				{
					Log::Error().log("At least one function argument is of the wrong type!");
					Log::Error(1).log("Expected: {}({})", Prop::getName(), Impl::pack_as_type_names<Args...>());
					assert(false && "Bad any cast in function arguments!");
				}
			}
			else
			{
				Log::Error().log("Failed to invoke for prop! Property \"{}\" belongs to \"{}\" but given \"{}\"!", Prop::getName(), Prop::getClassName(), obj.getTypeName());
				assert(false && "Property does not belong to given object!");
			}
			return std::any();
		}

		std::type_index getTypeIndex() const override
		{
			return std::type_index(typeid(ReturnType));
		}

	private:
		template <typename... Args, std::size_t... Is>
		std::any invoke_impl(ClassTypeSignature obj, const std::vector<std::any>& args, std::index_sequence<Is...>) const
		{
			return std::any((obj.*func)(std::any_cast<Args>(args[Is])...));
		}

		FunctionSignature func;
	};

	//
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	// Member Properties
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//

	// The base of all member properties
	// This includes member properties that work off of a setter/getter pair
	class MemberPropertyBase : public Prop
	{
	public:
		MemberPropertyBase(const std::string& name, const std::string& className)
			: Prop(name, className)
			, readOnly(false)
		{
		}
		virtual ~MemberPropertyBase() = default;

		virtual std::any createDefaultAsAny() const = 0;
		virtual std::any getAsAny(const MetaObject& obj) const = 0;
		virtual void setFromAny(MetaObject& obj, const std::any& val) const = 0;

		void setDefault(const std::any& val) { defaultValue = val; }
		void setReadOnly() { readOnly = true; }
		bool getReadOnly() const { return readOnly; }
		void applyDefault(MetaObject& obj) const
		{
			if (defaultValue.has_value())
				setFromAny(obj, defaultValue);
			else
			{
				Log::Error().log("Unable to apply default because one was never set!");
				assert(false && "Property was never given a default value!");
			}
		}

		template <typename T>
		T getAsType(const MetaObject& obj) const
		{
			if (std::type_index(typeid(T)) == getTypeIndex())
			{
				try
				{
					return std::any_cast<T>(getAsAny(obj));
				}
				catch (const std::bad_any_cast& e)
				{
					// Should never get here because we already did the type check!!
					Log::Error().log("Unable to getAsType for \"{}\"! Any cast failed!", getName());
					assert(false && "Any cast failed to turn type into T!");
				}
			}
			else
			{
				Log::Error().log("Unable to getAsType for \"{}\"! Type T does not match propertie's type!", getName());
				assert(false && "Type T does not match propertie's type!");
			}

			return T();
		}

	private:
		std::any defaultValue;
		bool readOnly;
	};

	namespace Impl
	{
		template<typename T>
		class MetaInitializer;
	}

	// The templated base for all member properties
	// Handles basic typeinfo operations shared between properties
	template<class ClassType, typename MemberType>
	class TemplateMemberPropertyBase : public MemberPropertyBase
	{
	public:
		TemplateMemberPropertyBase(const std::string& name, const std::string& className)
			: MemberPropertyBase(name, className)
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
				Log::Error().log("Failed to getAsAny for \"{}\"! Property does not belong to given object!", getName());
				assert(false && "Property does not belong to given object!");
			}
			return std::any();
		}

		void setFromAny(MetaObject& obj, const std::any& val) const override
		{
			if (getReadOnly())
			{
				Log::Error().log("Refusing to set value on read only property!");
				assert(false && "Refusing to set read only property!");
				return;
			}

			if (obj.getTypeIndex() == std::type_index(typeid(ClassType)))
			{
				try
				{
					set(static_cast<ClassType&>(obj), std::any_cast<MemberType>(val));
				}
				catch (const std::bad_any_cast& e)
				{
					Log::Error().log("Failed to set property for \"{}\"! Given 'any' is the wrong type!", getName());
					assert(false && "Attempted to set property with 'any' of wrong type!");
				}
			}
			else
			{
				Log::Error().log("Failed to setFromAny for \"{}\"! Property does not belong to given object!", getName());
				assert(false && "Property does not belong to given object!");
			}
		}
	};

	// A simple member property that operates off of a pointer to a class member
	template<class ClassType, typename MemberType>
	class MemberProperty : public TemplateMemberPropertyBase<ClassType, MemberType>
	{
		friend Impl::MetaInitializer<ClassType>;

		// Creation is limited to the initializer class
		MemberProperty(const std::string& name, const std::string& className, MemberType ClassType::* ptr)
			: TemplateMemberPropertyBase<ClassType, MemberType>(name, className)
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
		MemberPropertyFunctional(const std::string& name, const std::string& className, void (ClassType::*setter)(MemberType), MemberType (ClassType::*getter)(void) const)
			: TemplateMemberPropertyBase<ClassType, MemberType>(name, className)
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
	// Property Setters
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//

	namespace Impl
	{
		template<typename ClassType>
		class MetaInitializer;
	}

	template<typename PS>
	class PropSetter
	{
	protected:
		template <typename T>
		friend class Impl::MetaInitializer;

		PropSetter(Prop* prop)
			: prop(prop)
		{
		}

		void pointerFailed() const
		{
			Log::Error().log("Unable to modify function prop pointer as it was null!");
			assert(false && "Unable to modify function prop pointer as it was null!");
		}
	public:
		PS& setDescription(const std::string& str)
		{
			if (prop)
				prop->setDescription(str);
			else
				pointerFailed();

			return static_cast<PS&>(*this);
		}

		//
		// Prevent usage of the class outside the intended context
		// The prop-setter should only be used in the IMPLEMENT_META_OBJECT function
		//
		PropSetter(const PropSetter&) = delete;
		PropSetter(PropSetter&&) = delete;
		PropSetter& operator=(const PropSetter&) = delete;
		PropSetter& operator=(PropSetter&&) = delete;

	private:
		Prop* prop;
	};

	template<bool isConst, typename... Args>
	class FunctionPropSetter : public PropSetter<FunctionPropSetter<isConst, Args...>>
	{
		template <typename T>
		friend class Impl::MetaInitializer;

		FunctionPropSetter(MemberFunctionPropBase<isConst>* prop)
			: PropSetter<FunctionPropSetter>(prop)
			, fprop(prop)
		{
		}
		MemberFunctionPropBase<isConst>* fprop;
	public:

		FunctionPropSetter& setDefaultArgs(Args... args)
		{
			if (fprop)
				fprop->setDefaultArgs(std::vector<std::any>{args...});
			else
				PropSetter<FunctionPropSetter<isConst, Args...>>::pointerFailed();

			return *this;
		}

		//
		// Prevent usage of the class outside the intended context
		// The prop-setter should only be used in the IMPLEMENT_META_OBJECT function
		//
		FunctionPropSetter(const FunctionPropSetter&) = delete;
		FunctionPropSetter(FunctionPropSetter&&) = delete;
		FunctionPropSetter& operator=(const FunctionPropSetter&) = delete;
		FunctionPropSetter& operator=(FunctionPropSetter&&) = delete;
	};

	template <typename PType>
	class MemberPropSetter : public PropSetter<MemberPropSetter<PType>>
	{
		template <typename T>
		friend class Impl::MetaInitializer;

		MemberPropSetter(MemberPropertyBase* prop)
			: PropSetter<MemberPropSetter<PType>>(prop)
			, mprop(prop)
		{
		}
		MemberPropertyBase* mprop;
	public:

		MemberPropSetter& setDefault(const PType& val)
		{
			if (mprop)
				mprop->setDefault(std::any(val));
			else
				PropSetter<MemberPropSetter<PType>>::pointerFailed();

			return *this;
		}

		MemberPropSetter& setReadOnly()
		{
			if (mprop)
				mprop->setReadOnly();
			else
				PropSetter<MemberPropSetter<PType>>::pointerFailed();

			return *this;
		}
		//
		// Prevent usage of the class outside the intended context
		// The prop-setter should only be used in the IMPLEMENT_META_OBJECT function
		//
		MemberPropSetter(const MemberPropSetter&) = delete;
		MemberPropSetter(MemberPropSetter&&) = delete;
		MemberPropSetter& operator=(const MemberPropSetter&) = delete;
		MemberPropSetter& operator=(MemberPropSetter&&) = delete;
	};


	//
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	// Base Meta Storage Class
	// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
	//

	// The primary interface for operating on meta-data
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
		const std::vector<const MemberPropertyBase*> getMemberProps() const { return props; }
		const MemberPropertyBase* getMemberProp(const std::string& name) const;
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
						if (m_classPtr)
						{
							addClass(m_classPtr);
							ClassType::initMeta(*this);
						}
						else
						{
							Log::Critical().log("Unable to allocate new ClassMeta for \"{}\"!", name);
							assert(m_classPtr);
						}
					}
				);
			}
			~MetaInitializer() = default;

			template<auto member>
				requires std::is_member_object_pointer_v<decltype(member)>
			auto addMember(const std::string& name)
			{
				using V = member_value_type_t<member>;
				auto* prop = internalAddMemberProperty(new MemberProperty<ClassType, V>{ name, m_classPtr->getName(), member}, name);
				return MemberPropSetter<V>(prop);
			}

			template<auto setter, auto getter>
			requires std::is_member_function_pointer_v<decltype(setter)>
				  && std::is_member_function_pointer_v<decltype(getter)>
			auto addMember(const std::string& name)
			{
				// I find the static asserts provide better feedback on errors as opposed to just having these as a requires clause
				static_assert(is_member_setter_function_pointer<std::remove_cvref_t<member_function_return_type_t<getter>>, ClassType, decltype(setter)>::value
					, "Setter must match this signature where T matches the return type of the getter: void(T)");
				static_assert(is_member_getter_function_pointer<std::remove_cvref_t<member_function_return_type_t<getter>>, ClassType, decltype(getter)>::value
					, "Getter must match this signature where T matches the input type to the setter: T(void) const");
				using V = std::remove_reference_t<std::remove_const_t<member_function_return_type_t<getter>>>;
				auto* prop = internalAddMemberProperty(new MemberPropertyFunctional<ClassType, V>{ name, m_classPtr->getName(), setter, getter}, name);
				return MemberPropSetter<V>(prop);
			}

			template<auto function>
				requires std::is_member_function_pointer_v<decltype(function)>
			auto addFunction(const std::string& name)
			{
				constexpr bool isConst = is_const_member_function<decltype(function)>::value;
				using ApplyArgs = function_args_deducer<decltype(function)>::template apply_to_1<isConst, MemberFunctionProp>;
				auto* prop = internalAddMemberFunc<ApplyArgs, isConst, function>(name);
				using ApplySetter = function_args_deducer<decltype(function)>::template apply_to_2<isConst, FunctionPropSetter>; 
				return ApplySetter(prop);
			}

		private:
			template <bool isConst>
			constexpr auto get_member_vec_ptr() requires(isConst)
			{
				return &ClassMetaBase::constFunctions;
			}
			template <bool isConst>
			constexpr auto get_member_vec_ptr() requires(!isConst)
			{
				return &ClassMetaBase::nonConstFunctions;
			}
			template <typename Constructor, bool isConst, auto function>
			auto internalAddMemberFunc(const std::string& name)
			{
				// lol I kinda like this
				auto vecPtr = get_member_vec_ptr<isConst>();
				auto& vec = m_classPtr->*vecPtr;

				auto foundFunc = std::find_if(vec.begin(), vec.end(), [&name](const auto* comp) {return comp->getName() == name; });
				if (foundFunc == vec.end())
				{
					auto* func = new Constructor{name, m_classPtr->getName(), function};
					if (!func)
					{
						Log::Critical().log("Unable to allocate memory for new function property \"{}\"!", name);
						assert(func);
						delete func;
						return static_cast<Constructor*>(nullptr);
					}
					Log::Debug().log("Registered new const function property: \"{}\" to class: \"{}\"", func->getName(), m_classPtr->getName());
					vec.push_back(func);
					return func;
				}
				else
				{
					Log::Error().log("Failed to add function property! Property \"{}\" already exists on class \"{}\"", name, m_classPtr->getName());
					assert(false && "Property already added for class");
				}
				return static_cast<Constructor*>(nullptr);
			}

			MemberPropertyBase* internalAddMemberProperty(MemberPropertyBase* prop, const std::string& name)
			{
				if (!prop)
				{
					Log::Critical().log("Unable to allocate memory for new property \"{}\"!", name);
					assert(prop);
					delete prop;
					return nullptr;
				}

				auto foundProp = std::find_if(m_classPtr->props.begin(), m_classPtr->props.end(), [prop](const MemberPropertyBase* comp) {return comp->getName() == prop->getName(); });
				if (foundProp == m_classPtr->props.end())
				{
					Log::Debug().log("Registered new property: \"{}\" to class: \"{}\"", prop->getName(), m_classPtr->getName());
					m_classPtr->props.push_back(prop);
					return prop;
				}
				else
				{
					Log::Error().log("Failed to add property! Property \"{}\" already exists on class \"{}\"", prop->getName(), m_classPtr->getName());
					assert(false && "Property already added for class");
					delete prop;
				}

				return nullptr;
			}

			ClassMetaBase* m_classPtr;
		};
	}
}
