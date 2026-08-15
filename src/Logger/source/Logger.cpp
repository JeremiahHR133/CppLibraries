#include <Logger/Logger.h>

#include <algorithm>
#include <assert.h>
#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <mutex>
#include <vector>

namespace
{
class LogManager
{
public:
    LogManager();
    LogManager(const std::vector<Log::LogStream> &streams);
    ~LogManager();

    const std::vector<Log::LogStream> &getStreams() const;
    bool initialized() const;
    const std::chrono::steady_clock::time_point &getInitTime() const;

private:
    std::vector<Log::LogStream> m_streams;
    bool m_initialized;
    std::chrono::steady_clock::time_point m_initTime;
};
LogManager::LogManager() : m_initialized(false)
{
}
LogManager::LogManager(const std::vector<Log::LogStream> &streams) : m_initialized(true)
{
    m_initTime = std::chrono::steady_clock::now();
    m_streams = streams;
}
LogManager::~LogManager() = default;

const std::vector<Log::LogStream> &LogManager::getStreams() const
{
    return m_streams;
}
bool LogManager::initialized() const
{
    return m_initialized;
}

const std::chrono::steady_clock::time_point &LogManager::getInitTime() const
{
    return m_initTime;
}

LogManager g_logManager;
std::mutex g_logMutex;

const std::unordered_map<Log::Color, std::string> g_colorMap = {
    {Log::Color::reset, "\033[0m"},

    {Log::Color::regular_black, "\033[0;30m"},
    {Log::Color::regular_red, "\033[0;31m"},
    {Log::Color::regular_green, "\033[0;32m"},
    {Log::Color::regular_yellow, "\033[0;33m"},
    {Log::Color::regular_blue, "\033[0;34m"},
    {Log::Color::regular_purple, "\033[0;35m"},
    {Log::Color::regular_cyan, "\033[0;36m"},
    {Log::Color::regular_white, "\033[0;37m"},

    {Log::Color::bold_black, "\033[1;30m"},
    {Log::Color::bold_red, "\033[1;31m"},
    {Log::Color::bold_green, "\033[1;32m"},
    {Log::Color::bold_yellow, "\033[1;33m"},
    {Log::Color::bold_blue, "\033[1;34m"},
    {Log::Color::bold_purple, "\033[1;35m"},
    {Log::Color::bold_cyan, "\033[1;36m"},
    {Log::Color::bold_white, "\033[1;37m"},

    {Log::Color::underline_black, "\033[4;30m"},
    {Log::Color::underline_red, "\033[4;31m"},
    {Log::Color::underline_green, "\033[4;32m"},
    {Log::Color::underline_yellow, "\033[4;33m"},
    {Log::Color::underline_blue, "\033[4;34m"},
    {Log::Color::underline_purple, "\033[4;35m"},
    {Log::Color::underline_cyan, "\033[4;36m"},
    {Log::Color::underline_white, "\033[4;37m"},

    {Log::Color::background_black, "\033[40m"},
    {Log::Color::background_red, "\033[41m"},
    {Log::Color::background_green, "\033[42m"},
    {Log::Color::background_yellow, "\033[43m"},
    {Log::Color::background_blue, "\033[44m"},
    {Log::Color::background_purple, "\033[45m"},
    {Log::Color::background_cyan, "\033[46m"},
    {Log::Color::background_white, "\033[47m"},

    {Log::Color::highIntensity_black, "\033[0;90m"},
    {Log::Color::highIntensity_red, "\033[0;91m"},
    {Log::Color::highIntensity_green, "\033[0;92m"},
    {Log::Color::highIntensity_yellow, "\033[0;93m"},
    {Log::Color::highIntensity_blue, "\033[0;94m"},
    {Log::Color::highIntensity_purple, "\033[0;95m"},
    {Log::Color::highIntensity_cyan, "\033[0;96m"},
    {Log::Color::highIntensity_white, "\033[0;97m"},

    {Log::Color::boldHighIntensity_black, "\033[1;90m"},
    {Log::Color::boldHighIntensity_red, "\033[1;91m"},
    {Log::Color::boldHighIntensity_green, "\033[1;92m"},
    {Log::Color::boldHighIntensity_yellow, "\033[1;93m"},
    {Log::Color::boldHighIntensity_blue, "\033[1;94m"},
    {Log::Color::boldHighIntensity_purple, "\033[1;95m"},
    {Log::Color::boldHighIntensity_cyan, "\033[1;96m"},
    {Log::Color::boldHighIntensity_white, "\033[1;97m"},

    {Log::Color::backgroundHighIntensity_black, "\033[0;100m"},
    {Log::Color::backgroundHighIntensity_red, "\033[0;101m"},
    {Log::Color::backgroundHighIntensity_green, "\033[0;102m"},
    {Log::Color::backgroundHighIntensity_yellow, "\033[0;103m"},
    {Log::Color::backgroundHighIntensity_blue, "\033[0;104m"},
    {Log::Color::backgroundHighIntensity_purple, "\033[0;105m"},
    {Log::Color::backgroundHighIntensity_cyan, "\033[0;106m"},
    {Log::Color::backgroundHighIntensity_white, "\033[0;107m"},
};

void internalInitLogging(const std::vector<Log::LogStream> &streams)
{
    if (g_logManager.initialized())
    {
        Log::Warn().log("Log already initialized; Ignoring additional call to "
                        "initLogging!");
    }
    else
    {
        g_logManager = LogManager(streams);
        Log::Debug().log("Logging initialized!");
    }
}
} // namespace

