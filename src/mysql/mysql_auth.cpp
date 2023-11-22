// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "mysql_protocol.h"
#include "mysql_auth.h"
#include "base/crypto.h"
#include "util/string_utils.h"
#include "openssl/rsa.h"
#include "openssl/pem.h"
#include "openssl/err.h"


namespace mysql {

static const int SCRAMBLE_LENGTH = 20;

static const int SHA2_SCRAMBLE_LENGTH = 32;


static bool mysql_native_password(const InputAuthPluginData& input, OutpuAuthPluginData& output) {
    std::string stage1 = Crypto::sha1(*input.password);
    std::string stage2;
    stage2.append(*input.auth_plugin_data);
    stage2.append(Crypto::sha1(stage1));
    std::string stage3 = Crypto::sha1(stage2);
    output.auth_response->resize(stage1.length());
    for (size_t i = 0; i < stage1.length(); ++i) {
        (*output.auth_response)[i] =  stage1[i] ^ stage3[i];
    }
    if (output.flags)
        *output.flags |= protocol::CLIENT_SECURE_CONNECTION;
    return true;
}

static bool mysql_clear_password(const InputAuthPluginData& input, OutpuAuthPluginData& output) {
    output.auth_response->assign(*input.password);
    return true;
}

namespace old {
struct HashResult {
    unsigned long hash;
    unsigned long hash2;
};
HashResult hash(const char* str, size_t len) {
    unsigned nr = 1345345333L, add = 7, nr2 = 0x12345671L;
    unsigned tmp = 0;
    for (size_t i = 0; i < len; ++i) {
        tmp = str[i];
        if (tmp == ' ' || tmp == '\t') {
            continue;
        }
        nr ^= (((nr & 63) + add) * tmp) + (nr << 8);
        nr2 += (nr2 << 8) ^ nr;
        add += tmp;
    }
    HashResult result;
    result.hash = nr & (((unsigned long)1L << 31) - 1L);
    result.hash2 = nr2 & (((unsigned long)1L << 31) - 1L);
    return result;
}

struct RandomContext {
    unsigned seed;
    unsigned seed2;
};

const long MAX_RANDOM_VALUE = 0x3FFFFFFFL;

void random_init(RandomContext* ctx, long seed, long seed2) {
    ctx->seed = seed % MAX_RANDOM_VALUE;
    ctx->seed2 = seed2 % MAX_RANDOM_VALUE;
}

long random(RandomContext* ctx) {
    ctx->seed = (ctx->seed * 3 + ctx->seed2) % MAX_RANDOM_VALUE;
    ctx->seed2 = (ctx->seed + ctx->seed2 + 33) % MAX_RANDOM_VALUE;
    return (ctx->seed * 31) / MAX_RANDOM_VALUE;
}


const int OLD_SCRAMBLE_LENGTH = 8;
}


static bool mysql_old_password(const InputAuthPluginData& input, OutpuAuthPluginData& output) {
    old::RandomContext ctx;
    old::HashResult hash_pass =  old::hash(input.password->c_str(), input.password->length());
    old::HashResult hash_input = old::hash(input.auth_plugin_data->c_str(), old::OLD_SCRAMBLE_LENGTH);
    random_init(&ctx, hash_pass.hash ^ hash_input.hash, hash_pass.hash2 ^ hash_input.hash2);
    output.auth_response->resize(old::OLD_SCRAMBLE_LENGTH);
    for (int i = 0; i < old::OLD_SCRAMBLE_LENGTH; ++i) {
        (*output.auth_response)[i] = (char)(old::random(&ctx) + 64);
    }
    char salt = (char)random(&ctx);
    for (int i = 0; i < old::OLD_SCRAMBLE_LENGTH; ++i) {
        (*output.auth_response)[i] ^= salt;
    }
    if (output.flags)
        *output.flags |= protocol::CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA;
    return true;
}

std::string rsa_public_encrypt(const char* data, size_t len, const std::string& pub_key) {
    BIO* bio;
    RSA* rsa;
    std::string res;
    bio = BIO_new_mem_buf((void*)&pub_key[0], (int)pub_key.length());
    if (bio == NULL) {
        return res;
    }
    rsa = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);
    if (rsa == NULL) {
        BIO_free(bio);
        return res;
    }
    res.resize(RSA_size(rsa));
    if (RSA_public_encrypt((int)len, (unsigned char*)data, (unsigned char*)&res[0], rsa, RSA_PKCS1_OAEP_PADDING) < 0) {
        char buf[1024] = { 0 };
        ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
        res.append(buf);
    }
    BIO_free(bio);
    ERR_clear_error();
    RSA_free(rsa);
    return res;
}

