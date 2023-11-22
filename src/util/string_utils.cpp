// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "string_utils.h"
#include <string.h>
#include <algorithm>
#include <functional>
#include <clocale>
#include <cwchar>
#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#endif

namespace StringUtils {

struct LocaleModifier {
    LocaleModifier(const char* new_locale) {
        saved_locale = std::setlocale(LC_ALL, NULL);
        std::setlocale(LC_ALL, new_locale);
    }
    ~LocaleModifier() {
        std::setlocale(LC_ALL, saved_locale.c_str());
    }
    std::string saved_locale;
};

void Split(const std::string& str, const std::string& delimiters, std::vector<std::string>& output) {
    size_t start = 0;
    size_t pos = std::string::npos;
    while ((pos = str.find_first_of(delimiters, start)) != std::string::npos) {
        if (pos > start) {
            output.emplace_back(str, start, pos - start);
        } else {
            output.emplace_back("");
        }
        start = pos + 1;
    }
    if (start < str.size()) {
        output.emplace_back(str, start, str.size() - start);
    }
}

std::string Trim(const std::string& src) {
    std::string s = src;
    auto pred = std::bind(::isspace, std::placeholders::_1);
    s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), pred));
    s.erase(std::find_if_not(s.rbegin(), s.rend(), pred).base(), s.end());
    return s;
}

bool StartsWith(const std::string &src, const std::string &str) {
    return src.find(str) == 0;
}

bool EndsWith(const std::string &src, const std::string &str) {
    return src.rfind(str) == (src.length() - str.length());
}

bool StartWith(const std::string &src, const std::string &str) {
    return src.find_first_of(str) == 0;
}

bool EndWith(const std::string &src, const std::string &str) {
    return src.find_last_of(str) == (src.length() - 1);
}

std::string& Format(std::string& dst, const char *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    VFormat(dst, fmt, arg);
    va_end(arg);
    return dst;
}

std::string& VFormat(std::string &dst, const char *fmt, va_list arg) {
    va_list arg_dup;
    va_copy(arg_dup, arg);
    int len = vsnprintf(NULL, 0, fmt, arg_dup);
    va_end(arg_dup);
    if (len > 0) {
        size_t begin = dst.size();
        dst.reserve(begin + len + 1); //Make sure buffer space is len + 1;
        dst.resize(begin + len); //We don't need the tailing '\0'
        vsnprintf(&dst[begin], len + 1, fmt, arg);
    }
    return dst;
}

