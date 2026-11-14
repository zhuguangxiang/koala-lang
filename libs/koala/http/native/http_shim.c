/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * HTTP/1.x message parser over the vendored llhttp (native/llhttp, see
 * its VERSION file).
 *
 * llhttp reports a message through C callbacks, and native code cannot
 * call back into Koala. So the callbacks only collect into an
 * HttpParser, and the Koala side pulls the results through the
 * accessors below.
 *
 * The parser pauses at two points so the Koala side sees them as
 * boundaries of a feed:
 *   - after the head, so the caller learns where the body starts;
 *   - after the message, so bytes of a following message are not
 *     consumed.
 * `http_parser_feed` returns how many bytes it consumed; the caller
 * feeds the rest again.
 */

#include <stdlib.h>
#include "buffer.h"
#include "bytesobj.h"
#include "excobj.h"
#include "llhttp.h"

// Progress of the current message, returned by http_parser_state.
#define STATE_HEAD     0
#define STATE_BODY     1
#define STATE_COMPLETE 2

typedef struct _HeaderPair {
    Buffer name;
    Buffer value;
} HeaderPair;

typedef struct _HttpParser {
    llhttp_t parser;
    int state;
    // Set by the caller for a response to HEAD: the head says how long
    // the body is, but no body follows.
    int skip_body;
    // Bytes consumed by earlier feeds of the current message.
    int64_t consumed;
    // Offset, from the start of the message, where the body starts.
    int64_t body_offset;
    Buffer reason;
    // The request target, as it appeared on the request line. Requests
    // only; a response has no target.
    Buffer url;
    Buffer field;
    Buffer value;
    HeaderPair *headers;
    int header_count;
    int header_cap;
    Buffer body;
} HttpParser;

static llhttp_settings_t g_settings;
static int g_settings_ready = 0;

static void clear_message(HttpParser *hp)
{
    hp->state = STATE_HEAD;
    hp->consumed = 0;
    hp->body_offset = 0;

    FINI_BUF(hp->reason);
    INIT_BUF(hp->reason);
    FINI_BUF(hp->url);
    INIT_BUF(hp->url);
    FINI_BUF(hp->field);
    INIT_BUF(hp->field);
    FINI_BUF(hp->value);
    INIT_BUF(hp->value);

    for (int i = 0; i < hp->header_count; i++) {
        FINI_BUF(hp->headers[i].name);
        FINI_BUF(hp->headers[i].value);
    }
    hp->header_count = 0;

    FINI_BUF(hp->body);
    INIT_BUF(hp->body);
}

static int on_status(llhttp_t *p, const char *at, size_t len)
{
    HttpParser *hp = p->data;
    buf_write_nstr(&hp->reason, at, (int)len);
    return HPE_OK;
}

// The request target. May arrive in pieces when the request line is
// split across feeds.
static int on_url(llhttp_t *p, const char *at, size_t len)
{
    HttpParser *hp = p->data;
    buf_write_nstr(&hp->url, at, (int)len);
    return HPE_OK;
}

static int on_header_field(llhttp_t *p, const char *at, size_t len)
{
    HttpParser *hp = p->data;
    buf_write_nstr(&hp->field, at, (int)len);
    return HPE_OK;
}

static int on_header_value(llhttp_t *p, const char *at, size_t len)
{
    HttpParser *hp = p->data;
    buf_write_nstr(&hp->value, at, (int)len);
    return HPE_OK;
}

// A field and its value may each arrive in several pieces when the
// head is split across feeds; the pair is complete only here.
static int on_header_value_complete(llhttp_t *p)
{
    HttpParser *hp = p->data;

    if (hp->header_count == hp->header_cap) {
        int cap = hp->header_cap * 2;
        if (cap == 0) {
            cap = 8;
        }
        HeaderPair *grown = realloc(hp->headers, cap * sizeof(HeaderPair));
        if (grown == NULL) {
            return HPE_USER;
        }
        hp->headers = grown;
        hp->header_cap = cap;
    }

    // The pair takes over both buffers.
    HeaderPair *pair = &hp->headers[hp->header_count];
    pair->name = hp->field;
    pair->value = hp->value;
    hp->header_count++;

    INIT_BUF(hp->field);
    INIT_BUF(hp->value);
    return HPE_OK;
}

// Returning 1 tells llhttp the message has no body. Otherwise pause,
// so the feed ends exactly at the body start.
static int on_headers_complete(llhttp_t *p)
{
    HttpParser *hp = p->data;
    hp->state = STATE_BODY;
    if (hp->skip_body) {
        return 1;
    }
    return HPE_PAUSED;
}

static int on_body(llhttp_t *p, const char *at, size_t len)
{
    HttpParser *hp = p->data;
    buf_write_nstr(&hp->body, at, (int)len);
    return HPE_OK;
}

