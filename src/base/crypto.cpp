// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "crypto.h"
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <string>
#include <cassert>
#include <limits>
#include <stdexcept>
#include <cctype>
#include "util/string_utils.h"

namespace Crypto {

std::string md5(const std::string& src) {
    std::string buf(MD5_DIGEST_LENGTH, 0);
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, src.data(), src.length());
    MD5_Final((unsigned char*)&buf[0], &ctx);
    return buf;
}

std::string sha1(const std::string& src) {
    std::string buf(SHA_DIGEST_LENGTH, 0);
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, src.data(), src.length());
    SHA1_Final((unsigned char*)&buf[0], &ctx);
    return buf;
}

std::string bin2hex(const std::string& src) {
    std::string out(src.length() * 2, 0);
    for (size_t i = 0; i < src.length(); ++i) {
        sprintf(&out[i * 2], "%02X", (unsigned char)src[i]);
    }
    return out;
}

std::string hex2bin(const std::string& src) {
    size_t len = src.length() / 2;
    std::string out(len, 0);
    for (size_t i = 0; i < len; ++i) {
        int value = 0;
        sscanf(&src[2 * i], "%02X", &value);
        out[i] = (char)value;
    }
    return out;
}
std::string sha256(const std::string& src) {
    std::string buf(SHA256_DIGEST_LENGTH, 0);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, src.data(), src.length());
    SHA256_Final((unsigned char*)&buf[0], &ctx);
    return buf;
}

struct AlgoReg {
    const char* name;
    unsigned int hash;
    const EVP_MD* md;
};

static AlgoReg s_algos[] = {
    {"md5", StringUtils::Hash("md5"), EVP_md5()},
    {"sha1", StringUtils::Hash("sha1"), EVP_sha1()},
    {"sha256", StringUtils::Hash("sha256"), EVP_sha256()},
    {0, 0}
};

void hash_hmac_algos(std::vector<std::string>& algo_vec) {
    AlgoReg* reg;
    for (reg = s_algos; reg->name; reg++) {
        algo_vec.emplace_back(reg->name);
    }
}

std::string hash_hmac(const std::string& algo, const std::string& data, const std::string& key) {
    unsigned char buf[1024] = { 0 };
    unsigned int len = static_cast<unsigned int>(sizeof(buf));
    const EVP_MD * md = NULL;
    AlgoReg* reg;
    unsigned int hash = StringUtils::Hash(algo.c_str());
    for (reg = s_algos; reg->name; reg++) {
        if (hash == reg->hash) {
            md = reg->md;
            break;
        }
    }
    if (md == NULL) {
        return "ALGO NOT SUPPORTED";
    }
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    HMAC_CTX ctx_impl;
    HMAC_CTX* ctx = &ctx_impl;
    HMAC_CTX_init(ctx);
#else
    HMAC_CTX* ctx = HMAC_CTX_new();
#endif
    if (ctx == NULL) {
        return "";
    }
    HMAC_Init_ex(ctx, key.data(), static_cast<int>(key.length()), md, NULL);
    HMAC_Update(ctx, (const unsigned char*)data.data(), data.length());
    HMAC_Final(ctx, buf, &len);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    HMAC_CTX_cleanup(ctx);
#else
    HMAC_CTX_free(ctx);
    ctx = NULL;
#endif


    std::string out(len * 2, 0);
    for (unsigned int i = 0; i < len; ++i) {
        sprintf(&out[i * 2], "%02X", buf[i]);
    }
    return out;
}

// source code from https://stackoverflow.com/questions/5288076/base64-encoding-and-decoding-with-openssl

static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char reverse_table[128] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
};

