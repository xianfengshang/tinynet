// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "ssl_context.h"
#include "base/at_exit.h"


namespace tinynet {

std::once_flag SSLContext::once_flag_;

void SSLContext::Initialize() {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_library_init();
    OpenSSL_add_ssl_algorithms();
    SSL_load_error_strings();
    ERR_load_crypto_strings();
#else
    OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT, NULL);
#endif
    tinynet::AtExitManager::RegisterCallback(Finalize, nullptr);
}

void SSLContext::Finalize(void *) {
    FIPS_mode_set(0);
    ENGINE_cleanup();
    CONF_modules_unload(1);
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_remove_state(0);
    ERR_free_strings();
}

SSLContext::SSLContext():
    ctx_(NULL) {
}

SSLContext::~SSLContext() {
    if (ctx_ != NULL) {
        SSL_CTX_free(ctx_);
        ctx_ = NULL;
    }
}

bool SSLContext::Init(const Options& opts) {
    std::call_once(once_flag_, Initialize);
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    ctx_ = SSL_CTX_new(SSLv23_method());
#else
    ctx_ = SSL_CTX_new(TLS_method());
#endif
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    if (SSL_CTX_get_security_level(ctx_) > 1)
        SSL_CTX_set_security_level(ctx_, 1);
#endif

    SSL_CTX_set_verify(ctx_, SSL_VERIFY_NONE, NULL);
    SSL_CTX_set_cipher_list(ctx_, "DEFAULT:!DH");
    //SSL_CTX_use_certificate_file(ctx_, "cert/client-cert.pem", SSL_FILETYPE_PEM);
    //SSL_CTX_use_PrivateKey_file(ctx_, "cert/client-key.pem", SSL_FILETYPE_PEM);
    //int ret = SSL_CTX_check_private_key(ctx_);
    //TODO: Manipulate the ssl context
    return true;
}

}

