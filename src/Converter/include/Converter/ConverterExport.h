#pragma once

#ifdef _WIN32
#ifdef CONVERTER_LIB
#   define CONVERTER_EXPORT __declspec(dllexport)
#else
#   define CONVERTER_EXPORT __declspec(dllimport)
#endif // CONVERTER_LIB
#else
#define CONVERTER_EXPORT
#endif // _WIN32
