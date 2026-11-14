/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * Certificate management for a TLS server: load a certificate chain and
 * its private key from PEM, generate a self-signed pair, and export the
 * public certificate.
 *
 * A Certificate owns one server SSL_CTX, freed explicitly: a process may
 * serve several names, each with its own key. This is unlike the single
 * process-wide client CTX in conn.c.
 *
 * The handshake itself is a connection operation and lives in conn.c
 * (tls_accept), next to its client counterpart tls_conn_new.
 */

#include <arpa/inet.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <string.h>
#include "object.h"

// Koala's str has no NUL terminator, so anything handed to an OpenSSL
// API that takes a C string is copied into a bounded buffer first.
#define NAME_MAX_LEN 1024

static SSL_CTX *new_server_ctx(void)
{
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (ctx == NULL) {
        return NULL;
    }
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

    // Many clients close the TCP connection without sending
    // close_notify. Treat that as EOF, not as an error.
    SSL_CTX_set_options(ctx, SSL_OP_IGNORE_UNEXPECTED_EOF);
    return ctx;
}

// Install a PEM certificate chain and its private key into `ctx`. The
// first certificate is the leaf; any that follow are the chain, in
// order. Returns 1 on success, 0 on any failure.
static int install_pem(SSL_CTX *ctx, const char *cert, int cert_len,
                       const char *key, int key_len)
{
    BIO *bio = BIO_new_mem_buf(cert, cert_len);
    if (bio == NULL) {
        return 0;
    }

    X509 *leaf = PEM_read_bio_X509(bio, NULL, NULL, NULL);
    if (leaf == NULL) {
        BIO_free(bio);
        return 0;
    }
    int ok = SSL_CTX_use_certificate(ctx, leaf);
    X509_free(leaf);

    while (ok) {
        X509 *c = PEM_read_bio_X509(bio, NULL, NULL, NULL);
        if (c == NULL) {
            break;
        }
        ok = SSL_CTX_add1_chain_cert(ctx, c);
        X509_free(c);
    }
    BIO_free(bio);

    // The last PEM_read_bio_X509 failed because the input ran out, not
    // because anything is wrong; drop that error so it is not reported
    // as the reason for a later failure.
    ERR_clear_error();
    if (!ok) {
        return 0;
    }

    BIO *kb = BIO_new_mem_buf(key, key_len);
    if (kb == NULL) {
        return 0;
    }
    EVP_PKEY *pk = PEM_read_bio_PrivateKey(kb, NULL, NULL, NULL);
    BIO_free(kb);
    if (pk == NULL) {
        return 0;
    }

    ok = SSL_CTX_use_PrivateKey(ctx, pk);
    EVP_PKEY_free(pk);
    if (!ok) {
        return 0;
    }

    // A key that does not match the certificate would otherwise fail
    // every handshake with no clue as to why.
    return SSL_CTX_check_private_key(ctx) == 1;
}

// func tls_cert_from_pem(cert str, key str) int
//
// A server context from PEM text. Returns a handle, or 0 when either
// PEM is malformed or the key does not match the certificate.
static TValue _tls_cert_from_pem(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    Object *cert = kl_arg_strobj(0);
    Object *key = kl_arg_strobj(1);

    SSL_CTX *ctx = new_server_ctx();
    if (ctx == NULL) {
        return int64_value(0);
    }

    int ok = install_pem(ctx, STR_BUF(cert), (int)STR_LEN(cert),
                         STR_BUF(key), (int)STR_LEN(key));
    if (!ok) {
        SSL_CTX_free(ctx);
        return int64_value(0);
    }
    return int64_value((int64_t)(intptr_t)ctx);
}

// Append one "DNS:name" or "IP:addr" entry to the SAN list, choosing by
// whether the name parses as an IP literal.
static void add_san(char *out, size_t cap, const char *name, size_t len, int *first)
{
    char entry[NAME_MAX_LEN + 8];
    struct in_addr v4;
    struct in6_addr v6;

    // inet_pton needs a NUL-terminated string.
    char buf[NAME_MAX_LEN];
    if (len == 0 || len >= sizeof(buf)) {
        return;
    }
    memcpy(buf, name, len);
    buf[len] = '\0';

    const char *kind = "DNS";
    if (inet_pton(AF_INET, buf, &v4) == 1 || inet_pton(AF_INET6, buf, &v6) == 1) {
        kind = "IP";
    }

    size_t used = strlen(out);
    int n = snprintf(entry, sizeof(entry), "%s%s:%s", *first ? "" : ",", kind, buf);
    if (n < 0 || (size_t)n >= sizeof(entry) || used + (size_t)n >= cap) {
        return;
    }
    memcpy(out + used, entry, (size_t)n);
    out[used + (size_t)n] = '\0';
    *first = 0;
}

