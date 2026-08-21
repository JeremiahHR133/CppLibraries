#pragma once

#include <Logger/LoggerExport.h>

#include <ostream>
#include <print>
#include <source_location>
#include <sstream>
#include <string_view>
#include <vector>

// Simple logging library
// Logs to any std::ostream (provided at initialization)
namespace Log
{
// ANSI 4-bit colors
// Not all terminals support all options
struct TextProp
{
    enum class Color
    {
        reset,

        black,
        red,
        green,
        yellow,
        blue,
        magenta,
        cyan,
        white,

        bright_black,
        bright_red,
        bright_green,
        bright_yellow,
        bright_blue,
        bright_magenta,
        bright_cyan,
        bright_white,
    };
    enum class Effect
    {
        reset = 0,

        bold = 1 << 0,
        faint = 1 << 1,
        italic = 1 << 2,
        underline = 1 << 3,
        crossed_out = 1 << 4,
        overline = 1 << 5,
    };

    Color foregroundColor = Color::reset;
    Color backgroundColor = Color::reset;
    Effect effect = Effect::reset;
};

inline constexpr TextProp::Effect operator|(TextProp::Effect lhs, TextProp::Effect rhs)
{
    using T = std::underlying_type_t<TextProp::Effect>;
    return static_cast<TextProp::Effect>(static_cast<T>(lhs) | static_cast<T>(rhs));
}
inline constexpr TextProp::Effect operator&(TextProp::Effect lhs, TextProp::Effect rhs)
{
    using T = std::underlying_type_t<TextProp::Effect>;
    return static_cast<TextProp::Effect>(static_cast<T>(lhs) & static_cast<T>(rhs));
}
inline constexpr TextProp::Effect operator~(TextProp::Effect eff)
{
    using T = std::underlying_type_t<TextProp::Effect>;
    return static_cast<TextProp::Effect>(~static_cast<T>(eff));
}

// Logging level
// Allows a stream to distinguish which levels are enabled
enum class Level
{
    Debug = 0,
    Info = 1 << 0,
    Warning = 1 << 1,
    Error = 1 << 2,
    Critical = 1 << 3,
};
inline constexpr Level operator|(Level lhs, Level rhs)
{
    using T = std::underlying_type_t<Level>;
    return static_cast<Level>(static_cast<T>(lhs) | static_cast<T>(rhs));
}
inline constexpr Level operator&(Level lhs, Level rhs)
{
    using T = std::underlying_type_t<Level>;
    return static_cast<Level>(static_cast<T>(lhs) & static_cast<T>(rhs));
}
inline constexpr Level operator~(Level lvl)
{
    using T = std::underlying_type_t<Level>;
    return static_cast<Level>(~static_cast<T>(lvl));
}

// Options to control the behavior of the logger
// These are set for each stream
struct LogStreamOptions
{
    Level levelFilter = Level::Debug | Level::Info | Level::Warning | Level::Error | Level::Critical;
    bool printColor = true;
    bool printLocationInfo = true;
    bool logFullFunctionName = false;
    bool logFullFilePath = false;
    std::string indentationStep = "   ";

    struct TextSettings
    {
        TextProp debug = TextProp{.foregroundColor = TextProp::Color::bright_white,
                                  .backgroundColor = TextProp::Color::reset,
                                  .effect = TextProp::Effect::bold};
        TextProp info = TextProp{.foregroundColor = TextProp::Color::bright_green,
                                 .backgroundColor = TextProp::Color::reset,
                                 .effect = TextProp::Effect::bold};
        TextProp warn = TextProp{.foregroundColor = TextProp::Color::bright_yellow,
                                 .backgroundColor = TextProp::Color::reset,
                                 .effect = TextProp::Effect::bold};
        TextProp error = TextProp{.foregroundColor = TextProp::Color::bright_red,
                                  .backgroundColor = TextProp::Color::reset,
                                  .effect = TextProp::Effect::bold};
        TextProp critical =
            TextProp{.foregroundColor = TextProp::Color::bright_red,
                     .backgroundColor = TextProp::Color::reset,
                     .effect = TextProp::Effect::bold | TextProp::Effect::underline | TextProp::Effect::overline};

        TextProp functionInfo = TextProp{.foregroundColor = TextProp::Color::bright_black,
                                         .backgroundColor = TextProp::Color::reset,
                                         .effect = TextProp::Effect::bold};
        TextProp timeInfo = TextProp{.foregroundColor = TextProp::Color::bright_black,
                                     .backgroundColor = TextProp::Color::reset,
                                     .effect = TextProp::Effect::bold};
    } textSettings;

