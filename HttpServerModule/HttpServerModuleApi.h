// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
#if defined(HTTP_SERVER_MODULE_EXPORTS)
#define HTTP_SERVER_MODULE_API __declspec(dllexport)
#else
#define HTTP_SERVER_MODULE_API __declspec(dllimport)
#endif
#else
#define HTTP_SERVER_MODULE_API
#endif
