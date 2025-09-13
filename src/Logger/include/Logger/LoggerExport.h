#pragma once

#ifdef _WIN32
#ifdef LOGGER_LIB
#   define LOGGER_EXPORT __declspec(dllexport)
#else
#   define LOGGER_EXPORT __declspec(dllimport)
#endif // LOGGER_LIB
#else
#define LOGGER_EXPORT
#endif // _WIN32

