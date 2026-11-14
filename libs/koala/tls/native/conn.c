/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * TLS connection over an already-connected TCP fd, in both directions:
 * tls_conn_new runs the client handshake, tls_accept the server one.
 * Follows Go's crypto/tls split: the TLS layer only wraps an existing
 * connection and never dials or listens on its own. The Koala-side
 * tls.dial is the convenience that does TcpConn.connect + TlsConn.wrap.
 *
 * The server certificate context that tls_accept needs is built in
 * certificate.c; read/write/close below are handle-based and serve both
 * directions unchanged.
 */

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <string.h>
#include "bytesobj.h"

// One client SSL_CTX for the whole process. Created on first use,
// never freed: it holds the parsed CA bundle and per-process state.
static SSL_CTX *g_ctx = NULL;

static SSL_CTX *get_ctx(void)
{
    if (g_ctx != NULL) {
        return g_ctx;
    }
    g_ctx = SSL_CTX_new(TLS_client_method());
    if (g_ctx == NULL) {
        return NULL;
    }
    SSL_CTX_set_verify(g_ctx, SSL_VERIFY_PEER, NULL);

    // Many servers close the TCP connection without sending
    // close_notify. Treat that as EOF, not as an error.
    SSL_CTX_set_options(g_ctx, SSL_OP_IGNORE_UNEXPECTED_EOF);

    if (SSL_CTX_set_default_verify_paths(g_ctx) != 1) {
        SSL_CTX_free(g_ctx);
        g_ctx = NULL;
        return NULL;
    }
    return g_ctx;
}

// A trust store built from PEM text, for a caller that trusts a private
// CA or a self-signed server certificate. Returns NULL when the PEM
// holds no certificate at all.
static X509_STORE *store_from_pem(const char *pem, int len)
{
    BIO *bio = BIO_new_mem_buf(pem, len);
    if (bio == NULL) {
        return NULL;
    }

    X509_STORE *store = X509_STORE_new();
    if (store == NULL) {
        BIO_free(bio);
        return NULL;
    }

    int count = 0;
    for (;;) {
        X509 *c = PEM_read_bio_X509(bio, NULL, NULL, NULL);
        if (c == NULL) {
            break;
        }
        // A duplicate is not a failure: the same CA may appear twice.
        if (X509_STORE_add_cert(store, c) == 1) {
            count++;
        }
        X509_free(c);
    }
    BIO_free(bio);
    ERR_clear_error();

    if (count == 0) {
        X509_STORE_free(store);
        return NULL;
    }
    return store;
}

// func tls_conn_new(fd int, hostname str, verify bool, ca_pem str) int64
//
// Wraps an established TCP fd in a TLS session, sending `hostname` as
// SNI and verifying the server certificate. Trust comes from the system
// CA store, or from `ca_pem` when it is non-empty. Returns a handle, or
// 0 when the handshake or verification fails.
//
// The hostname is copied into a NUL-terminated buffer because
// SSL_set_tlsext_host_name expects a C string and Koala's str has no
// terminator.
static TValue _tls_conn_new(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 4);
    int64_t fd = kl_arg_int(0);
    StringObject *host = kl_arg_obj_as(1, str_type);
    ASSERT(host != NULL);
    int verify = kl_arg_bool(2);
    StringObject *ca = kl_arg_obj_as(3, str_type);
    ASSERT(ca != NULL);

    char hostname[256];
    size_t hlen = STR_LEN(host);
    if (hlen == 0 || hlen >= sizeof(hostname)) {
        return int64_value(0);
    }
    memcpy(hostname, STR_BUF(host), hlen);
    hostname[hlen] = '\0';

    SSL_CTX *ctx = get_ctx();
    if (ctx == NULL) {
        return int64_value(0);
    }

    SSL *ssl = SSL_new(ctx);
    if (ssl == NULL) {
        return int64_value(0);
    }

    if (SSL_set_fd(ssl, (int)fd) != 1) {
        SSL_free(ssl);
        return int64_value(0);
    }

    // `verify` false is for tests only: any certificate is accepted.
    if (!verify) {
        SSL_set_verify(ssl, SSL_VERIFY_NONE, NULL);
    }

    // A caller-supplied trust store replaces the system one for this
    // connection only; the shared client CTX is left alone. SSL takes
    // ownership, so SSL_free releases it on every path below.
    if (STR_LEN(ca) > 0) {
        X509_STORE *store = store_from_pem(STR_BUF(ca), (int)STR_LEN(ca));
        if (store == NULL) {
            SSL_free(ssl);
            return int64_value(0);
        }
        if (SSL_set0_verify_cert_store(ssl, store) != 1) {
            X509_STORE_free(store);
            SSL_free(ssl);
            return int64_value(0);
        }
    }

    // A valid chain is not enough: the certificate must also be issued
    // for this host, or any CA-signed certificate would be accepted.
    // An IP literal is checked against the certificate's IP entries.
    X509_VERIFY_PARAM *param = SSL_get0_param(ssl);
    int is_ip = X509_VERIFY_PARAM_set1_ip_asc(param, hostname);
    if (!is_ip) {
        if (SSL_set1_host(ssl, hostname) != 1) {
            SSL_free(ssl);
            return int64_value(0);
        }

        // SNI: send the server name in the ClientHello so a CDN returns
        // the right certificate. RFC 6066 forbids IP literals in SNI.
        SSL_set_tlsext_host_name(ssl, hostname);
    }

    if (SSL_connect(ssl) != 1) {
        SSL_free(ssl);
        return int64_value(0);
    }

    if (verify && SSL_get_verify_result(ssl) != X509_V_OK) {
        SSL_free(ssl);
        return int64_value(0);
    }

    return int64_value((int64_t)(intptr_t)ssl);
}