namespace Log
{
const std::unordered_map<Color, std::string> &getColorMap()
{
    return g_colorMap;
}

const std::string &getColorStr(Color color)
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

Color getColorForLevel(Level level, const Log::LogStreamOptions &opts)
{
    switch (level)
    {
    case Level::Debug:
        return opts.colorSettings.debug;
        break;
    case Level::Info:
        return opts.colorSettings.info;
        break;
    case Level::Warning:
        return opts.colorSettings.warn;
        break;
    case Level::Error:
        return opts.colorSettings.error;
        break;
    case Level::Critical:
        return opts.colorSettings.critical;
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

LoggerBase::LoggerBase(int indentaion, Level level, const std::source_location &location)
    : m_level(level), m_location(location), m_indentation(indentaion)
{
}

LoggerBase::~LoggerBase() = default;

void LoggerBase::logInternal(std::string_view message)
{
    for (auto logStream : g_logManager.getStreams())
    {
        if (!logStream.stream)
        {
            assert(false && "Stream is nullptr! Was Log::initLogging called? Was the stream resource freed?");
            continue;
        }

        if (std::ranges::find(logStream.options.levelFilter, m_level) == logStream.options.levelFilter.end())
            continue;

        std::ostringstream tmpBuff;

        if (logStream.options.timeMode != LogStreamOptions::TimeMode::None)
        {
            if (logStream.options.printColor)
                tmpBuff << getColorStr(logStream.options.colorSettings.timeInfo);

            tmpBuff << "[";

            if (logStream.options.timeMode == LogStreamOptions::TimeMode::Absolute)
            {
                auto now = std::chrono::system_clock::now();
                std::chrono::zoned_time localTime{std::chrono::current_zone(), now};
                // I get an intellisense error on this line lol
                tmpBuff << std::format("{:%F %T}", localTime);
            }
            else if (logStream.options.timeMode == LogStreamOptions::TimeMode::Relative)
            {
                auto ellapsedTime = std::chrono::steady_clock::now() - g_logManager.getInitTime();
                tmpBuff << std::format("{:%T}", ellapsedTime);
            }

            tmpBuff << "]";

            if (logStream.options.printColor)
                tmpBuff << getColorStr(Color::reset);

            tmpBuff << " ";
        }

        if (logStream.options.printColor)
            tmpBuff << getColorStr(getColorForLevel(m_level, logStream.options));

        tmpBuff << "[" << getStringForLevel(m_level) << "]";

        for (int i = 0; i < m_indentation; i++)
        {
            tmpBuff << logStream.options.indentationStep;
        }

        if (logStream.options.printColor)
            tmpBuff << getColorStr(Color::reset);

        tmpBuff << " " << message;

        if (logStream.options.printLocationInfo)
        {
            if (logStream.options.printColor)
                tmpBuff << getColorStr(logStream.options.colorSettings.functionInfo);

            tmpBuff << " --- ";
            if (logStream.options.logFullFunctionName)
                tmpBuff << m_location.function_name();
            else
                tmpBuff << getSimpleFunctionName(m_location.function_name());

            tmpBuff << " (";

            if (logStream.options.logFullFilePath)
                tmpBuff << m_location.file_name();
            else
                tmpBuff << std::filesystem::path(m_location.file_name())
                               .filename()
                               .string(); // .string() here to remove quotes from path

            tmpBuff << ":" << m_location.line() << "," << m_location.column() << ")";

            if (logStream.options.printColor)
                tmpBuff << getColorStr(Color::reset);
        }

        tmpBuff << "\n";

        {
            std::lock_guard<std::mutex> guard(g_logMutex);
            *logStream.stream << tmpBuff.view();
        }
    }
}

std::vector<LogStream> getDefaultStdLogStreams()
{
    return {
        {.stream = &std::cout, .options = {.levelFilter = {Level::Info, Level::Warning}}},
        {.stream = &std::cerr, .options = {.levelFilter = {Level::Error, Level::Critical}}},
    };
}

void initLogging(const std::vector<LogStream> &streams)
{
    internalInitLogging(streams);
}
} // namespace Log
