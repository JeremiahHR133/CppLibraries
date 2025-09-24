#include <Converter/Converter.h>

#include <mutex>


namespace
{
	// Delay the initialization of converters until the initialize function is called
	std::mutex g_delayConvertersMutex;
	std::vector<Converter::ConverterInfo> g_delayConverters;
	std::vector<Converter::ConverterInfo> g_converters;
	bool g_convertersRegistered = false;
}

namespace Converter
{

	namespace Impl
	{
		void addConverter(const ConverterInfo& info)
		{
			std::lock_guard<std::mutex> lock(g_delayConvertersMutex);
			g_delayConverters.push_back(info);
		}

		const std::vector<ConverterInfo>& getRegisteredConverters()
		{
			return g_converters;
		}
	}

	void initializeConverters()
	{
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

		if (g_convertersRegistered)
		{
			Log::Warn().log("initializeConverters has already been called!");
			return;
		}
		for (const auto& delayConverter : g_delayConverters)
		{
			auto findConverter = std::find_if(g_converters.begin(), g_converters.end(),
				[&delayConverter](const ConverterInfo& comp) { return delayConverter.index == comp.index; }
			);
			if (findConverter == g_converters.end())
			{
				g_converters.push_back(delayConverter);
				Log::Info().log("Successfully registered converter for type: {}", delayConverter.name);
			}
			else
			{
				// Should not happen as uniqueness is compile time enforced unless namespace shennanigains are used
				Log::Warn().log("Converter already registered for type: {}", delayConverter.name);
			}
		}

		g_convertersRegistered = true;
	}

	std::string getStringFromAny(std::string_view typeName, const std::any val)
	{
		auto findConverter = std::find_if(Impl::getRegisteredConverters().begin(), Impl::getRegisteredConverters().end(),
			[typeName](const ConverterInfo& comp) { return typeName == comp.name; }
		);
		if (findConverter == Impl::getRegisteredConverters().end())
		{
			Log::Error().log("Unable to convert to string! Converter not registered for type: {}!", typeName);
		}
		else
		{
			try
			{
				return findConverter->toStr(val);
			}
			catch (const std::bad_any_cast& e)
			{
				assert(false && "Caught bad any cast!");
				Log::Error().log("Unable to convert any to string! toStr failed! Attempted to use converter with name: {}", findConverter->name);
			}
		}

		return "";
	}
}