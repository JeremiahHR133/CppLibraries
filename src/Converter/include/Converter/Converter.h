#pragma once

#include <Converter/ConverterExport.h>

#include <Logger/Logger.h>

#include <typeindex>
#include <functional>
#include <any>
#include <string>
#include <vector>
#include <assert.h>

// Register a global converter for a type
// A converter is a binding of a classname -> <toStringFunc, fromStringFunc>
// Registering a type as a converter makes it available for the 
// getStringForType and getTypeFromString functions
// Only call this macro once per type.
#define REGISTER_CONVERTER(classname, toStringFunc, fromStringFunc) \
	Converter::Impl::registerConverter<classname>(#classname, toStringFunc, fromStringFunc);

// Library to convert from any registered type to a string and vice versa
// See the REGISTER_CONVERTER macro for how to register a type
// Many defualt types are alreay registered
namespace Converter
{
	template<typename T>
	using ToStringFunc = std::function<std::string(const T&)>;
	template<typename T>
	using FromStringFunc = std::function<T(const std::string&)>;

	struct CONVERTER_EXPORT ConverterInfo
	{
		std::string name;
		std::type_index index;
		ToStringFunc<std::any> toStr;
		FromStringFunc<std::any> fromStr;
	};

	// Implementation specifics
	// No functions in this namespace should be called directly
	namespace Impl
	{
		CONVERTER_EXPORT void addConverter(const ConverterInfo& info);
		CONVERTER_EXPORT const std::vector<ConverterInfo>& getRegisteredConverters();

		CONVERTER_EXPORT std::string getStrUsingConverter(const ConverterInfo& converter, const std::any& val);
		CONVERTER_EXPORT std::any getAnyUsingConverter(const ConverterInfo& converter, const std::string& val);

		CONVERTER_EXPORT const ConverterInfo* findConverter(const std::string& name);
		CONVERTER_EXPORT const ConverterInfo* findConverter(const std::type_index& index);

		template<typename T>
		void registerConverter(const std::string& name, ToStringFunc<T> toString, FromStringFunc<T> fromString)
		{
			Impl::addConverter(
				ConverterInfo
				{
					.name    = name,
					.index   = std::type_index(typeid(T)),
					.toStr   = [toString](const std::any& val) -> std::string { return toString(std::any_cast<T>(val)); },
					.fromStr = [fromString](const std::string& val) -> std::any { return std::any(fromString(val));  },
				}
			);
		}
	}

	// Call this funciton once at program initialization
	// This library uses the logging library and depends on logging
	// being initialized before the call to initializeConverters
	CONVERTER_EXPORT void initializeConverters();

	// Use the registered converter to turn the type T into a string
	template<typename T>
	std::string getStringForType(const T& val)
	{
		if (auto* converter = Impl::findConverter(std::type_index(typeid(T))))
			return Impl::getStrUsingConverter(*converter, std::any(val));

		return "";
	}

	// Use the registered converter to turn a string into type T
	template<typename T>
	T getTypeFromString(const std::string& str)
	{
		if (auto* converter = Impl::findConverter(std::type_index(typeid(T))))
		{
			try
			{
				return std::any_cast<T>(Impl::getAnyUsingConverter(*converter, str));
			}
			catch (const std::bad_any_cast& e)
			{
				Log::Error().log(
					"Unable to convert string to type; could not cast converter result to type: {}! "
					"Attempted to use converter with name: {}",
					std::type_index(typeid(T)), converter->name
				);
				assert(false && "Caught bad any cast!");
			}
		}

		return T();
	}

	// Use a type index to convert val into a string using a registered converter
	CONVERTER_EXPORT std::string getStringFromAny(const std::type_index& index, const std::any val);
	// Use a name lookup to convert val into a string using a registered converter
	CONVERTER_EXPORT std::string getStringFromAny(const std::string& name, const std::any val);
}