// func tls_accept(cert int, fd int) int
//
// Run the server handshake over an established TCP fd against the
// certificate context `cert`. Returns a session handle for tls_read /
// tls_write / tls_conn_close, or 0 when the handshake fails. The fd is
// left open either way: the caller owns it and closes it.
static TValue _tls_accept(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    int64_t handle = kl_arg_int(0);
    int64_t fd = kl_arg_int(1);
    if (handle == 0) {
        return int64_value(0);
    }

    SSL_CTX *ctx = (SSL_CTX *)(intptr_t)handle;
    SSL *ssl = SSL_new(ctx);
    if (ssl == NULL) {
        return int64_value(0);
    }

    if (SSL_set_fd(ssl, (int)fd) != 1) {
        SSL_free(ssl);
        return int64_value(0);
    }

    if (SSL_accept(ssl) != 1) {
        SSL_free(ssl);
        return int64_value(0);
    }

    return int64_value((int64_t)(intptr_t)ssl);
}

// func tls_read(ssl int64, buf bytes) int
//
// Reads up to buf.len() bytes into buf. Returns the byte count on
// success, 0 on clean shutdown, -1 on error, -2 on read timeout. The
// timeout is whatever was set on the underlying fd with
// set_read_timeout.
static TValue _tls_read(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    int64_t handle = kl_arg_int(0);
    BytesObject *buf = kl_arg_obj_as(1, bytes_type);
    ASSERT(buf != NULL);

    // 0 is the handle of a closed connection.
    if (handle == 0) {
        return int64_value(-1);
    }

    SSL *ssl = (SSL *)(intptr_t)handle;
    size_t len = BYTES_SIZE(buf);
    if (len == 0) {
        return int64_value(0);
    }

    int n = SSL_read(ssl, BYTES_DATA(buf) + BYTES_OFFSET(buf), (int)len);
    if (n > 0) {
        return int64_value(n);
    }

    int err = SSL_get_error(ssl, n);
    if (err == SSL_ERROR_ZERO_RETURN) {
        return int64_value(0);
    }
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
        return int64_value(-2);
    }
    return int64_value(-1);
}

// func tls_write(ssl int64, data bytes) int
static TValue _tls_write(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    int64_t handle = kl_arg_int(0);
    BytesObject *data = kl_arg_obj_as(1, bytes_type);
    ASSERT(data != NULL);

    // 0 is the handle of a closed connection.
    if (handle == 0) {
        return int64_value(-1);
    }

    SSL *ssl = (SSL *)(intptr_t)handle;
    size_t len = BYTES_SIZE(data);
    if (len == 0) {
        return int64_value(0);
    }

    int n = SSL_write(ssl, BYTES_DATA(data) + BYTES_OFFSET(data), (int)len);
    if (n > 0) {
        return int64_value(n);
    }

    int err = SSL_get_error(ssl, n);
    if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ) {
        return int64_value(-2);
    }
    return int64_value(-1);
}

// func tls_conn_close(ssl int)
static TValue _tls_conn_close(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t handle = kl_arg_int(0);
    if (handle == 0) {
        return no_value;
    }

    SSL *ssl = (SSL *)(intptr_t)handle;
    SSL_shutdown(ssl);
    SSL_free(ssl);
    return no_value;
}

void tls_conn_register(NativeLib *lib)
{
    kl_reg_func(lib, "tls_conn_new", _tls_conn_new);
    kl_reg_func(lib, "tls_accept", _tls_accept);
    kl_reg_func(lib, "tls_read", _tls_read);
    kl_reg_func(lib, "tls_write", _tls_write);
    kl_reg_func(lib, "tls_conn_close", _tls_conn_close);
}
