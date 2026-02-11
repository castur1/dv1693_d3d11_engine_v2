#ifndef LOGGING_HPP
#define LOGGING_HPP

#include "stdio.h"

namespace LogImpl {
    static const int SPACES_PER_INDENT = 2;

    void Indent();
    void Unindent();
    int GetIndent();

    const char *GetFilename(const char *path);
}

#ifdef _DEBUG

#define LogInfo(format, ...) \
    do { \
        printf("[INFO] "); \
        if (LogImpl::GetIndent() > 0) printf("%*s> ", (LogImpl::GetIndent() - 1) * LogImpl::SPACES_PER_INDENT, ""); \
        printf(format, __VA_ARGS__); \
    } while (0)

#define LogWarn(format, ...) \
        printf("[WARNING] %s:%d in %s(): " format, LogImpl::GetFilename(__FILE__), __LINE__, __func__, ##__VA_ARGS__)

#define LogError(format, ...) \
    do { \
        printf("[ERROR] %s:%d in %s(): " format, LogImpl::GetFilename(__FILE__), __LINE__, __func__, ##__VA_ARGS__); \
        (void)getchar(); \
    } while (0)

#define LogIndent() LogImpl::Indent()
#define LogUnindent() LogImpl::Unindent()

#else // _DEBUG

#include "Windows.h"

#define LogInfo(format, ...) (void)0

#define LogWarn(format, ...) (void)0

#define LogError(format, ...) \
    do { \
        wchar_t buffer[128]; \
        swprintf_s(buffer, L"[ERROR] %hs:%d in %hs(): " L##format, LogImpl::GetFilename(__FILE__), __LINE__, __func__, ##__VA_ARGS__); \
        MessageBox(NULL, buffer, L"Fatal error", MB_OK | MB_ICONERROR); \
    } while (0)

#define LogIndent() (void)0
#define LogUnindent() (void)0

#endif // _DEBUG

#endif