// Pause, so bytes of a pipelined next message stay unconsumed.
static int on_message_complete(llhttp_t *p)
{
    HttpParser *hp = p->data;
    hp->state = STATE_COMPLETE;
    return HPE_PAUSED;
}

static llhttp_settings_t *get_settings(void)
{
    if (g_settings_ready) {
        return &g_settings;
    }
    llhttp_settings_init(&g_settings);
    g_settings.on_status = on_status;
    g_settings.on_url = on_url;
    g_settings.on_header_field = on_header_field;
    g_settings.on_header_value = on_header_value;
    g_settings.on_header_value_complete = on_header_value_complete;
    g_settings.on_headers_complete = on_headers_complete;
    g_settings.on_body = on_body;
    g_settings.on_message_complete = on_message_complete;
    g_settings_ready = 1;
    return &g_settings;
}

// func http_parser_new(kind int) int
//
// `kind` is llhttp's type: 1 parses requests, 2 responses. Returns a
// handle, or 0 when `kind` is neither or allocation fails.
static TValue _http_parser_new(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int64_t kind = kl_arg_int(0);
    if (kind != HTTP_REQUEST && kind != HTTP_RESPONSE) {
        return int64_value(0);
    }

    HttpParser *hp = calloc(1, sizeof(HttpParser));
    if (hp == NULL) {
        return int64_value(0);
    }

    llhttp_init(&hp->parser, (llhttp_type_t)kind, get_settings());
    hp->parser.data = hp;
    return int64_value((intptr_t)hp);
}

// func http_parser_skip_body(p int, skip bool)
//
// Mark the coming response as one to a HEAD request. Set before the
// head is fed.
static TValue _http_parser_skip_body(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    hp->skip_body = kl_arg_bool(1);
    return no_value;
}

// func http_parser_feed(p int, data bytes) int
//
// Parse `data`. Returns the number of bytes consumed, which is less
// than `data.len()` when the parser stopped at the end of the head or
// of the message; feed the rest again. -1 on a parse error.
static TValue _http_parser_feed(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    BytesObject *data = kl_arg_obj_as(1, bytes_type);
    ASSERT(data != NULL);

    // A completed message stays until http_parser_reset.
    if (hp->state == STATE_COMPLETE) {
        return int64_value(0);
    }

    if (llhttp_get_errno(&hp->parser) == HPE_PAUSED) {
        llhttp_resume(&hp->parser);
    }

    const char *start = (const char *)BYTES_DATA(data) + BYTES_OFFSET(data);
    size_t len = BYTES_SIZE(data);
    int before = hp->state;

    llhttp_errno_t err = llhttp_execute(&hp->parser, start, len);

    int64_t n = (int64_t)len;
    if (err == HPE_PAUSED) {
        n = llhttp_get_error_pos(&hp->parser) - start;
    } else if (err != HPE_OK) {
        return int64_value(-1);
    }

    if (before == STATE_HEAD && hp->state != STATE_HEAD) {
        // With skip_body the message may complete in the same call;
        // the head then ends where the message ends.
        hp->body_offset = hp->consumed + n;
    }
    hp->consumed += n;
    return int64_value(n);
}

// func http_parser_finish(p int) int
//
// Tell the parser the peer closed the connection. A body framed by
// connection close completes here. 0 on success, -1 when the message
// was cut short.
static TValue _http_parser_finish(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    if (hp->state == STATE_COMPLETE) {
        return int64_value(0);
    }

    if (llhttp_get_errno(&hp->parser) == HPE_PAUSED) {
        llhttp_resume(&hp->parser);
    }

    llhttp_errno_t err = llhttp_finish(&hp->parser);
    if (err != HPE_OK && err != HPE_PAUSED) {
        return int64_value(-1);
    }
    if (hp->state != STATE_COMPLETE) {
        return int64_value(-1);
    }
    return int64_value(0);
}

// func http_parser_state(p int) int
//
// 0 while in the head, 1 once the head is parsed, 2 once the whole
// message is parsed.
static TValue _http_parser_state(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    return int64_value(hp->state);
}

// func http_parser_status(p int) int
static TValue _http_parser_status(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    return int64_value(llhttp_get_status_code(&hp->parser));
}

// func http_parser_reason(p int) str
static TValue _http_parser_reason(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    return kl_val_nstr(BUF_STR(hp->reason), BUF_LEN(hp->reason));
}

// func http_parser_method(p int) str
//
// The request method, uppercased as llhttp normalizes it. A request
// parser sets it from the request line; a response parser leaves it at
// 0, so only call this on a request parser.
static TValue _http_parser_method(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    llhttp_method_t method = (llhttp_method_t)llhttp_get_method(&hp->parser);
    return kl_val_str((char *)llhttp_method_name(method));
}

