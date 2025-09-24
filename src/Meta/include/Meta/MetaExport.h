#pragma once

#ifdef _WIN32
#ifdef META_LIB
#   define META_EXPORT __declspec(dllexport)
#else
#   define META_EXPORT __declspec(dllimport)
#endif // META_LIB
#else
#define META_EXPORT
#endif // _WIN32