std::string& to_lower(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

std::string& to_upper(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

//BKDR Hash
unsigned int Hash(const char* str) {
    unsigned seed = 131;
    unsigned int hash = 0;
    while (*str) {
        hash = hash * seed + (*str++);
    }
    return (hash & 0x7FFFFFFF);
}

//djb2 by Dan Bernstein
unsigned int Hash2(const char *str) {
    unsigned  int hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

unsigned long long Hash3(const char* str) {
    uint32_t h = Hash(str);
    uint32_t l = Hash2(str);
    return (unsigned long long)h << 32 | l;
}

#ifdef _WIN32
struct CodePageReg {
    const char* name;
    unsigned int value;
    unsigned long long hash;
};
#define CODE_PAGE(NAME, VALUE) {#NAME, (VALUE), Hash3(#NAME)}
static CodePageReg code_pages[] = {
    CODE_PAGE(IBM037, 037),
    CODE_PAGE(IBM437, 437),
    CODE_PAGE(IBM500, 500),
    CODE_PAGE(ASMO - 708, 708),
    CODE_PAGE(DOS - 720, 720),
    CODE_PAGE(ibm737, 737),
    CODE_PAGE(ibm775, 775),
    CODE_PAGE(ibm850, 850),
    CODE_PAGE(ibm852, 852),
    CODE_PAGE(IBM855, 855),
    CODE_PAGE(ibm857, 857),
    CODE_PAGE(IBM00858, 858),
    CODE_PAGE(IBM860, 860),
    CODE_PAGE(ibm861, 861),
    CODE_PAGE(DOS - 862, 862),
    CODE_PAGE(IBM863, 863),
    CODE_PAGE(IBM864, 864),
    CODE_PAGE(IBM865, 865),
    CODE_PAGE(cp866, 866),
    CODE_PAGE(ibm869, 869),
    CODE_PAGE(IBM870, 870),
    CODE_PAGE(windows - 874, 874),
    CODE_PAGE(cp875, 875),
    CODE_PAGE(shift_jis, 932),
    CODE_PAGE(gb2312, 936),
    CODE_PAGE(ks_c_5601 - 1987, 949),
    CODE_PAGE(big5, 950),
    CODE_PAGE(IBM1026, 1026),
    CODE_PAGE(IBM01047, 1047),
    CODE_PAGE(IBM01140, 1140),
    CODE_PAGE(IBM01141, 1141),
    CODE_PAGE(IBM01142, 1142),
    CODE_PAGE(IBM01143, 1143),
    CODE_PAGE(IBM01144, 1144),
    CODE_PAGE(IBM01145, 1145),
    CODE_PAGE(IBM01146, 1146),
    CODE_PAGE(IBM01147, 1147),
    CODE_PAGE(IBM01148, 1148),
    CODE_PAGE(IBM01149, 1149),
    CODE_PAGE(utf-16, 1200),
    CODE_PAGE(unicodeFFFE, 1201),
    CODE_PAGE(windows-1250, 1250),
    CODE_PAGE(windows-1251, 1251),
    CODE_PAGE(windows-1252, 1252),
    CODE_PAGE(windows-1253, 1253),
    CODE_PAGE(windows-1254, 1254),
    CODE_PAGE(windows-1255, 1255),
    CODE_PAGE(windows-1256, 1256),
    CODE_PAGE(windows-1257, 1257),
    CODE_PAGE(windows-1258, 1258),
    CODE_PAGE(Johab, 1361),
    CODE_PAGE(macintosh, 10000),
    CODE_PAGE(x-mac-japanese, 10001),
    CODE_PAGE(x-mac-chinesetrad, 10002),
    CODE_PAGE(x-mac-korean, 10003),
    CODE_PAGE(x-mac-arabic, 10004),
    CODE_PAGE(x-mac-hebrew, 10005),
    CODE_PAGE(x-mac-greek, 10006),
    CODE_PAGE(x-mac-cyrillic, 10007),
    CODE_PAGE(x-mac-chinesesimp, 10008),
    CODE_PAGE(x-mac-romanian, 10010),
    CODE_PAGE(x-mac-ukrainian, 10017),
    CODE_PAGE(x-mac-thai, 10021),
    CODE_PAGE(x-mac-ce, 10029),
    CODE_PAGE(x-mac-icelandic, 10079),
    CODE_PAGE(x-mac-turkish, 10081),
    CODE_PAGE(x-mac-croatian, 10082),
    CODE_PAGE(utf-32, 12000),
    CODE_PAGE(utf-32BE, 12001),
    CODE_PAGE(x-Chinese_CNS, 20000),
    CODE_PAGE(x-cp20001, 20001),
    CODE_PAGE(x_Chinese-Eten, 20002),
    CODE_PAGE(x-cp20003, 20003),
    CODE_PAGE(x-cp20004, 20004),
    CODE_PAGE(x-cp20005, 20005),
    CODE_PAGE(x-IA5, 20105),
    CODE_PAGE(x-IA5-German, 20106),
    CODE_PAGE(x-IA5-Swedish, 20107),
    CODE_PAGE(x-IA5-Norwegian, 20108),
    CODE_PAGE(us-ascii, 20127),
    CODE_PAGE(x-cp20261, 20261),
    CODE_PAGE(x-cp20269, 20269),
    CODE_PAGE(IBM273, 20273),
    CODE_PAGE(IBM277, 20277),
    CODE_PAGE(IBM278, 20278),
    CODE_PAGE(IBM280, 20280),
    CODE_PAGE(IBM284, 20284),
    CODE_PAGE(IBM285, 20285),
    CODE_PAGE(IBM290, 20290),
    CODE_PAGE(IBM297, 20297),
    CODE_PAGE(IBM420, 20420),
    CODE_PAGE(IBM423, 20423),
    CODE_PAGE(IBM424, 20424),
    CODE_PAGE(x-EBCDIC-KoreanExtended, 20833),
    CODE_PAGE(IBM-Thai, 20838),
    CODE_PAGE(koi8-r, 20866),
    CODE_PAGE(IBM871, 20871),
    CODE_PAGE(IBM880, 20880),
    CODE_PAGE(IBM905, 20905),
    CODE_PAGE(IBM00924, 20924),
    CODE_PAGE(EUC-JP, 20932),
    CODE_PAGE(x-cp20936, 20936),
    CODE_PAGE(x-cp20949, 20949),
    CODE_PAGE(cp1025, 21025),
    CODE_PAGE(koi8-u, 21866),
    CODE_PAGE(iso-8859-1, 28591),
    CODE_PAGE(iso-8859-2, 28592),
    CODE_PAGE(iso-8859-3, 28593),
    CODE_PAGE(iso-8859-4, 28594),
    CODE_PAGE(iso-8859-5, 28595),
    CODE_PAGE(iso-8859-6, 28596),
    CODE_PAGE(iso-8859-7, 28597),
    CODE_PAGE(iso-8859-8, 28598),
    CODE_PAGE(iso-8859-9, 28599),
    CODE_PAGE(iso-8859-13, 28603),
    CODE_PAGE(iso-8859-15, 28605),
    CODE_PAGE(x-Europa, 29001),
    CODE_PAGE(iso-8859-8-i, 38598),
    CODE_PAGE(iso-2022-jp, 50220),
    CODE_PAGE(csISO2022JP, 50221),
    CODE_PAGE(iso-2022-jp, 50222),
    CODE_PAGE(iso-2022-kr, 50225),
    CODE_PAGE(x-cp50227, 50227),
    CODE_PAGE(euc-jp, 51932),
    CODE_PAGE(EUC-CN, 51936),
    CODE_PAGE(euc-kr, 51949),
    CODE_PAGE(hz-gb-2312, 52936),
    CODE_PAGE(GB18030, 54936),
    CODE_PAGE(x-iscii-de, 57002),
    CODE_PAGE(x-iscii-be, 57003),
    CODE_PAGE(x-iscii-ta, 57004),
    CODE_PAGE(x-iscii-te, 57005),
    CODE_PAGE(x-iscii-as, 57006),
    CODE_PAGE(x-iscii-or, 57007),
    CODE_PAGE(x-iscii-ka, 57008),
    CODE_PAGE(x-iscii-ma, 57009),
    CODE_PAGE(x-iscii-gu, 57010),
    CODE_PAGE(x-iscii-pa, 57011),
    CODE_PAGE(utf-7, 65000),
    CODE_PAGE(utf-8, 65001),
    {0, 0},
};

bool iconv(const std::string& in_charset, const std::string& out_charset, const std::string& input, std::string& output) {
    if (input.empty()) {
        return true;
    }
    unsigned int in_codepage = CP_ACP;
    unsigned int out_codepage = CP_ACP;
    CodePageReg* reg;
    bool in_found = false;
    bool out_found = false;
    uint64_t in_hash = Hash3(in_charset.c_str());
    uint64_t out_hash = Hash3(out_charset.c_str());
    for (reg = code_pages; reg->name; reg++) {
        if (!in_found && reg->hash == in_hash) {
            in_codepage = reg->value;
            in_found = true;
        }
        if (!out_found && reg->hash == out_hash) {
            out_codepage = reg->value;
            out_found = true;
        }
        if (in_found && out_found) break;
    }
    const char* in_buf = &input[0];
    int buf_size = MultiByteToWideChar(in_codepage, 0, (LPCSTR)in_buf, -1, NULL, 0);
    std::unique_ptr<wchar_t[]> buf_ptr(new(std::nothrow) wchar_t[buf_size]);
    if (!buf_ptr) {
        return false;
    }
    wmemset(buf_ptr.get(), 0, buf_size);
    int result = MultiByteToWideChar(in_codepage, 0, (LPCSTR)in_buf, -1, buf_ptr.get(), buf_size);
    if (result <= 0) {
        return false;
    }
    int out_size = WideCharToMultiByte(out_codepage, 0, (LPCWSTR)buf_ptr.get(), -1, NULL, 0, NULL, NULL);
    if (result <= 0) {
        return false;
    }
    output.resize(out_size);
    result = WideCharToMultiByte(out_codepage, 0, (LPCWSTR)buf_ptr.get(), -1, &output[0], out_size, NULL, NULL);
    if (result <= 0) {
        return false;
    }
    output.resize(out_size - 1); //remove trailing '\0'
    return true;
}
#else
bool iconv(const std::string& in_charset, const std::string& out_charset, const std::string& input, std::string& output) {
    if (input.empty()) {
        return false;
    }
    iconv_t cd = iconv_open(out_charset.c_str(), in_charset.c_str());
    if (cd == (iconv_t)-1) {
        return false;
    }
    char * in_buf = const_cast<char*>(&input[0]);
    size_t in_len = input.length();
    size_t out_len = 2 * in_len + 1;
    std::unique_ptr<char[]> out_ptr(new(std::nothrow) char[out_len]);
    if(!out_ptr) {
        return false;
    }
    char* out_buf = out_ptr.get();
    memset(out_ptr.get(), 0, out_len);
    size_t  result = ::iconv(cd, &in_buf, &in_len, &out_buf, &out_len);
    if (result == (size_t)-1) {
        iconv_close(cd);
        return false;
    }
    iconv_close(cd);
    output = out_ptr.get();
    return true;
}
#endif

struct BoolValueReg {
    const char* name;
    unsigned int hash;
    bool value;
};
static BoolValueReg bool_values[] = {
    {"true", Hash("true"), true},
    {"True", Hash("True"), true},
    {"TRUE", Hash("TRUE"), true},
    {"false", Hash("false"), false},
    {"False", Hash("False"), false},
    {"FALSE", Hash("FALSE"), false},
    {"yes", Hash("yes"), true},
    {"Yes", Hash("Yes"), true},
    {"YES", Hash("YES"), true},
    {"no", Hash("no"), false},
    {"No", Hash("No"), false},
    {"NO", Hash("NO"), false},
    {"on", Hash("on"), true},
    {"On", Hash("On"), true},
    {"ON", Hash("ON"), true},
    {"off", Hash("off"), false},
    {"Off", Hash("Off"), false},
    {"OFF", Hash("OFF"), false},
    {"1", Hash("1"), true},
    {"0", Hash("0"), false},
    {0, 0}
};
bool to_boolean(const char* str) {
    unsigned int value;
    BoolValueReg* reg;
    value = Hash(str);
    for (reg = bool_values; reg->name; reg++) {
        if (value == reg->hash) {
            return reg->value;
        }
    }
    return false;
}


std::string join(const std::vector<std::string>& values, const std::string& delimiter) {
    std::string ret;
    for (size_t i = 0; i < values.size(); ++i) {
        ret.append(values[i]);
        if (i < values.size() - 1)
            ret.append(delimiter);
    }
    return ret;
}

std::string join(const std::vector<tinynet::string_view>& values, const std::string& delimiter) {
    std::string ret;
    for (size_t i = 0; i < values.size(); ++i) {
        ret.append(values[i].data(), values[i].size());
        if (i < values.size() - 1)
            ret.append(delimiter);
    }
    return ret;
}
#ifdef _WIN32
std::wstring* convert_utf8_to_utf16(const std::string& u8_string, std::wstring* u16_string) {
    int size = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)u8_string.c_str(), -1, NULL, 0);
    if (size <= 0) {
        u16_string->assign(L"Convert Error");
        return NULL;
    }
    u16_string->resize(size);
    size = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)u8_string.c_str(), -1, &(*u16_string)[0], (int)u16_string->size());
    if (size <= 0) {
        u16_string->assign(L"Convert Error");
        return NULL;
    }
    u16_string->erase(u16_string->length() - 1); // length() - 1 is '\0', now we don't need it
    return u16_string;
}

