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
}