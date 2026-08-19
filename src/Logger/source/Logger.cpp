#include <Logger/Logger.h>

#include <assert.h>
#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
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

// Stored as foreground code, for anything other than reset, adding 60 gets the background code
const std::unordered_map<Log::TextProp::Color, int> g_colorMap = {
    {Log::TextProp::Color::reset, 0},

    {Log::TextProp::Color::black, 30},        {Log::TextProp::Color::red, 31},
    {Log::TextProp::Color::green, 32},        {Log::TextProp::Color::yellow, 33},
    {Log::TextProp::Color::blue, 34},         {Log::TextProp::Color::magenta, 35},
    {Log::TextProp::Color::cyan, 36},         {Log::TextProp::Color::white, 37},

    {Log::TextProp::Color::bright_black, 90}, {Log::TextProp::Color::bright_red, 91},
    {Log::TextProp::Color::bright_green, 92}, {Log::TextProp::Color::bright_yellow, 93},
    {Log::TextProp::Color::bright_blue, 94},  {Log::TextProp::Color::bright_magenta, 95},
    {Log::TextProp::Color::bright_cyan, 96},  {Log::TextProp::Color::bright_white, 97},
};
} // namespace

namespace Log
{

std::string getTextPropStr(TextProp prop)
{
    using T = std::underlying_type_t<TextProp::Effect>;

    std::string ret = "\033[";

    if (static_cast<T>(prop.effect & TextProp::Effect::reset))
        ret.append("0;");
    else
    {
        if (static_cast<T>(prop.effect & TextProp::Effect::bold))
            ret.append("1;");
        if (static_cast<T>(prop.effect & TextProp::Effect::faint))
            ret.append("2;");
        if (static_cast<T>(prop.effect & TextProp::Effect::italic))
            ret.append("3;");
        if (static_cast<T>(prop.effect & TextProp::Effect::underline))
            ret.append("4;");
        if (static_cast<T>(prop.effect & TextProp::Effect::crossed_out))
            ret.append("9;");
        if (static_cast<T>(prop.effect & TextProp::Effect::overline))
            ret.append("53;");
    }

    if (prop.foregroundColor != TextProp::Color::reset)
        ret.append(std::to_string(g_colorMap.at(prop.foregroundColor)));

    if (prop.backgroundColor != TextProp::Color::reset)
    {
        ret.append(";");
        ret.append(std::to_string(g_colorMap.at(prop.backgroundColor) + 60)).append("m");
    }
    else
        ret.append("m");

    return ret;
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

TextProp getTextPropForLevel(Level level, const Log::LogStreamOptions &opts)
{
    switch (level)
    {
    case Level::Debug:
        return opts.textSettings.debug;
        break;
    case Level::Info:
        return opts.textSettings.info;
        break;
    case Level::Warning:
        return opts.textSettings.warn;
        break;
    case Level::Error:
        return opts.textSettings.error;
        break;
    case Level::Critical:
        return opts.textSettings.critical;
        break;
    default:
        break;
    }

    // The default constructor is all resets
    return TextProp{};
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
    if (!g_logManager.initialized())
        return;

    for (auto logStream : g_logManager.getStreams())
    {
        if (!logStream.stream)
        {
            assert(false && "Stream is nullptr! Was the stream resource released?");
            continue;
        }

        if (!static_cast<std::underlying_type_t<Level>>(logStream.options.levelFilter & m_level))
            continue;

        std::ostringstream tmpBuff;

        if (logStream.options.timeMode != LogStreamOptions::TimeMode::None)
        {
            if (logStream.options.printColor)
                tmpBuff << getTextPropStr(logStream.options.textSettings.timeInfo);

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
                tmpBuff << getTextPropStr(TextProp{});

            tmpBuff << " ";
        }

        if (logStream.options.printColor)
            tmpBuff << getTextPropStr(getTextPropForLevel(m_level, logStream.options));

        tmpBuff << "[" << getStringForLevel(m_level) << "]";

        for (int i = 0; i < m_indentation; i++)
        {
            tmpBuff << logStream.options.indentationStep;
        }

        if (logStream.options.printColor)
            tmpBuff << getTextPropStr(TextProp{});

        tmpBuff << " " << message;

        if (logStream.options.printLocationInfo)
        {
            if (logStream.options.printColor)
                tmpBuff << getTextPropStr(logStream.options.textSettings.functionInfo);

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
                tmpBuff << getTextPropStr(TextProp{});
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
        {.stream = &std::cout, .options = {.levelFilter = Level::Info | Level::Warning}},
        {.stream = &std::cerr, .options = {.levelFilter = Level::Error | Level::Critical}},
    };
}

void initLogging(const std::vector<LogStream> &streams)
{
    internalInitLogging(streams);
}
} // namespace Log
