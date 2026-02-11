#include "logging.hpp"

#include <stdio.h>
#include <string.h>

namespace LogImpl {
    static int g_indentLevel = 0;

    void Indent() {
        ++g_indentLevel;
    }

    void Unindent() {
        if (g_indentLevel > 0)
            --g_indentLevel;
    }

    int GetIndent() {
        return g_indentLevel;
    }

    const char *GetFilename(const char *path) {
        const char* slash = strrchr(path, '/');
        const char* backslash = strrchr(path, '\\');
        const char* separator = slash > backslash ? slash : backslash;
        return separator ? separator + 1 : path;
    }
}