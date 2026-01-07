#include "shaderc.h"

#include <bx/file.h>
#include <bx/string.h>

#include <cstdarg>

namespace bgfx {

bool g_verbose = false;

Options::Options()
    : shaderType(' ')
    , disasm(false)
    , raw(false)
    , preprocessOnly(false)
    , depends(false)
    , debugInformation(false)
    , avoidFlowControl(false)
    , noPreshader(false)
    , partialPrecision(false)
    , preferFlowControl(false)
    , backwardsCompatibility(false)
    , warningsAreErrors(false)
    , keepIntermediate(false)
    , optimize(false)
    , optimizationLevel(3) {
}

void Options::dump() {
    BX_TRACE("Options:\n"
             "\t  shaderType: %c\n"
             "\t  platform: %s\n"
             "\t  profile: %s\n"
             "\t  inputFile: %s\n"
             "\t  outputFile: %s\n"
             "\t  disasm: %s\n"
             "\t  raw: %s\n"
             "\t  preprocessOnly: %s\n"
             "\t  depends: %s\n"
             "\t  debugInformation: %s\n"
             "\t  avoidFlowControl: %s\n"
             "\t  noPreshader: %s\n"
             "\t  partialPrecision: %s\n"
             "\t  preferFlowControl: %s\n"
             "\t  backwardsCompatibility: %s\n"
             "\t  warningsAreErrors: %s\n"
             "\t  keepIntermediate: %s\n"
             "\t  optimize: %s\n"
             "\t  optimizationLevel: %d\n",
             shaderType,
             platform.c_str(),
             profile.c_str(),
             inputFilePath.c_str(),
             outputFilePath.c_str(),
             disasm ? "true" : "false",
             raw ? "true" : "false",
             preprocessOnly ? "true" : "false",
             depends ? "true" : "false",
             debugInformation ? "true" : "false",
             avoidFlowControl ? "true" : "false",
             noPreshader ? "true" : "false",
             partialPrecision ? "true" : "false",
             preferFlowControl ? "true" : "false",
             backwardsCompatibility ? "true" : "false",
             warningsAreErrors ? "true" : "false",
             keepIntermediate ? "true" : "false",
             optimize ? "true" : "false",
             optimizationLevel);

    for (size_t ii = 0; ii < includeDirs.size(); ++ii) {
        BX_TRACE("\t  include :%s\n", includeDirs[ii].c_str());
    }

    for (size_t ii = 0; ii < defines.size(); ++ii) {
        BX_TRACE("\t  define :%s\n", defines[ii].c_str());
    }

    for (size_t ii = 0; ii < dependencies.size(); ++ii) {
        BX_TRACE("\t  dependency :%s\n", dependencies[ii].c_str());
    }
}

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
