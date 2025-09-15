#include <Logger/Logger.h>

#include <assert.h>
#include <mutex>
#include <chrono>
#include <format>

namespace
{
	class LogManager
	{
	public:
		LogManager();
		LogManager(std::ostream& primaryStream, std::ostream& errorStream, const Log::LogInitOptions& opts);
		~LogManager();

		bool initialized() const;
		std::ostream* primaryStream() const;
		std::ostream* errorStream() const;
		const Log::LogInitOptions& getOpts() const;
		const std::chrono::steady_clock::time_point& getInitTime() const;

	private:
		std::ostream* m_primaryStream;
		std::ostream* m_errorStream;
		bool m_initialized;
		Log::LogInitOptions m_opts;
		std::chrono::steady_clock::time_point m_initTime;
	};
	LogManager::LogManager()
		: m_primaryStream(nullptr)
		, m_errorStream(nullptr)
		, m_initialized(false)
	{
	}
	LogManager::LogManager(std::ostream& primaryStream, std::ostream& errorStream, const Log::LogInitOptions& opts)
		: m_primaryStream(&primaryStream)
		, m_errorStream(&errorStream)
		, m_initialized(true)
		, m_opts(opts)
	{
		m_initTime = std::chrono::steady_clock::now();
	}
	LogManager::~LogManager() = default;

	bool LogManager::initialized() const
	{
		return m_initialized;
	}
	std::ostream* LogManager::primaryStream() const
	{
		return m_primaryStream;
	}
	std::ostream* LogManager::errorStream() const
	{
		return m_errorStream;
	}

	const Log::LogInitOptions& LogManager::getOpts() const
	{
		return m_opts;
	}

	const std::chrono::steady_clock::time_point& LogManager::getInitTime() const
	{
		return m_initTime;
	}

	LogManager g_logManager;
	std::mutex g_logMutex;

	const std::unordered_map<Log::Color, std::string> g_colorMap =
	{
		{Log::Color::reset, "\033[0m"},

		{Log::Color::regular_black,  "\033[0;30m"},
		{Log::Color::regular_red,    "\033[0;31m"},
		{Log::Color::regular_green,  "\033[0;32m"},
		{Log::Color::regular_yellow, "\033[0;33m"},
		{Log::Color::regular_blue,   "\033[0;34m"},
		{Log::Color::regular_purple, "\033[0;35m"},
		{Log::Color::regular_cyan,   "\033[0;36m"},
		{Log::Color::regular_white,  "\033[0;37m"},

		{Log::Color::bold_black,  "\033[1;30m"},
		{Log::Color::bold_red,    "\033[1;31m"},
		{Log::Color::bold_green,  "\033[1;32m"},
		{Log::Color::bold_yellow, "\033[1;33m"},
		{Log::Color::bold_blue,   "\033[1;34m"},
		{Log::Color::bold_purple, "\033[1;35m"},
		{Log::Color::bold_cyan,   "\033[1;36m"},
		{Log::Color::bold_white,  "\033[1;37m"},

		{Log::Color::underline_black,  "\033[4;30m"},
		{Log::Color::underline_red,    "\033[4;31m"},
		{Log::Color::underline_green,  "\033[4;32m"},
		{Log::Color::underline_yellow, "\033[4;33m"},
		{Log::Color::underline_blue,   "\033[4;34m"},
		{Log::Color::underline_purple, "\033[4;35m"},
		{Log::Color::underline_cyan,   "\033[4;36m"},
		{Log::Color::underline_white,  "\033[4;37m"},

		{Log::Color::background_black,  "\033[40m"},
		{Log::Color::background_red,    "\033[41m"},
		{Log::Color::background_green,  "\033[42m"},
		{Log::Color::background_yellow, "\033[43m"},
		{Log::Color::background_blue,   "\033[44m"},
		{Log::Color::background_purple, "\033[45m"},
		{Log::Color::background_cyan,   "\033[46m"},
		{Log::Color::background_white,  "\033[47m"},

		{Log::Color::highIntensity_black,  "\033[0;90m"},
		{Log::Color::highIntensity_red,    "\033[0;91m"},
		{Log::Color::highIntensity_green,  "\033[0;92m"},
		{Log::Color::highIntensity_yellow, "\033[0;93m"},
		{Log::Color::highIntensity_blue,   "\033[0;94m"},
		{Log::Color::highIntensity_purple, "\033[0;95m"},
		{Log::Color::highIntensity_cyan,   "\033[0;96m"},
		{Log::Color::highIntensity_white,  "\033[0;97m"},

		{Log::Color::boldHighIntensity_black,  "\033[1;90m"},
		{Log::Color::boldHighIntensity_red,    "\033[1;91m"},
		{Log::Color::boldHighIntensity_green,  "\033[1;92m"},
		{Log::Color::boldHighIntensity_yellow, "\033[1;93m"},
		{Log::Color::boldHighIntensity_blue,   "\033[1;94m"},
		{Log::Color::boldHighIntensity_purple, "\033[1;95m"},
		{Log::Color::boldHighIntensity_cyan,   "\033[1;96m"},
		{Log::Color::boldHighIntensity_white,  "\033[1;97m"},

		{Log::Color::backgroundHighIntensity_black,  "\033[0;100m"},
		{Log::Color::backgroundHighIntensity_red,    "\033[0;101m"},
		{Log::Color::backgroundHighIntensity_green,  "\033[0;102m"},
		{Log::Color::backgroundHighIntensity_yellow, "\033[0;103m"},
		{Log::Color::backgroundHighIntensity_blue,   "\033[0;104m"},
		{Log::Color::backgroundHighIntensity_purple, "\033[0;105m"},
		{Log::Color::backgroundHighIntensity_cyan,   "\033[0;106m"},
		{Log::Color::backgroundHighIntensity_white,  "\033[0;107m"},
	};