static bool caching_sha2_password(const InputAuthPluginData& input, OutpuAuthPluginData& output) {
    if (!input.auth_extra_data || input.auth_extra_data->empty()) {
        std::string stage1 = Crypto::sha256(*input.password);
        std::string stage2;
        stage2.append(Crypto::sha256(stage1));
        stage2.append(*input.auth_plugin_data);
        std::string stage3 = Crypto::sha256(stage2);
        output.auth_response->resize(stage1.length());
        for (size_t i = 0; i < stage1.length(); ++i) {
            (*output.auth_response)[i] = stage1[i] ^ stage3[i];
        }
        *output.auth_response = stage1;
        if (output.flags)
            *output.flags |= protocol::CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA;
        return true;
    }
    if (input.auth_extra_data->length() == 1 && (*input.auth_extra_data)[0] == protocol::PERFORM_FULL_AUTHENTICATION) {
        output.auth_response->append(1, 2);
        return true;
    }
    size_t len = input.password->length() + 1;
    std::string data;
    data.resize(len);
    for (size_t i = 0; i < len; ++i) {
        data[i] = (*input.password)[i] ^ (*input.auth_plugin_data)[i % SCRAMBLE_LENGTH];
    }
    *output.auth_response = rsa_public_encrypt(&data[0], len, *input.auth_extra_data);
    if (output.flags)
        *output.flags |= protocol::CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA;
    return true;
}

static bool sha256_password(const InputAuthPluginData& input, OutpuAuthPluginData& output) {
    if (!input.auth_extra_data || input.auth_extra_data->empty()) {
        output.auth_response->append(1, 1);
        return true;
    }
    size_t len = input.password->length() + 1;
    std::string data;
    data.resize(len);
    for (size_t i = 0; i < len; ++i) {
        data[i] = (*input.password)[i] ^ (*input.auth_plugin_data)[i % SCRAMBLE_LENGTH];
    }
    *output.auth_response = rsa_public_encrypt(&data[0], len, *input.auth_extra_data);
    if (output.flags)
        *output.flags |= protocol::CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA;
    return true;
}


typedef bool(*AuthMethod)(const InputAuthPluginData& input, OutpuAuthPluginData& ouput);

struct AuthPluginReg {
    const char* name;
    uint64_t hash;
    AuthMethod method;
};

static AuthPluginReg s_auth_plugins[] = {
    {"mysql_native_password", StringUtils::Hash3("mysql_native_password"), mysql_native_password},
    {"mysql_clear_password", StringUtils::Hash3("mysql_clear_password"), mysql_clear_password},
    {"mysql_old_password", StringUtils::Hash3("mysql_old_password"), mysql_old_password},
    {"caching_sha2_password", StringUtils::Hash3("caching_sha2_password"), caching_sha2_password},
    {"sha256_password", StringUtils::Hash3("sha256_password"), sha256_password},
    {NULL, 0, NULL}
};


bool invoke_mysql_auth_plugin(const InputAuthPluginData& input, OutpuAuthPluginData& output) {
    if (!input.password || input.password->empty()) {
        return true;
    }
    const char* auth_plugin_name = input.auth_plugin_name->empty() ? "mysql_old_password" : input.auth_plugin_name->c_str();
    auto hash = StringUtils::Hash3(auth_plugin_name);
    AuthPluginReg* reg;
    for (reg = s_auth_plugins; reg->name; ++reg) {
        if (reg->hash == hash) {
            return reg->method(input, output);
        }
    }
    return false;
}
}
