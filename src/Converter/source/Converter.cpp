#include <Converter/Converter.h>

#include <mutex>

namespace
{
// Delay the initialization of converters until the initialize function is
// called
std::mutex g_delayConvertersMutex;
std::vector<Converter::ConverterInfo> g_delayConverters;
std::vector<Converter::ConverterInfo> g_converters;
bool g_convertersRegistered = false;
} // namespace

namespace Converter
{

namespace Impl
{
void addConverter(const ConverterInfo &info)
{
    std::lock_guard<std::mutex> lock(g_delayConvertersMutex);
    g_delayConverters.push_back(info);
}

const std::vector<ConverterInfo> &getRegisteredConverters()
{
    return g_converters;
}

std::string getStrUsingConverter(const ConverterInfo &converter, const std::any &val)
{
    try
    {
        return converter.toStr(val);
    }
    catch (const std::bad_any_cast &e)
    {
        Log::Error().log("Unable to convert type to string; toStr failed! "
                         "Attempted to use converter with name: {}",
                         converter.name);
        assert(false && "Caught bad any cast!");
    }

    return "";
}

std::any getAnyUsingConverter(const ConverterInfo &converter, const std::string &val)
{
    try
    {
        return converter.fromStr(val);
    }
    catch (const std::bad_any_cast &e)
    {
        Log::Error().log("Unable to convert string to type; fromStr failed! "
                         "Attempted to use converter with name: {}",
                         converter.name);
        assert(false && "Caught bad any cast!");
    }

    return std::any();
}

const ConverterInfo *findConverter(const std::string &name)
{
    auto findConverter = std::find_if(Impl::getRegisteredConverters().begin(), Impl::getRegisteredConverters().end(),
                                      [&name](const ConverterInfo &comp) { return name == comp.name; });
    if (findConverter == Impl::getRegisteredConverters().end())
    {
        Log::Error().log("Unable to convert from string! Converter not found! Name: {}!", name);
    }
    else
    {
        return &(*findConverter);
    }

    return nullptr;
}

const ConverterInfo *findConverter(const std::type_index &index)
{
    auto findConverter = std::find_if(Impl::getRegisteredConverters().begin(), Impl::getRegisteredConverters().end(),
                                      [&index](const ConverterInfo &comp) { return index == comp.index; });
    if (findConverter == Impl::getRegisteredConverters().end())
    {
        Log::Error().log("Unable to convert from string! Converter not found! Name: {}!", index.name());
    }
    else
    {
        return &(*findConverter);
    }

    return nullptr;
}
} // namespace Impl

std::string getStringFromAny(const std::type_index &index, const std::any &val)
{
    if (auto *converter = Impl::findConverter(index))
        return Impl::getStrUsingConverter(*converter, val);

    return "";
}

std::string getStringFromAny(const std::string &name, const std::any &val)
{
    if (auto *converter = Impl::findConverter(name))
        return Impl::getStrUsingConverter(*converter, val);

    return "";
}

void initializeConverters()
{
    if (g_convertersRegistered)
    {
        Log::Warn().log("initializeConverters has already been called!");
        return;
    }
    for (const auto &delayConverter : g_delayConverters)
    {
        auto findConverter =
            std::find_if(g_converters.begin(), g_converters.end(),
                         [&delayConverter](const ConverterInfo &comp) { return delayConverter.index == comp.index; });
        if (findConverter == g_converters.end())
        {
            g_converters.push_back(delayConverter);
            Log::Info().log("Successfully registered converter for type: {}", delayConverter.name);
        }
        else
        {
            // Should not happen as uniqueness is compile time enforced unless
            // namespace shennanigains are used
            Log::Warn().log("Converter already registered for type: {}", delayConverter.name);
        }
    }

    g_convertersRegistered = true;
}
} // namespace Converter

#define REGISTER_STANDARD_CONVERTER(type, fromStringFuncName)                                                          \
    REGISTER_CONVERTER_FOR_TYPE(                                                                                       \
        type, [](const type &v) { return std::to_string(v); },                                                         \
        [](const std::string &str) { return std::fromStringFuncName(str); })

REGISTER_STANDARD_CONVERTER(int, stoi)
REGISTER_STANDARD_CONVERTER(long, stol)
REGISTER_STANDARD_CONVERTER(long long, stoll)
REGISTER_STANDARD_CONVERTER(unsigned int, stoul)
REGISTER_STANDARD_CONVERTER(unsigned long, stoul)
REGISTER_STANDARD_CONVERTER(unsigned long long, stoull)
REGISTER_STANDARD_CONVERTER(float, stof)
REGISTER_STANDARD_CONVERTER(double, stod)
REGISTER_STANDARD_CONVERTER(long double, stold)

REGISTER_CONVERTER_FOR_TYPE(
    bool, [](const bool &b) { return b ? "true" : "false"; },
    [](const std::string &str) { return str == "true" ? true : false; })
REGISTER_CONVERTER_FOR_TYPE(
    char, [](const char &c) { return std::string(1, c); },
    [](const std::string &str) -> char { return str.size() ? str[0] : 0; })
REGISTER_CONVERTER_FOR_TYPE(
    std::string, [](const std::string &str) { return str; }, [](const std::string &str) { return str; })