	void internalInitLogging(std::ostream& primary, std::ostream& error, const Log::LogInitOptions& opts)
	{
		if (g_logManager.initialized())
		{
			Log::Warn().log("Log already initialized; Ignoring additional call to initLogging!");
		}
		else
		{
			g_logManager = LogManager(primary, error, opts);

			if (g_logManager.getOpts().reportLogInitialized)
			{
				Log::Info().log("Logging initialized!");
			}
		}
	}
}

namespace Log
{
	const std::unordered_map<Color, std::string>& getColorMap()
	{
		return g_colorMap;
	}

	const std::string& getColorStr(Color color)
	{
		return g_colorMap.at(color);
	}

	std::string getStringForLevel(Level level)
	{
		switch (level)
		{
		case Level::Debug:
			return "Debug";
			break;
		case Level::Info:
			return "Info ";
			break;
		case Level::Warning:
			return "Warn ";
			break;
		case Level::Error:
			return "Error";
			break;
		case Level::Critical:
			return "CRITICAL";
			break;
		default:
			break;
		}

		return "";
	}

	Color getColorForLevel(Level level)
	{
		switch (level)
		{
		case Level::Debug:
			return g_logManager.getOpts().colorSettings.debug;
			break;
		case Level::Info:
			return g_logManager.getOpts().colorSettings.info;
			break;
		case Level::Warning:
			return g_logManager.getOpts().colorSettings.warn;
			break;
		case Level::Error:
			return g_logManager.getOpts().colorSettings.error;
			break;
		case Level::Critical:
			return g_logManager.getOpts().colorSettings.critical;
			break;
		default:
			break;
		}

		return Color::reset;
	}

	std::string getSimpleFunctionName(std::string_view name)
	{
		size_t paramStart = name.find_first_of('(');
		size_t nameStart = name.rfind(' ', paramStart);
		if (paramStart != std::string::npos && nameStart != std::string::npos && nameStart < paramStart && name.size() > 1)
		{
			return std::string(name.substr(nameStart + 1, paramStart - nameStart - 1));
		}

		return "";
	}

	LoggerBase::LoggerBase(int indentaion, Level level, const std::source_location& location)
		: m_level(level)
		, m_location(location)
		, m_indentation(indentaion)
	{
	}

	LoggerBase::~LoggerBase() = default;

	void LoggerBase::logInternal(std::string_view message)
	{
		std::ostream* stdStreamPtr = g_logManager.primaryStream();
		std::ostream* errStreamPtr = g_logManager.errorStream();
		if (!stdStreamPtr || !errStreamPtr)
		{
			assert(false && "Stream is nullptr! Was Log::initLogging called?");
			return;
		}

		std::ostringstream tmpBuff;

		if (g_logManager.getOpts().timeMode != LogInitOptions::TimeMode::None)
		{
			if (g_logManager.getOpts().printColor)
				tmpBuff << getColorStr(g_logManager.getOpts().colorSettings.timeInfo);

			tmpBuff << "[";

			if (g_logManager.getOpts().timeMode == LogInitOptions::TimeMode::Absolute)
			{
				auto now = std::chrono::system_clock::now();
				std::chrono::zoned_time localTime{std::chrono::current_zone(), now};
				// I get an intellisense error on this line lol
				tmpBuff << std::format("{:%F %T}", localTime);
			}
			else if (g_logManager.getOpts().timeMode == LogInitOptions::TimeMode::Relative)
			{
				auto ellapsedTime = std::chrono::steady_clock::now() - g_logManager.getInitTime();
				tmpBuff << std::format("{:%T}", ellapsedTime);
			}

			tmpBuff << "]";

			if (g_logManager.getOpts().printColor)
				tmpBuff << getColorStr(Color::reset);

			tmpBuff << " ";
		}

		if (g_logManager.getOpts().printColor)
			tmpBuff << getColorStr(getColorForLevel(m_level));

		tmpBuff << "[" << getStringForLevel(m_level) << "]";

		for (int i = 0; i < m_indentation; i++)
		{
			tmpBuff << g_logManager.getOpts().indentationLevel;
		}

		if (g_logManager.getOpts().printColor)
			tmpBuff << getColorStr(Color::reset);

		tmpBuff << " " << message;

		if (g_logManager.getOpts().printLocationInfo)
		{
			if (g_logManager.getOpts().printColor)
				tmpBuff << getColorStr(g_logManager.getOpts().colorSettings.functionInfo);

			tmpBuff << " --- ";
			if (g_logManager.getOpts().logFullFunctionName)
				tmpBuff << m_location.function_name();
			else
				tmpBuff << getSimpleFunctionName(m_location.function_name());
			tmpBuff
				<< " ("
				<< m_location.file_name() << ":"
				<< m_location.line() << ","
				<< m_location.column() << ")"
				;

			if (g_logManager.getOpts().printColor)
				tmpBuff << getColorStr(Color::reset);
		}

		tmpBuff << "\n";

		std::ostream& stream = (m_level == Level::Error || m_level == Level::Critical)
			? *errStreamPtr
			: *stdStreamPtr;
		{
			std::lock_guard<std::mutex> guard(g_logMutex);
			stream << tmpBuff.view();
		}
	}

	void initLogging(std::ostream& stream, const LogInitOptions& opts)
	{
		internalInitLogging(stream, stream, opts);
	}

	void initLogging(std::ostream& primaryStream, std::ostream& errorStream, const LogInitOptions& opts)
	{
		internalInitLogging(primaryStream, errorStream, opts);
	}
}