// func tls_cert_self_signed(names str, days int) int
//
// A server context with a freshly generated self-signed certificate:
// RSA 2048, SHA-256, valid for `days`, with every comma-separated name
// in the subjectAltName. The first name is also the common name.
//
// The certificate is its own CA, so a client can trust it directly.
// Returns a handle, or 0 on failure.
static TValue _tls_cert_self_signed(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    Object *names = kl_arg_strobj(0);
    int64_t days = kl_arg_int(1);
    if (days <= 0) {
        return int64_value(0);
    }

    size_t nlen = STR_LEN(names);
    if (nlen == 0 || nlen >= NAME_MAX_LEN) {
        return int64_value(0);
    }
    char list[NAME_MAX_LEN];
    memcpy(list, STR_BUF(names), nlen);
    list[nlen] = '\0';

    EVP_PKEY *pk = EVP_RSA_gen(2048);
    if (pk == NULL) {
        return int64_value(0);
    }

    X509 *x = X509_new();
    if (x == NULL) {
        EVP_PKEY_free(pk);
        return int64_value(0);
    }

    X509_set_version(x, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(x), 1);
    X509_gmtime_adj(X509_getm_notBefore(x), 0);
    X509_gmtime_adj(X509_getm_notAfter(x), (long)days * 86400);
    X509_set_pubkey(x, pk);

    // The common name is the first entry; the SAN list carries them all.
    char san[NAME_MAX_LEN * 2];
    san[0] = '\0';
    int first = 1;
    char cn[NAME_MAX_LEN] = "";

    char *save = NULL;
    for (char *tok = strtok_r(list, ",", &save); tok != NULL;
         tok = strtok_r(NULL, ",", &save)) {
        while (*tok == ' ') {
            tok++;
        }
        size_t tlen = strlen(tok);
        while (tlen > 0 && tok[tlen - 1] == ' ') {
            tlen--;
        }
        if (tlen == 0) {
            continue;
        }
        if (first) {
            memcpy(cn, tok, tlen);
            cn[tlen] = '\0';
        }
        add_san(san, sizeof(san), tok, tlen, &first);
    }

    if (san[0] == '\0') {
        X509_free(x);
        EVP_PKEY_free(pk);
        return int64_value(0);
    }

    X509_NAME *name = X509_get_subject_name(x);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                               (const unsigned char *)cn, -1, -1, 0);
    X509_set_issuer_name(x, name);

    X509_EXTENSION *ext = X509V3_EXT_conf_nid(NULL, NULL, NID_subject_alt_name, san);
    if (ext == NULL) {
        X509_free(x);
        EVP_PKEY_free(pk);
        return int64_value(0);
    }
    X509_add_ext(x, ext, -1);
    X509_EXTENSION_free(ext);

    // CA:TRUE lets a client use this same certificate as its trust
    // anchor, which is what makes a self-signed server testable with
    // verification on.
    ext = X509V3_EXT_conf_nid(NULL, NULL, NID_basic_constraints, "critical,CA:TRUE");
    if (ext != NULL) {
        X509_add_ext(x, ext, -1);
        X509_EXTENSION_free(ext);
    }

    if (X509_sign(x, pk, EVP_sha256()) == 0) {
        X509_free(x);
        EVP_PKEY_free(pk);
        return int64_value(0);
    }

    SSL_CTX *ctx = new_server_ctx();
    if (ctx == NULL) {
        X509_free(x);
        EVP_PKEY_free(pk);
        return int64_value(0);
    }

    int ok = SSL_CTX_use_certificate(ctx, x) == 1 &&
             SSL_CTX_use_PrivateKey(ctx, pk) == 1;
    X509_free(x);
    EVP_PKEY_free(pk);
    if (!ok) {
        SSL_CTX_free(ctx);
        return int64_value(0);
    }
    return int64_value((int64_t)(intptr_t)ctx);
}

// func tls_cert_pem(cert int) str
//
// The leaf certificate as PEM text, so a caller can hand it to clients
// as a trust anchor. "" when the context is closed or has no
// certificate.
static TValue _tls_cert_pem(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t handle = kl_arg_int(0);
    if (handle == 0) {
        return kl_val_nstr("", 0);
    }

    SSL_CTX *ctx = (SSL_CTX *)(intptr_t)handle;
    X509 *x = SSL_CTX_get0_certificate(ctx);
    if (x == NULL) {
        return kl_val_nstr("", 0);
    }

    BIO *bio = BIO_new(BIO_s_mem());
    if (bio == NULL) {
        return kl_val_nstr("", 0);
    }
    if (PEM_write_bio_X509(bio, x) != 1) {
        BIO_free(bio);
        return kl_val_nstr("", 0);
    }

    char *data = NULL;
    long len = BIO_get_mem_data(bio, &data);
    TValue pem = kl_val_nstr(data, (int)len);
    BIO_free(bio);
    return pem;
}

// func tls_cert_free(cert int)
static TValue _tls_cert_free(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t handle = kl_arg_int(0);
    if (handle == 0) {
        return no_value;
    }
    SSL_CTX_free((SSL_CTX *)(intptr_t)handle);
    return no_value;
}

void tls_cert_register(NativeLib *lib)
{
    kl_reg_func(lib, "tls_cert_from_pem", _tls_cert_from_pem);
    kl_reg_func(lib, "tls_cert_self_signed", _tls_cert_self_signed);
    kl_reg_func(lib, "tls_cert_pem", _tls_cert_pem);
    kl_reg_func(lib, "tls_cert_free", _tls_cert_free);
}