std::string* convert_utf16_to_utf8(const std::wstring& u16_string, std::string* u8_string) {
    int size = WideCharToMultiByte(CP_UTF8, 0, u16_string.c_str(), -1, NULL, 0, NULL, NULL);
    if (size <= 0) {
        u8_string->assign("Convert Error");
        return NULL;
    }
    u8_string->resize(size);
    int res = WideCharToMultiByte(CP_UTF8, 0, u16_string.c_str(), -1, &(*u8_string)[0], (int)u8_string->size(), NULL, NULL);
    if (res <= 0) {
        u8_string->assign("Convert Error");
        return NULL;
    }
    u8_string->erase(u8_string->length() - 1); // length() - 1 is '\0', now we don't need it
    return u8_string;
}

#else
std::wstring* convert_utf8_to_utf16(const std::string& u8_string, std::wstring* u16_string) {
    const char* mbstr = u8_string.c_str();
    std::mbstate_t state = std::mbstate_t();
    std::size_t len = std::mbsrtowcs(NULL, &mbstr, 0, &state);
    if (len == (size_t)-1) {
        return NULL;
    }
    if (len == 0) {
        return u16_string;
    }
    u16_string->resize(len + 1);
    std::mbsrtowcs(&(*u16_string)[0], &mbstr, len + 1, &state);
    u16_string->resize(len);
    return u16_string;
}