    enum class TimeMode
    {
        None,     // Don't print any timing information
        Relative, // Print the time since initLogging was called
        Absolute, // Print the current date time
    } timeMode = TimeMode::None;
};

struct LogStream
{
    std::ostream *stream;
    LogStreamOptions options;
};

// Returns a string holding the ANSI escape code for the text property
LOGGER_EXPORT std::string getTextPropStr(TextProp prop);
// Returns the string to be printed for a given level (e.g. "Debug", "Info", etc.)
LOGGER_EXPORT std::string getStringForLevel(Level level);
// Returns the TextProperty that the StringForLevel will be printed in
LOGGER_EXPORT TextProp getTextPropForLevel(Level level, const Log::LogStreamOptions &opts);
// Creates default stream loggers for cout and cerr with resonable defaults set for each
LOGGER_EXPORT std::vector<LogStream> getDefaultStdLogStreams();
// Initializes global logging
// Call this once at the start of a program
// All logs will be written to 'stream'
LOGGER_EXPORT void initLogging(const std::vector<LogStream> &streams = getDefaultStdLogStreams());
// Simplifies the complex semi-mangled function names
// Example:
//    void __cdecl Log::initLogging(class std::basic_ostream<char,struct
//    std::char_traits<char> > &,const struct Log::LogInitOptions &)
// becomes
//    Log::initLogging
LOGGER_EXPORT std::string getSimpleFunctionName(std::string_view name);

// Returns true if a message of a certain level
// will be logged to any streams
LOGGER_EXPORT bool willLogMessage(Level level);

// Base class for loggers
// Can be used directly, but is meant to be used via the derived classes
// Intended usage example:
//    Log::Debug().log("Example debug message with a value: {:.3f}", 1.0f);
//    Log::Info().log("Log 1").log("Log 2");
//    Log::Info(1).log("This message is indented 1 level");
class LOGGER_EXPORT LoggerBase
{
public:
    LoggerBase(int indentaiton, Level level, const std::source_location &location);
    virtual ~LoggerBase();

    template <class... Args> LoggerBase &log(std::format_string<Args...> fmt, Args &&...args)
    {
        if (!willLogMessage(m_level))
            return *this;

        std::ostringstream stream;
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
    Debug(int indentation = 0, const std::source_location &location = std::source_location::current())
        : LoggerBase(indentation, Level::Debug, location){};
};
// Create an info log
// Example usage:
//    Log::Info().log("Example info log! Value: {:.3f}", 1.0f);
//    Log::Info().log("Log 1").log("Log 2");
//    Log::Info(1).log("This message is indented 1 level");
class LOGGER_EXPORT Info : public LoggerBase
{
public:
    Info(int indentation = 0, const std::source_location &location = std::source_location::current())
        : LoggerBase(indentation, Level::Info, location){};
};
// Create a warning log
// Example usage:
//    Log::Warn().log("Example warning log! Value: {:.3f}", 1.0f);
//    Log::Warn().log("Log 1").log("Log 2");
//    Log::Warn(1).log("This message is indented 1 level");
class LOGGER_EXPORT Warn : public LoggerBase
{
public:
    Warn(int indentation = 0, const std::source_location &location = std::source_location::current())
        : LoggerBase(indentation, Level::Warning, location){};
};
// Create an error log
// Example usage:
//    Log::Error().log("Example error log! Value: {:.3f}", 1.0f);
//    Log::Error().log("Log 1").log("Log 2");
//    Log::Error(1).log("This message is indented 1 level");
class LOGGER_EXPORT Error : public LoggerBase
{
public:
    Error(int indentation = 0, const std::source_location &location = std::source_location::current())
        : LoggerBase(indentation, Level::Error, location){};
};
// Create a critical log
// Example usage:
//    Log::Critical().log("Example critical log! Value: {:.3f}", 1.0f);
//    Log::Critical().log("Log 1").log("Log 2");
//    Log::Critical(1).log("This message is indented 1 level");
class LOGGER_EXPORT Critical : public LoggerBase
{
public:
    Critical(int indentation = 0, const std::source_location &location = std::source_location::current())
        : LoggerBase(indentation, Level::Critical, location){};
};
} // namespace Log
