/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <openssl/ssl.h>
#include "object.h"

// func probe() bool
static TValue _probe(TValue *self, TValue *args, int nargs)
{
    const SSL_METHOD *m = TLS_client_method();
    if (m == NULL) {
        return bool_value(false);
    }
    SSL_CTX *ctx = SSL_CTX_new(m);
    if (ctx == NULL) {
        return bool_value(false);
    }
    SSL_CTX_free(ctx);
    return bool_value(true);
}

void tls_conn_register(NativeLib *lib);
void tls_cert_register(NativeLib *lib);

void tls_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "probe", _probe);
    tls_conn_register(lib);
    tls_cert_register(lib);
}
