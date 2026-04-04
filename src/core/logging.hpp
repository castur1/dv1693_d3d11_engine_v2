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
        printf("\033[33m[WARNING]\033[0m %s:%d in %s(): " format, LogImpl::GetFilename(__FILE__), __LINE__, __func__, ##__VA_ARGS__)

#define LogError(format, ...) \
    do { \
        printf("\033[31m[ERROR]\033[0m %s:%d in %s(): " format, LogImpl::GetFilename(__FILE__), __LINE__, __func__, ##__VA_ARGS__); \
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
        char buffer[512]; \
        sprintf_s(buffer, "[ERROR] %s:%d in %s(): " format, LogImpl::GetFilename(__FILE__), __LINE__, __func__, ##__VA_ARGS__); \
        wchar_t wideBuffer[512]; \
        size_t dummy = 0; \
        mbstowcs_s(&dummy, wideBuffer, buffer, _TRUNCATE); \
        MessageBox(NULL, wideBuffer, L"Fatal error", MB_OK | MB_ICONERROR); \
    } while (0)

#define LogIndent() (void)0
#define LogUnindent() (void)0

#endif // _DEBUG

#endif