::std::string base64_encode(const tinynet::string_view &bindata) {
    using ::std::string;
    using ::std::numeric_limits;

    if (bindata.size() > (numeric_limits<string::size_type>::max() / 4u) * 3u) {
        //throw ::std::length_error("Converting too large a string to base64.");
        return std::string("Converting too large a string to base64.");
    }

    const ::std::size_t binlen = bindata.size();
    // Use = signs so the end is properly padded.
    string retval((((binlen + 2) / 3) * 4), '=');
    ::std::size_t outpos = 0;
    int bits_collected = 0;
    unsigned int accumulator = 0;
    auto binend = bindata.end();

    for (auto i = bindata.begin(); i != binend; ++i) {
        accumulator = (accumulator << 8) | (*i & 0xffu);
        bits_collected += 8;
        while (bits_collected >= 6) {
            bits_collected -= 6;
            retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
        }
    }
    if (bits_collected > 0) { // Any trailing bits that are missing.
        assert(bits_collected < 6);
        accumulator <<= 6 - bits_collected;
        retval[outpos++] = b64_table[accumulator & 0x3fu];
    }
    assert(outpos >= (retval.size() - 2));
    assert(outpos <= retval.size());
    return retval;
}

::std::string base64_decode(const tinynet::string_view &ascdata) {
    using ::std::string;
    string retval;
    auto last = ascdata.end();
    int bits_collected = 0;
    unsigned int accumulator = 0;

    for (auto i = ascdata.begin(); i != last; ++i) {
        const int c = *i;
        if (::std::isspace(c) || c == '=') {
            // Skip whitespace and padding. Be liberal in what you accept.
            continue;
        }
        if ((c > 127) || (c < 0) || (reverse_table[c] > 63)) {
            throw ::std::invalid_argument("This contains characters not legal in a base64 encoded string.");
        }
        accumulator = (accumulator << 6) | reverse_table[c];
        bits_collected += 6;
        if (bits_collected >= 8) {
            bits_collected -= 8;
            retval += static_cast<char>((accumulator >> bits_collected) & 0xffu);
        }
    }
    return retval;
}

static const uint32_t crc32table[256] = {
    0x0000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x76dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0xedb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x9b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x1db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x6b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0xf00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x86d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x3b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x4db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0xd6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0xa00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x26d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x5005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0xcb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0xbdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};

uint32_t crc32(const char *buf, int len) {
    int counter;
    uint32_t crc = 0xFFFFFFFF;
    for (counter = 0; counter < len; counter++)
        crc = ((crc >> 8) & 0x00FFFFFF) ^ crc32table[(crc ^ buf[counter]) & 0xFF];
    uint32_t result = crc ^ 0xFFFFFFFF;
    return result;
}

