#include <Logger/Logger.h>
#include <Converter/Converter.h>

#include <iostream>

int main()
{
	Log::initLogging(std::cout, std::cerr);
	Converter::initializeConverters();

	Log::Debug().log("Debug log {}", "example!");
	Log::Info().log("Info log {}", "example!");
	Log::Warn().log("Warn log {}", "example!");
	Log::Error().log("Error log {}", "example!");
	Log::Critical().log("Critical log {}", "example!");

	Log::Info().log("Test double log").log("   1").log("   2");

	Log::Info().log("Test of indentation!");
	Log::Info(1).log("Test of indentation!");
	Log::Info(2).log("Test of indentation!");
}