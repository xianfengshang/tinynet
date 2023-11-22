// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <vector>
#include <stdio.h>
#include <cstdarg>
#include <tuple>
#include <cstring>
#include <memory>
#include <map>
#include "base/string_view.h"

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

namespace StringUtils {
void Split(const std::string& str, const std::string& delimiters, std::vector<std::string>& output);

std::string Trim(const std::string& src);

bool StartsWith(const std::string &src, const std::string &str);

bool EndsWith(const std::string &src, const std::string &str);

bool StartWith(const std::string &src, const std::string &str);

bool EndWith(const std::string &src, const std::string &str);

std::string& Format(std::string& dst, const char *fmt, ...);

std::string& VFormat(std::string& dst, const char *fmt, va_list arg);


std::string& to_lower(std::string& str);

std::string& to_upper(std::string& str);

/// BKDR hash function
unsigned int Hash(const char* str);

/// djb2 hash function
unsigned int Hash2(const char* str);

/// A combination of BKDR and djb2
unsigned long long Hash3(const char* str);

bool iconv(const std::string& in_charset, const std::string& out_charset, const std::string& input, std::string& output);

bool to_boolean(const char* str);

std::string join(const std::vector<std::string>& values, const std::string& delimiter = "");

std::string join(const std::vector<tinynet::string_view>& values, const std::string& delimiter = "");

std::wstring* convert_utf8_to_utf16(const std::string& u8_string, std::wstring* u16_string);

std::string* convert_utf16_to_utf8(const std::wstring& u16_string, std::string* u8_string);

std::wstring to_utf16_string(const std::string& u8_string);

std::string to_utf8_string(const std::wstring& u16_string);

size_t utf8_len(const char* str);

void split_pieces(const std::string& str, size_t block_size, std::vector<std::string>& output);

std::string dump_hex(const std::string& bin);
}