/*
* Copyright 2001-2010 Georges Menie (www.menie.org)
* Copyright 2010 Salvatore Sanfilippo (adapted to Redis coding style)
* All rights reserved.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the University of California, Berkeley nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* CRC16 implementation acording to CCITT standards.
*
* Note by @antirez: this is actually the XMODEM CRC 16 algorithm, using the
* following parameters:
*
* Name                       : "XMODEM", also known as "ZMODEM", "CRC-16/ACORN"
* Width                      : 16 bit
* Poly                       : 1021 (That is actually x^16 + x^12 + x^5 + 1)
* Initialization             : 0000
* Reflect Input byte         : False
* Reflect Output CRC         : False
* Xor constant to output CRC : 0000
* Output for "123456789"     : 31C3
*/
static const uint16_t crc16tab[256] = {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
    0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
    0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
    0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
    0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
    0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
    0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
    0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
    0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
    0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
    0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
    0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
    0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

uint16_t crc16(const char *buf, int len) {
    int counter;
    uint16_t crc = 0;
    for (counter = 0; counter < len; counter++)
        crc = (crc << 8) ^ crc16tab[((crc >> 8) ^ *buf++) & 0x00FF];
    return crc;
}


std::string openssl_sign(const std::string& data, const std::string& key, const std::string& algo) {
    const EVP_MD * md = NULL;
    AlgoReg* reg;
    unsigned int hash = StringUtils::Hash(algo.c_str());
    for (reg = s_algos; reg->name; reg++) {
        if (hash == reg->hash) {
            md = reg->md;
            break;
        }
    }
    if (md == NULL) {
        return "ALGO NOT SUPPORTED";
    }
    BIO* bio;
    EVP_PKEY* pk;
    EVP_MD_CTX* ctx;

    bio = BIO_new_mem_buf((void*)key.c_str(), (int)key.length());
    if (bio == NULL) {
        return "";
    }
    pk = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
    if (pk == NULL) {
        BIO_free(bio);
        return "";
    }
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX ctx_impl;
    ctx = &ctx_impl;
    EVP_MD_CTX_init(ctx);
#else
    ctx = EVP_MD_CTX_create();
#endif
    if (ctx == NULL) {
        BIO_free(bio);
        EVP_PKEY_free(pk);
        return "";
    }
    unsigned int len = (unsigned int)EVP_PKEY_size(pk);
    std::string res;
    res.resize(len);
    if (!EVP_SignInit(ctx, md) || !EVP_SignUpdate(ctx, &data[0], data.length()) || !EVP_SignFinal(ctx, (unsigned char*)&res[0], &len, pk)) {
        char buf[1024] = { 0 };
        ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
        res.append(buf);
    }
    BIO_free(bio);
    EVP_PKEY_free(pk);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX_cleanup(ctx);
#else
    EVP_MD_CTX_free(ctx);
#endif
    return res;
}

bool openssl_verify(const std::string& data, const std::string& sign, const std::string& key, const std::string& algo) {
    const EVP_MD * md = NULL;
    AlgoReg* reg;
    unsigned int hash = StringUtils::Hash(algo.c_str());
    for (reg = s_algos; reg->name; reg++) {
        if (hash == reg->hash) {
            md = reg->md;
            break;
        }
    }
    if (md == NULL) {
        return false;
    }
    BIO* bio;
    EVP_PKEY* pk;
    EVP_MD_CTX* ctx;

    bio = BIO_new_mem_buf((void*)key.c_str(), (int)key.length());
    if (bio == NULL) {
        return false;
    }
    pk = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
    if (pk == NULL) {
        char buf[1024] = { 0 };
        ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
        BIO_free(bio);
        return false;
    }
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX ctx_impl;
    ctx = &ctx_impl;
    EVP_MD_CTX_init(ctx);
#else
    ctx = EVP_MD_CTX_create();
#endif
    if (ctx == NULL) {
        BIO_free(bio);
        EVP_PKEY_free(pk);
        return false;
    }
    bool res = EVP_VerifyInit(ctx, md) && EVP_VerifyUpdate(ctx, &data[0], data.length()) && EVP_VerifyFinal(ctx, (unsigned char*)&sign[0], (unsigned int)sign.length(), pk) >= 0;
    BIO_free(bio);
    EVP_PKEY_free(pk);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_MD_CTX_cleanup(ctx);
#else
    EVP_MD_CTX_free(ctx);
#endif
    return res;
}

int openssl_cipher_iv_length(const char* name) {
    const EVP_CIPHER* type = EVP_get_cipherbyname(name);
    if (!type) {
        return -1;
    }
    return EVP_CIPHER_iv_length(type);
}

std::string openssl_random_pseudo_bytes(int num) {
    std::string result;
    if (num <= 0) {
        return result;
    }
    result.resize(num);
    RAND_bytes((unsigned char*)&result[0], num);
    return result;
}


std::string openssl_encrypt(const std::string& data, const char* method, const std::string& key,
                            const std::string* iv /*= nullptr*/,
                            const std::string* aad /* = nullptr*/,
                            std::string* tag /*= nullptr*/) {
    std::string out;
    const EVP_CIPHER *cipher = EVP_get_cipherbyname(method);
    if (!cipher) {
        out = "Cipher can not be found";
        return out;
    }
    int key_len = EVP_CIPHER_key_length(cipher);
    if (key_len != (int)key.length()) {
        out = "Invalid key length";
        return out;
    }
    const unsigned char* iv_data = iv ? (unsigned char*)iv->data() : NULL;
    int iv_len = iv ? (int)iv->length() : 0;
    const unsigned char* add_data = aad ? (unsigned char*)aad->data() : NULL;
    int aad_len = aad ? (int)aad->length() : 0;
    EVP_CIPHER_CTX* ctx;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX ctx_impl;
    ctx = &ctx_impl;
    EVP_CIPHER_CTX_init(ctx);
#else
    ctx = EVP_CIPHER_CTX_new();
#endif
    if (!ctx) {
        out = "Cipher context create failed";
        return out;
    }
    int mode = EVP_CIPHER_mode(cipher);
    int out1 = 0, out2 = 0;
    out.resize(data.length() + EVP_MAX_BLOCK_LENGTH);
    if (mode == EVP_CIPH_GCM_MODE) {
        if (!EVP_EncryptInit_ex(ctx, cipher, NULL, NULL, NULL)) {
            out = "Encrypt init failed";
            goto FINALIZE;
        }
        if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL)) {
            out = "IV length set failed";
            goto FINALIZE;
        }
        if (!EVP_EncryptInit_ex(ctx, NULL, NULL, (unsigned char*)key.data(), iv_data)) {
            out = "Key/IV set failed";
            goto FINALIZE;
        }
        if (aad != NULL && !EVP_EncryptUpdate(ctx, NULL, &out1, add_data, aad_len)) {
            out = "AAD set failed";
            goto FINALIZE;
        }
    } else if (mode == EVP_CIPH_CCM_MODE) {
        if (!EVP_EncryptInit_ex(ctx, cipher, NULL, NULL, NULL)) {
            out = "Encrypt init failed";
            goto FINALIZE;
        }
        if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, iv_len, NULL)) {
            out = "IV length set failed";
            goto FINALIZE;
        }
        if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, 14, NULL)) {
            out = "Tag length set failed";
            goto FINALIZE;
        }
        if (!EVP_EncryptInit_ex(ctx, cipher, NULL, (unsigned char*)key.data(), iv_data)) {
            out = "Key/IV set failed";
            goto FINALIZE;
        }
        if (!EVP_EncryptUpdate(ctx, NULL, &out1, NULL, (int)data.length())) {
            out = "Data length set failed";
            goto FINALIZE;
        }
        if (aad != NULL && !EVP_EncryptUpdate(ctx, NULL, &out1, add_data, aad_len)) {
            out = "AAD set failed";
            goto FINALIZE;
        }
    } else if (!EVP_EncryptInit_ex(ctx, cipher, NULL, (unsigned char*)key.data(), iv_data)) {
        out = "Encrypt init failed";
        goto FINALIZE;
    }
    if (!EVP_EncryptUpdate(ctx, (unsigned char*)&out[0], &out1, (unsigned char*)data.data(), (int)data.length())) {
        out = "Encrypt failed";
        goto FINALIZE;
    }
    if (!EVP_EncryptFinal_ex(ctx, (unsigned char*)&out[out1], &out2)) {
        out = "Encrypt final failed";
        goto FINALIZE;
    }
    out.resize(out1 + out2);
    if (tag) {
        if (mode == EVP_CIPH_GCM_MODE) {
            int tag_size = 16;
            tag->resize(tag_size);
            if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, tag_size, &(*tag)[0])) {
                tag->resize(0);
            }
        } else if (mode == EVP_CIPH_CCM_MODE) {
            int tag_size = 14;
            tag->resize(tag_size);
            if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_GET_TAG, tag_size, &(*tag)[0])) {
                tag->resize(0);
            }
        }
    }