// func http_parser_url(p int) str
//
// The request target exactly as it arrived: origin-form "/path?q=1",
// absolute-form "http://host/path", or "*". Requests only.
static TValue _http_parser_url(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    return kl_val_nstr(BUF_STR(hp->url), BUF_LEN(hp->url));
}

// func http_parser_header_count(p int) int
static TValue _http_parser_header_count(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    return int64_value(hp->header_count);
}

// func http_parser_header_name(p int, i int) str
static TValue _http_parser_header_name(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    int64_t i = kl_arg_int(1);
    if (!(i >= 0 && i < hp->header_count)) {
        raise_exc_str("header index out of range");
        return error_value;
    }
    Buffer *name = &hp->headers[i].name;
    return kl_val_nstr(BUF_STR(*name), BUF_LEN(*name));
}

// func http_parser_header_value(p int, i int) str
static TValue _http_parser_header_value(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    int64_t i = kl_arg_int(1);
    if (!(i >= 0 && i < hp->header_count)) {
        raise_exc_str("header index out of range");
        return error_value;
    }
    Buffer *value = &hp->headers[i].value;
    return kl_val_nstr(BUF_STR(*value), BUF_LEN(*value));
}

// func http_parser_body_offset(p int) int
//
// Where the body starts, counted from the first byte of the message.
// Valid once the state is past the head.
static TValue _http_parser_body_offset(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    return int64_value(hp->body_offset);
}

// func http_parser_take_body(p int) bytes
//
// The body decoded so far, chunked framing removed. The parser keeps
// no copy, so a streaming caller can take the body piece by piece.
static TValue _http_parser_take_body(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    uint8_t *buf = (uint8_t *)BUF_STR(hp->body);
    Object *body = kl_bytes_from_data(buf, (uint32_t)BUF_LEN(hp->body));
    FINI_BUF(hp->body);
    INIT_BUF(hp->body);
    return obj_value(body);
}

// func http_parser_keep_alive(p int) bool
//
// Whether the connection can carry another message after this one.
static TValue _http_parser_keep_alive(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    return bool_value(llhttp_should_keep_alive(&hp->parser));
}

// func http_parser_error(p int) str
//
// llhttp's reason for the last parse error, or "" when there is none.
static TValue _http_parser_error(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    llhttp_errno_t err = llhttp_get_errno(&hp->parser);
    if (err == HPE_OK || err == HPE_PAUSED) {
        return kl_val_nstr("", 0);
    }
    const char *reason = llhttp_get_error_reason(&hp->parser);
    if (reason == NULL) {
        reason = llhttp_errno_name(err);
    }
    return kl_val_str((char *)reason);
}

// func http_parser_reset(p int)
//
// Drop the current message and get ready for the next one on the
// same connection. Clears skip_body.
static TValue _http_parser_reset(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    clear_message(hp);
    hp->skip_body = 0;
    llhttp_type_t kind = (llhttp_type_t)llhttp_get_type(&hp->parser);
    llhttp_init(&hp->parser, kind, get_settings());
    hp->parser.data = hp;
    return no_value;
}

// func http_parser_free(p int)
static TValue _http_parser_free(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    HttpParser *hp = (HttpParser *)(intptr_t)kl_arg_int(0);
    if (hp == NULL) {
        return no_value;
    }
    clear_message(hp);
    free(hp->headers);
    free(hp);
    return no_value;
}

void http_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "http_parser_new", _http_parser_new);
    kl_reg_func(lib, "http_parser_skip_body", _http_parser_skip_body);
    kl_reg_func(lib, "http_parser_feed", _http_parser_feed);
    kl_reg_func(lib, "http_parser_finish", _http_parser_finish);
    kl_reg_func(lib, "http_parser_state", _http_parser_state);
    kl_reg_func(lib, "http_parser_status", _http_parser_status);
    kl_reg_func(lib, "http_parser_reason", _http_parser_reason);
    kl_reg_func(lib, "http_parser_method", _http_parser_method);
    kl_reg_func(lib, "http_parser_url", _http_parser_url);
    kl_reg_func(lib, "http_parser_header_count", _http_parser_header_count);
    kl_reg_func(lib, "http_parser_header_name", _http_parser_header_name);
    kl_reg_func(lib, "http_parser_header_value", _http_parser_header_value);
    kl_reg_func(lib, "http_parser_body_offset", _http_parser_body_offset);
    kl_reg_func(lib, "http_parser_take_body", _http_parser_take_body);
    kl_reg_func(lib, "http_parser_keep_alive", _http_parser_keep_alive);
    kl_reg_func(lib, "http_parser_error", _http_parser_error);
    kl_reg_func(lib, "http_parser_reset", _http_parser_reset);
    kl_reg_func(lib, "http_parser_free", _http_parser_free);
}
