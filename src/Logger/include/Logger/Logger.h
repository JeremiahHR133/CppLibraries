#pragma once

#include <Logger/LoggerExport.h>

#include <string_view>
#include <sstream>
#include <ostream>
#include <source_location>
#include <print>
#include <unordered_map>

// Simple logging library
// Logs to any std::ostream (provided at initialization)
// Currently does not support multithreaded environments
namespace Log
{
	// All ANSI color codes
	// These codes are bound to a map containing the string value of the color
	// that can be printed to a terminal that supports ANSI color escapes
	// Use getColorMap and getColorStr to interact with the colors
	enum class Color
	{
		reset,

		regular_black,
		regular_red,
		regular_green,
		regular_yellow,
		regular_blue,
		regular_purple,
		regular_cyan,
		regular_white,

		bold_black,
		bold_red,
		bold_green,
		bold_yellow,
		bold_blue,
		bold_purple,
		bold_cyan,
		bold_white,

		underline_black,
		underline_red,
		underline_green,
		underline_yellow,
		underline_blue,
		underline_purple,
		underline_cyan,
		underline_white,

		background_black,
		background_red,
		background_green,
		background_yellow,
		background_blue,
		background_purple,
		background_cyan,
		background_white,

		highIntensity_black,
		highIntensity_red,
		highIntensity_green,
		highIntensity_yellow,
		highIntensity_blue,
		highIntensity_purple,
		highIntensity_cyan,
		highIntensity_white,

		boldHighIntensity_black,
		boldHighIntensity_red,
		boldHighIntensity_green,
		boldHighIntensity_yellow,
		boldHighIntensity_blue,
		boldHighIntensity_purple,
		boldHighIntensity_cyan,
		boldHighIntensity_white,

		backgroundHighIntensity_black,
		backgroundHighIntensity_red,
		backgroundHighIntensity_green,
		backgroundHighIntensity_yellow,
		backgroundHighIntensity_blue,
		backgroundHighIntensity_purple,
		backgroundHighIntensity_cyan,
		backgroundHighIntensity_white,
	};

	// Logging level
	// Controls some aspects of the way messages are logged
	// depending on what logging options are set
	enum class Level
	{
		Debug,
		Info,
		Warning,
		Error,
		Critical,
	};

	// Options to control the behavior of the logger
	// These are set once at initialization
	struct LogInitOptions
	{
		bool printColor = true;
		bool printLocationInfo = true;
		bool reportLogInitialized = true;
		bool logFullFunctionName = false;
		std::string indentationLevel = "   ";

		struct ColorSettings
		{
			Color debug = Color::highIntensity_white;
			Color info = Color::highIntensity_green;
			Color warn = Color::highIntensity_yellow;
			Color error = Color::highIntensity_red;
			Color critical = Color::underline_red;
		}
		colorSettings;
	};

	// Returns a map of <color enum, string holding color escape code>
	LOGGER_EXPORT const std::unordered_map<Color, std::string>& getColorMap();
	// Returns a string holding the color escape code
	LOGGER_EXPORT const std::string& getColorStr(Color color);
	// Returns the string to be printed for a given level
	LOGGER_EXPORT std::string getStringForLevel(Level level);
	// Returns the color that the StringForLevel will be printed in
	LOGGER_EXPORT Color getColorForLevel(Level level);
	// Initializes global logging
	// Call this once at the start of a program
	// All logs will be written to 'stream'
	LOGGER_EXPORT void initLogging(std::ostream& stream, const LogInitOptions& opts = LogInitOptions());
	// Initializes global logging
	// Call this once at the start of a program
	// Normal logs will be written to primaryStream
	// Erorr logs (Error, Critical) will be written to errorStream
	LOGGER_EXPORT void initLogging(std::ostream& primaryStream, std::ostream& errorStream, const LogInitOptions& opts = LogInitOptions());
	// Simplifies the complex semi-mangled function names
	// Example:
	//    void __cdecl Log::initLogging(class std::basic_ostream<char,struct std::char_traits<char> > &,const struct Log::LogInitOptions &)
	// becomes
	//    Log::initLogging
	LOGGER_EXPORT std::string getSimpleFunctionName(std::string_view name);

	// Base class for loggers
	// Can be used directly, but is meant to be used via the derived classes
	// Intended usage example:
	//    Log::Debug().log("Example debug message with a value: {:.3f}", 1.0f);
	//    Log::Info().log("Log 1").log("Log 2");
	//    Log::Info(1).log("This message is indented 1 level");
	class LOGGER_EXPORT LoggerBase
	{
	public:
		LoggerBase(int indentaiton, Level level, const std::source_location& location);
		virtual ~LoggerBase();

		template<class... Args>
		LoggerBase& log(std::format_string<Args...> fmt, Args&&... args)
		{
			std::stringstream stream;
			std::print(stream, fmt, std::forward<Args>(args)...);
			logInternal(stream.view());
			return *this;
		}

	private:
		void logInternal(std::string_view message);

	private:
		Level m_level;
		std::source_location m_location;
		int m_indentation;
	};

	// Create a debug log
	// Example usage:
	//    Log::Debug().log("Example debug log! Value: {:.3f}", 1.0f);
	//    Log::Debug().log("Log 1").log("Log 2");
	//    Log::Debug(1).log("This message is indented 1 level");
	class LOGGER_EXPORT Debug : public LoggerBase
	{
	public:
		Debug(int indentation = 0, const std::source_location& location = std::source_location::current()) : LoggerBase(indentation, Level::Debug, location) {};
	};
	// Create an info log
	// Example usage:
	//    Log::Info().log("Example info log! Value: {:.3f}", 1.0f);
	//    Log::Info().log("Log 1").log("Log 2");
	//    Log::Info(1).log("This message is indented 1 level");
	class LOGGER_EXPORT Info : public LoggerBase
	{
	public:
		Info(int indentation = 0, const std::source_location& location = std::source_location::current()) : LoggerBase(indentation, Level::Info, location) {};
	};
	// Create a warning log
	// Example usage:
	//    Log::Warn().log("Example warning log! Value: {:.3f}", 1.0f);
	//    Log::Warn().log("Log 1").log("Log 2");
	//    Log::Warn(1).log("This message is indented 1 level");
	class LOGGER_EXPORT Warn : public LoggerBase
	{
	public:
		Warn(int indentation = 0, const std::source_location& location = std::source_location::current()) : LoggerBase(indentation, Level::Warning, location) {};
	};
	// Create an error log
	// Example usage:
	//    Log::Error().log("Example error log! Value: {:.3f}", 1.0f);
	//    Log::Error().log("Log 1").log("Log 2");
	//    Log::Error(1).log("This message is indented 1 level");
	class LOGGER_EXPORT Error : public LoggerBase
	{
	public:
		Error(int indentation = 0, const std::source_location& location = std::source_location::current()) : LoggerBase(indentation, Level::Error, location) {};
	};
	// Create a critical log
	// Example usage:
	//    Log::Critical().log("Example critical log! Value: {:.3f}", 1.0f);
	//    Log::Critical().log("Log 1").log("Log 2");
	//    Log::Critical(1).log("This message is indented 1 level");
	class LOGGER_EXPORT Critical : public LoggerBase
	{
	public:
		Critical(int indentation = 0, const std::source_location& location = std::source_location::current()) : LoggerBase(indentation, Level::Critical, location) {};
	};
}