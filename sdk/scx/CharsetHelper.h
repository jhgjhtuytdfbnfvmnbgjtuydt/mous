#pragma once

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <string>
#include <vector>

namespace CharsetHelper {

static const wchar_t bad_wchar = 0xFFFD;

// depends on locale
inline std::string
WStringToMBStr(const std::wstring& str)
{
    using namespace std;
    const size_t buflen = (str.size() + 2) * MB_LEN_MAX;
    vector<char> buf(buflen);
    size_t ret = wcstombs(&buf[0], str.c_str(), buflen);
    return (ret != (size_t)-1) ? string(&buf[0], ret) : string("");
}

/*
 * COPYRIGHT: editors/nano
 * NOTE: call setlocale() before use this function
 * This function is equivalent to wcwidth() for multibyte characters.
 */
inline int
MBWidth(const char* c, bool useUtf8 = true)
{
    wchar_t wc;
    int width;

    if (useUtf8) {
        if (mbtowc(&wc, c, MB_CUR_MAX) < 0) {
            mbtowc(nullptr, nullptr, 0);
            wc = bad_wchar;
        }

        width = wcwidth(wc);

        if (width == -1) {
            wc = bad_wchar;
            width = wcwidth(wc);
        }

        return width;
    } else {
        return 1;
    }
}

inline int
MBStrWidth(const std::string& str)
{
    int width = 0;
    const char* const s = str.c_str();
    for (size_t i = 0; i < str.size();) {
        int len = mblen(s + i, MB_CUR_MAX);
        if (len < 0) {
            mblen(nullptr, 0);
            width += 1;
            i += 1;
        } else {
            width += MBWidth(s + i);
            i += len;
        }
    }
    return width;
}

inline int
MBStrLen(const std::string& str)
{
    return mbstowcs(nullptr, str.c_str(), str.size());
}

inline std::string
MBSubStr(const std::string& str, int n, int ignoreN = 0)
{
    const char* const s = str.c_str();
    size_t beg = 0;
    size_t i = 0;

    int nch[2] = { ignoreN, n };
    for (int idx = 0; idx < 2; ++idx) {
        beg = i;
        for (int c = 0; c < nch[idx] && i < str.size(); ++c) {
            int len = mblen(s + i, MB_CUR_MAX);
            if (len < 0) {
                mblen(nullptr, 0);
                i += 1;
            } else {
                i += len;
            }
        }
    }

    return std::string(s + beg, s + i);
}

// Another way is binary search algorithm
// (Of course should based on MBSubStr(), not str.size()/2 !)
inline std::string
MBWidthStr(const std::string& str, int width)
{
    const char* const s = str.c_str();
    size_t beg = 0;
    size_t i = 0;
    int hasWidth = 0;

    while (i < str.size() && hasWidth < width) {
        int len = mblen(s + i, MB_CUR_MAX);
        if (len < 0) {
            mblen(nullptr, 0);
            i += 1;
            hasWidth += 1;
        } else {
            i += len;
            hasWidth += MBWidth(s + i);
        }
    }

    return std::string(s + beg, s + i);
}

// COPYRIGHT: http://stackoverflow.com/questions/1031645/how-to-detect-utf-8-in-plain-c
inline bool
IsUtf8(const char* string)
{
    if (!string) {
        return false;
    }

    const unsigned char* bytes = (const unsigned char*)string;
    while (*bytes) {
        if (( // ASCII
              bytes[0] == 0x09 || bytes[0] == 0x0A || bytes[0] == 0x0D || (0x20 <= bytes[0] && bytes[0] <= 0x7E))) {
            bytes += 1;
            continue;
        }

        if (( // non-overlong 2-byte
              (0xC2 <= bytes[0] && bytes[0] <= 0xDF) && (0x80 <= bytes[1] && bytes[1] <= 0xBF))) {
            bytes += 2;
            continue;
        }

        if (( // excluding overlongs
              bytes[0] == 0xE0 && (0xA0 <= bytes[1] && bytes[1] <= 0xBF) && (0x80 <= bytes[2] && bytes[2] <= 0xBF)) ||
            ( // straight 3-byte
              ((0xE1 <= bytes[0] && bytes[0] <= 0xEC) || bytes[0] == 0xEE || bytes[0] == 0xEF) &&
              (0x80 <= bytes[1] && bytes[1] <= 0xBF) && (0x80 <= bytes[2] && bytes[2] <= 0xBF)) ||
            ( // excluding surrogates
              bytes[0] == 0xED && (0x80 <= bytes[1] && bytes[1] <= 0x9F) && (0x80 <= bytes[2] && bytes[2] <= 0xBF))) {
            bytes += 3;
            continue;
        }

        if (( // planes 1-3
              bytes[0] == 0xF0 && (0x90 <= bytes[1] && bytes[1] <= 0xBF) && (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
              (0x80 <= bytes[3] && bytes[3] <= 0xBF)) ||
            ( // planes 4-15
              (0xF1 <= bytes[0] && bytes[0] <= 0xF3) && (0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
              (0x80 <= bytes[2] && bytes[2] <= 0xBF) && (0x80 <= bytes[3] && bytes[3] <= 0xBF)) ||
            ( // plane 16
              bytes[0] == 0xF4 && (0x80 <= bytes[1] && bytes[1] <= 0x8F) && (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
              (0x80 <= bytes[3] && bytes[3] <= 0xBF))) {
            bytes += 4;
            continue;
        }

        return false;
    }

    return true;
}
}