std::string* convert_utf16_to_utf8(const std::wstring& u16_string, std::string* u8_string) {
    const wchar_t* wstr = u16_string.c_str();
    std::mbstate_t state = std::mbstate_t();
    std::size_t len = std::wcsrtombs(NULL, &wstr, 0, &state);
    if (len == (size_t)-1) {
        return NULL;
    }
    if (len == 0) {
        return u8_string;
    }
    u8_string->resize(len + 1);
    std::wcsrtombs(&(*u8_string)[0], &wstr, len + 1, &state);
    u8_string->resize(len);
    return u8_string;
}
#endif

size_t utf8_len(const char* str) {
    size_t len = strlen(str);
    size_t result = 0;
    for (size_t i = 0; i < len;) {
        char ch = str[i];
        size_t wanted = 0;
        while ((ch <<= 1) & 0x80) {
            ++wanted;
            if ((str[i + wanted] & 0xc0) != 0x80) {
                wanted = 0;
                break;
            }
        }
        i += wanted + 1;
        ++result;
    }
    return result;
}


void split_pieces(const std::string& str, size_t block_size, std::vector<std::string>& output) {
    for (size_t i = 0; i < str.length(); i += block_size) {
        output.emplace_back(str.substr(i, block_size));
    }
}

std::string dump_hex(const std::string& bin) {
    std::string hex_str(bin.length()*3, 0);
    for (size_t i = 0; i < bin.length(); ++i) {
        sprintf(&hex_str[i * 3], "%02x ", (unsigned char)bin[i]);
    }
    return hex_str;
}

std::wstring to_utf16_string(const std::string& u8_string) {
    std::wstring u16_string;
    convert_utf8_to_utf16(u8_string, &u16_string);
    return u16_string;
}

std::string to_utf8_string(const std::wstring& u16_string) {
    std::string u8_string;
    convert_utf16_to_utf8(u16_string, &u8_string);
    return u8_string;
}

}
