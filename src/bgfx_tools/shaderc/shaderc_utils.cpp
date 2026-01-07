#include "shaderc.h"

#include <bx/file.h>
#include <bx/string.h>

#include <cstdarg>

namespace bgfx {

bool g_verbose = false;

int32_t writef(bx::WriterI* _writer, const char* _format, ...) {
    va_list argList;
    va_start(argList, _format);

    char temp[2048];

    char* out = temp;
    int32_t max = sizeof(temp);
    int32_t len = bx::vsnprintf(out, max, _format, argList);
    if (len > max) {
        out = static_cast<char*>(BX_STACK_ALLOC(len));
        len = bx::vsnprintf(out, len, _format, argList);
    }

    len = bx::write(_writer, out, len, bx::ErrorAssert{});

    va_end(argList);

    return len;
}

void strReplace(char* _str, const char* _find, const char* _replace) {
    const int32_t len = bx::strLen(_find);

    char* replace = static_cast<char*>(BX_STACK_ALLOC(len + 1));
    bx::strCopy(replace, len + 1, _replace);
    for (int32_t ii = bx::strLen(replace); ii < len; ++ii) {
        replace[ii] = ' ';
    }
    replace[len] = '\0';

    BX_ASSERT(len >= bx::strLen(_replace), "");
    for (bx::StringView ptr = bx::strFind(_str, _find);
         !ptr.isEmpty();
         ptr = bx::strFind(ptr.getPtr() + len, _find)) {
        bx::memCopy(const_cast<char*>(ptr.getPtr()), replace, len);
    }
}

void printCode(const char* _code, int32_t _line, int32_t _start, int32_t _end, int32_t _column) {
    bx::printf("Code:\n---\n");

    bx::LineReader reader(_code);
    for (int32_t line = 1; !reader.isDone() && line < _end; ++line) {
        bx::StringView strLine = reader.next();

        if (line >= _start) {
            if (_line == line) {
                bx::printf("\n");
                bx::printf(">>> %3d: %.*s\n", line, strLine.getLength(), strLine.getPtr());
                if (-1 != _column) {
                    bx::printf(">>> %3d: %*s\n", _column, _column, "^");
                }
                bx::printf("\n");
            } else {
                bx::printf("    %3d: %.*s\n", line, strLine.getLength(), strLine.getPtr());
            }
        }
    }

    bx::printf("---\n");
}

void writeFile(const char* _filePath, const void* _data, int32_t _size) {
    bx::FileWriter out;
    if (bx::open(&out, _filePath)) {
        bx::write(&out, _data, _size, bx::ErrorAssert{});
        bx::close(&out);
    }
}

bx::StringView nextWord(bx::StringView& _parse) {
    bx::StringView word = bx::strWord(bx::strLTrimSpace(_parse));
    _parse = bx::strLTrimSpace(bx::StringView(word.getTerm(), _parse.getTerm()));
    return word;
}

} // namespace bgfx
