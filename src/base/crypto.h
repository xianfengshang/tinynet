// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#pragma once
#include <string>
#include <vector>
#include "base/string_view.h"
namespace Crypto {
std::string sha1(const std::string& src);

std::string md5(const std::string& src);

std::string bin2hex(const std::string& src);

std::string hex2bin(const std::string& src);

std::string base64_encode(const tinynet::string_view &bindata);

std::string base64_decode(const tinynet::string_view &ascdata);

uint32_t crc32(const char* buf, int len);

uint16_t crc16(const char *buf, int len);

std::string hash_hmac(const std::string& algo, const std::string& data, const std::string& key);

void hash_hmac_algos(std::vector<std::string>& algo_vec);

std::string openssl_sign(const std::string& data, const std::string& key, const std::string& algo);

bool openssl_verify(const std::string& data, const std::string& sign, const std::string& key, const std::string& algo);

std::string sha256(const std::string& src);

int openssl_cipher_iv_length(const char* name);

std::string openssl_random_pseudo_bytes(int num);

std::string openssl_encrypt(const std::string& data, const char* method, const std::string& key,
                            const std::string* iv = nullptr,
                            const std::string* aad = nullptr,
                            std::string* tag = nullptr);

std::string openssl_decrypt(const std::string& data, const char* method, const std::string& key,
                            const std::string* iv = nullptr,
                            const std::string* aad = nullptr,
                            std::string* tag = nullptr);

}