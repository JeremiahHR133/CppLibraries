#pragma once

#include <Converter/ConverterExport.h>

#include <Logger/Logger.h>

#include <typeindex>
#include <functional>
#include <any>
#include <string>
#include <vector>

// Register a global converter for a type
// A converter is a binding of a classname -> <toStringFunc, fromStringFunc>
// Registering a type as a converter makes it available for the 
// getStringForType and getTypeFromString functions
// Only call this macro once per type. This is compile time enforced.
#define REGISTER_CONVERTER(classname, toStringFunc, fromStringFunc) \
	namespace Converter::Impl::Type_ ## classname \
	{ \
	struct ConverterRegister \
	{ \
		ConverterRegister() \
		{ \
			Converter::Impl::registerConverter<classname>(#classname, toStringFunc, fromStringFunc); \
		} \
	}; \
	static const ConverterRegister _r; \
	} \

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
		const char* name;
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

		template<typename T>
		void registerConverter(const char* name, ToStringFunc<T> toString, FromStringFunc<T> fromString)
		{
			Impl::addConverter(
				ConverterInfo
				{
					.name    = name,
					.index   = std::type_index(typeid(T)),
					.toStr   = [&toString](const std::any& val) -> std::string { return toString(std::any_cast<T>(val)); },
					.fromStr = [&fromString](const std::string& val) -> std::any { return std::any(fromString(val));  },
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
		auto findConverter = std::find_if(Impl::getRegisteredConverters().begin(), Impl::getRegisteredConverters().end(),
			[](const ConverterInfo& comp) { return std::type_index(typeid(T)) == comp.index; }
		);
		if (findConverter == Impl::getRegisteredConverters().end())
		{
			Log::Error().log("Unable to convert to string! Converter not registered for type!");
		}
		else
		{
			return findConverter->toStr(std::any(val));
		}

		return "";
	}

	// Use the registered converter to turn a string into type T
	template<typename T>
	T getTypeFromString(const std::string& str)
	{
		auto findConverter = std::find_if(Impl::getRegisteredConverters().begin(), Impl::getRegisteredConverters().end(),
			[](const ConverterInfo& comp) { return std::type_index(typeid(T)) == comp.index; }
		);
		if (findConverter == Impl::getRegisteredConverters().end())
		{
			Log::Error().log("Unable to convert from string! Converter not registered for type!");
		}
		else
		{
			return std::any_cast<T>(findConverter->fromStr(str));
		}

		return T();
	}
}

REGISTER_CONVERTER(
	int, 
	[](const int& i) { return std::to_string(i); },
	[](const std::string& str) { return std::stoi(str); }
)
REGISTER_CONVERTER(
	float, 
	[](const float& f) { return std::to_string(f); },
	[](const std::string& str) { return std::stof(str); }
)
REGISTER_CONVERTER(
	double, 
	[](const double& d) { return std::to_string(d); },
	[](const std::string& str) { return std::stod(str); }
)
REGISTER_CONVERTER(
	bool, 
	[](const bool& b) { return std::to_string(b); },
	[](const std::string& str) { return static_cast<bool>(std::stoi(str)); }
)
REGISTER_CONVERTER(
	std::string, 
	[](const std::string& str) { return str; },
	[](const std::string& str) { return str; }
)