FINALIZE:
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX_cleanup(ctx);
#else
    EVP_CIPHER_CTX_free(ctx);
#endif
    return out;
}

std::string openssl_decrypt(const std::string& data, const char* method, const std::string& key, const std::string* iv /* = nullptr */, const std::string* aad /* = nullptr */, std::string* tag /* = nullptr */) {
    std::string out;
    const EVP_CIPHER *cipher = EVP_get_cipherbyname(method);
    if (!cipher) {
        out = "Cipher can not be found";
        return out;
    }
    int key_len = EVP_CIPHER_key_length(cipher);
    if (key_len != (int)key.length()) {
        out = "Invalid key length";
        return out;
    }
    const unsigned char* iv_data = iv ? (unsigned char*)iv->data() : NULL;
    int iv_len = iv ? (int)iv->length() : 0;
    const unsigned char* add_data = aad ? (unsigned char*)aad->data() : NULL;
    int aad_len = aad ? (int)aad->length() : 0;
    unsigned char* tag_data = tag ? (unsigned char*)tag->data() : NULL;
    int tag_len = tag ? (int)tag->length() : 0;
    EVP_CIPHER_CTX* ctx;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX ctx_impl;
    ctx = &ctx_impl;
    EVP_CIPHER_CTX_init(ctx);
#else
    ctx = EVP_CIPHER_CTX_new();
#endif
    if (!ctx) {
        out = "Cipher context create failed";
        return out;
    }
    int mode = EVP_CIPHER_mode(cipher);
    int out1 = 0, out2 = 0;
    out.resize(data.length() + EVP_MAX_BLOCK_LENGTH);
    if (mode == EVP_CIPH_GCM_MODE) {
        if (!EVP_DecryptInit_ex(ctx, cipher, NULL, NULL, NULL)) {
            out = "Decrypt init failed";
            goto FINALIZE;
        }
        if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL)) {
            out = "IV length set failed";
            goto FINALIZE;
        }
        if (!EVP_DecryptInit_ex(ctx, NULL, NULL, (unsigned char*)key.data(), iv_data)) {
            out = "Key/IV set failed";
            goto FINALIZE;
        }
        if (aad != NULL && !EVP_EncryptUpdate(ctx, NULL, &out1, add_data, aad_len)) {
            out = "AAD set failed";
            goto FINALIZE;
        }
    } else if (mode == EVP_CIPH_CCM_MODE) {
        if (!EVP_DecryptInit_ex(ctx, cipher, NULL, NULL, NULL)) {
            out = "Decrypt init failed";
            goto FINALIZE;
        }
        if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_IVLEN, iv_len, NULL)) {
            out = "IV length set failed";
            goto FINALIZE;
        }
        if (tag && !EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_CCM_SET_TAG, tag_len, (void*)tag_data)) {
            out = "Tag length set failed";
            goto FINALIZE;
        }
        if (!EVP_DecryptInit_ex(ctx, cipher, NULL, (unsigned char*)key.data(), iv_data)) {
            out = "Key/IV set failed";
            goto FINALIZE;
        }
        if (aad && !EVP_DecryptUpdate(ctx, NULL, &out1, NULL, (int)data.length())) {
            out = "Data length set failed";
            goto FINALIZE;
        }
        if (aad && !EVP_DecryptUpdate(ctx, NULL, &out1, add_data, aad_len)) {
            out = "AAD set failed";
            goto FINALIZE;
        }
    } else if (!EVP_DecryptInit_ex(ctx, cipher, NULL, (unsigned char*)key.data(), iv_data)) {
        out = "Encrypt init failed";
        goto FINALIZE;
    }
    if (!EVP_DecryptUpdate(ctx, (unsigned char*)&out[0], &out1, (unsigned char*)data.data(), (int)data.length())) {
        out = "Encrypt failed";
        goto FINALIZE;
    }
    if (mode == EVP_CIPH_GCM_MODE) {
        if (tag_data && !EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_len, tag_data)) {
            out = "Set tag failed";
            goto FINALIZE;
        }
        if (!EVP_DecryptFinal_ex(ctx, (unsigned char*)&out[out1], &out2)) {
            out = "Encrypt final failed";
            goto FINALIZE;
        }
    }
    out.resize(out1 + out2);
FINALIZE:
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_CIPHER_CTX_cleanup(ctx);
#else
    EVP_CIPHER_CTX_free(ctx);
#endif
    return out;
}
}