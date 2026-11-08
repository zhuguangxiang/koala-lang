/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

/*
 * A thin layer over the vendored SQLite (native/sqlite3, see its VERSION
 * file).
 *
 * A connection and a statement are handles: the address of a `sqlite3`
 * and of a `Statement`, 0 when there is none. The functions keep SQLite's
 * names (`sqlite3_bind_int64` becomes `stmt_bind_int64`) and return its
 * result codes; the Koala side turns them into the `sqlite3` module's API.
 *
 * A value goes in and out through one function per SQLite storage class,
 * as in the C API. The Koala side picks the function from the value's
 * type, or from `stmt_column_type`.
 */

#include <stdlib.h>
#include <string.h>
#include "bytesobj.h"
#include "sqlite3.h"

typedef struct _Statement {
    sqlite3_stmt *stmt;
    // Something other than spaces and comments follows the statement.
    int has_tail;
    // INSERT, UPDATE, DELETE or REPLACE: the statements that open a
    // transaction under the legacy isolation levels, as in CPython.
    int is_dml;
} Statement;

// Skip spaces and SQL comments, as CPython's `lstrip_sql`.
static const char *skip_blank(const char *s, const char *end)
{
    while (s < end) {
        if (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\f') {
            s++;
        } else if (s + 1 < end && s[0] == '-' && s[1] == '-') {
            while (s < end && *s != '\n') s++;
        } else if (s + 1 < end && s[0] == '/' && s[1] == '*') {
            s += 2;
            while (s + 1 < end && !(s[0] == '*' && s[1] == '/')) s++;
            if (s + 1 >= end) return end;
            s += 2;
        } else {
            break;
        }
    }
    return s;
}

static int starts_with_word(const char *s, const char *end, const char *word)
{
    size_t n = strlen(word);
    if ((size_t)(end - s) < n) return 0;
    if (sqlite3_strnicmp(s, word, (int)n) != 0) return 0;
    return 1;
}

static int is_dml_sql(const char *s, const char *end)
{
    s = skip_blank(s, end);
    if (starts_with_word(s, end, "insert")) return 1;
    if (starts_with_word(s, end, "update")) return 1;
    if (starts_with_word(s, end, "delete")) return 1;
    if (starts_with_word(s, end, "replace")) return 1;
    return 0;
}

#define DB_ARG(i)   ((sqlite3 *)(intptr_t)kl_arg_int(i))
#define STMT_ARG(i) ((Statement *)(intptr_t)kl_arg_int(i))

// func sqlite_libversion() str
static TValue _sqlite_libversion(TValue *self, TValue *args, int nargs)
{
    return kl_val_str((char *)sqlite3_libversion());
}

// func sqlite_errstr(code int) str
//
// The English text of a result code.
static TValue _sqlite_errstr(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    int code = (int)kl_arg_int(0);
    return kl_val_str((char *)sqlite3_errstr(code));
}

// func db_open(path str, flags int, timeout_ms int) int
//
// Open a connection with `sqlite3_open_v2` and set its busy timeout.
// Extended result codes are turned on. Returns the handle, or 0 when the
// database cannot be opened.
static TValue _db_open(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    Object *path = kl_arg_strobj(0);
    int flags = (int)kl_arg_int(1);
    int timeout_ms = (int)kl_arg_int(2);

    // SQLite wants a NUL-terminated name.
    char *name = strndup(STR_BUF(path), STR_LEN(path));
    if (name == NULL) {
        return int64_value(0);
    }

    sqlite3 *db = NULL;
    int rc = sqlite3_open_v2(name, &db, flags, NULL);
    free(name);
    if (rc != SQLITE_OK) {
        // A handle is usually returned even on failure; it must be closed.
        sqlite3_close_v2(db);
        return int64_value(0);
    }

    sqlite3_extended_result_codes(db, 1);
    sqlite3_busy_timeout(db, timeout_ms);
    return int64_value((intptr_t)db);
}

// func db_close(db int) int
//
// `sqlite3_close_v2`: the connection goes away once its statements are
// finalized.
static TValue _db_close(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return int64_value(sqlite3_close_v2(DB_ARG(0)));
}

// func db_exec(db int, sql str) int
//
// Run every statement of `sql`, with no parameters and no rows returned.
static TValue _db_exec(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    sqlite3 *db = DB_ARG(0);
    Object *sql = kl_arg_strobj(1);

    char *text = strndup(STR_BUF(sql), STR_LEN(sql));
    if (text == NULL) {
        return int64_value(SQLITE_NOMEM);
    }
    int rc = sqlite3_exec(db, text, NULL, NULL, NULL);
    free(text);
    return int64_value(rc);
}

// func db_errcode(db int) int
//
// The extended result code of the last failed call on `db`.
static TValue _db_errcode(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return int64_value(sqlite3_extended_errcode(DB_ARG(0)));
}

// func db_errmsg(db int) str
static TValue _db_errmsg(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return kl_val_str((char *)sqlite3_errmsg(DB_ARG(0)));
}

// func db_changes(db int) int
//
// Rows changed by the last INSERT, UPDATE or DELETE.
static TValue _db_changes(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return int64_value(sqlite3_changes64(DB_ARG(0)));
}

// func db_total_changes(db int) int
static TValue _db_total_changes(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return int64_value(sqlite3_total_changes64(DB_ARG(0)));
}

// func db_last_insert_rowid(db int) int
static TValue _db_last_insert_rowid(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return int64_value(sqlite3_last_insert_rowid(DB_ARG(0)));
}

// func db_autocommit(db int) bool
//
// False while a transaction is open.
static TValue _db_autocommit(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return bool_value(sqlite3_get_autocommit(DB_ARG(0)) != 0);
}

// func stmt_prepare(db int, sql str) int
//
// Compile the first statement of `sql`. Returns the handle, 0 when `sql`
// holds only spaces and comments, -1 on an error (see db_errcode).
static TValue _stmt_prepare(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    sqlite3 *db = DB_ARG(0);
    Object *sql = kl_arg_strobj(1);
    const char *start = STR_BUF(sql);
    const char *end = start + STR_LEN(sql);

    sqlite3_stmt *stmt = NULL;
    const char *tail = NULL;
    int rc = sqlite3_prepare_v2(db, start, (int)(end - start), &stmt, &tail);
    if (rc != SQLITE_OK) {
        return int64_value(-1);
    }
    if (stmt == NULL) {
        return int64_value(0);
    }

    Statement *st = calloc(1, sizeof(Statement));
    if (st == NULL) {
        sqlite3_finalize(stmt);
        return int64_value(-1);
    }
    st->stmt = stmt;
    st->has_tail = skip_blank(tail, end) < end;
    st->is_dml = is_dml_sql(start, end);
    return int64_value((intptr_t)st);
}

// func stmt_finalize(st int)
static TValue _stmt_finalize(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    Statement *st = STMT_ARG(0);
    sqlite3_finalize(st->stmt);
    free(st);
    return no_value;
}

// func stmt_has_tail(st int) bool
//
// More than one statement was given; CPython refuses to run it.
static TValue _stmt_has_tail(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return bool_value(STMT_ARG(0)->has_tail);
}

// func stmt_is_dml(st int) bool
static TValue _stmt_is_dml(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return bool_value(STMT_ARG(0)->is_dml);
}

// func stmt_readonly(st int) bool
static TValue _stmt_readonly(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return bool_value(sqlite3_stmt_readonly(STMT_ARG(0)->stmt) != 0);
}

// func stmt_reset(st int) int
//
// Rewind the statement so it can run again; the bindings are kept.
static TValue _stmt_reset(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return int64_value(sqlite3_reset(STMT_ARG(0)->stmt));
}

// func stmt_clear_bindings(st int)
static TValue _stmt_clear_bindings(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    sqlite3_clear_bindings(STMT_ARG(0)->stmt);
    return no_value;
}

// func stmt_bind_parameter_count(st int) int
static TValue _stmt_bind_parameter_count(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return int64_value(sqlite3_bind_parameter_count(STMT_ARG(0)->stmt));
}

// func stmt_bind_parameter_name(st int, i int) str
//
// The name of parameter `i` (from 1) with its prefix, as ":id"; "" for a
// nameless `?`.
static TValue _stmt_bind_parameter_name(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    Statement *st = STMT_ARG(0);
    int i = (int)kl_arg_int(1);
    const char *name = sqlite3_bind_parameter_name(st->stmt, i);
    if (name == NULL) {
        return kl_val_nstr("", 0);
    }
    return kl_val_str((char *)name);
}

// func stmt_bind_null(st int, i int) int
static TValue _stmt_bind_null(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    Statement *st = STMT_ARG(0);
    int i = (int)kl_arg_int(1);
    return int64_value(sqlite3_bind_null(st->stmt, i));
}

// func stmt_bind_int64(st int, i int, value int) int
static TValue _stmt_bind_int64(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    Statement *st = STMT_ARG(0);
    int i = (int)kl_arg_int(1);
    int64_t value = kl_arg_int(2);
    return int64_value(sqlite3_bind_int64(st->stmt, i, value));
}

// func stmt_bind_double(st int, i int, value float) int
static TValue _stmt_bind_double(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    Statement *st = STMT_ARG(0);
    int i = (int)kl_arg_int(1);
    double value = kl_arg_float64(2);
    return int64_value(sqlite3_bind_double(st->stmt, i, value));
}

// func stmt_bind_text(st int, i int, value str) int
static TValue _stmt_bind_text(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    Statement *st = STMT_ARG(0);
    int i = (int)kl_arg_int(1);
    Object *value = kl_arg_strobj(2);
    int rc = sqlite3_bind_text64(st->stmt, i, STR_BUF(value), STR_LEN(value), SQLITE_TRANSIENT,
                                 SQLITE_UTF8);
    return int64_value(rc);
}

// func stmt_bind_blob(st int, i int, value bytes) int
static TValue _stmt_bind_blob(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 3);
    Statement *st = STMT_ARG(0);
    int i = (int)kl_arg_int(1);
    BytesObject *value = kl_arg_obj_as(2, bytes_type);
    const uint8_t *data = BYTES_DATA(value) + BYTES_OFFSET(value);
    int rc = sqlite3_bind_blob64(st->stmt, i, data, BYTES_SIZE(value), SQLITE_TRANSIENT);
    return int64_value(rc);
}

// func stmt_step(st int) int
//
// SQLITE_ROW when a row is ready, SQLITE_DONE at the end, otherwise an
// error code.
static TValue _stmt_step(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return int64_value(sqlite3_step(STMT_ARG(0)->stmt));
}

// func stmt_column_count(st int) int
static TValue _stmt_column_count(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 1);
    return int64_value(sqlite3_column_count(STMT_ARG(0)->stmt));
}

// func stmt_column_name(st int, i int) str
static TValue _stmt_column_name(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    Statement *st = STMT_ARG(0);
    int i = (int)kl_arg_int(1);
    const char *name = sqlite3_column_name(st->stmt, i);
    if (name == NULL) {
        return kl_val_nstr("", 0);
    }
    return kl_val_str((char *)name);
}

// func stmt_column_type(st int, i int) int
//
// The storage class of column `i` in the current row: SQLITE_INTEGER,
// SQLITE_FLOAT, SQLITE_TEXT, SQLITE_BLOB or SQLITE_NULL.
static TValue _stmt_column_type(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    Statement *st = STMT_ARG(0);
    int i = (int)kl_arg_int(1);
    return int64_value(sqlite3_column_type(st->stmt, i));
}

// func stmt_column_int64(st int, i int) int
static TValue _stmt_column_int64(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    Statement *st = STMT_ARG(0);
    int i = (int)kl_arg_int(1);
    return int64_value(sqlite3_column_int64(st->stmt, i));
}

// func stmt_column_double(st int, i int) float
static TValue _stmt_column_double(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    Statement *st = STMT_ARG(0);
    int i = (int)kl_arg_int(1);
    return float64_value(sqlite3_column_double(st->stmt, i));
}

// func stmt_column_text(st int, i int) str
static TValue _stmt_column_text(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    Statement *st = STMT_ARG(0);
    int i = (int)kl_arg_int(1);
    const unsigned char *text = sqlite3_column_text(st->stmt, i);
    int n = sqlite3_column_bytes(st->stmt, i);
    if (text == NULL) {
        return kl_val_nstr("", 0);
    }
    return kl_val_nstr((char *)text, (size_t)n);
}

// func stmt_column_blob(st int, i int) bytes
static TValue _stmt_column_blob(TValue *self, TValue *args, int nargs)
{
    ASSERT(nargs == 2);
    Statement *st = STMT_ARG(0);
    int i = (int)kl_arg_int(1);
    const void *data = sqlite3_column_blob(st->stmt, i);
    int n = sqlite3_column_bytes(st->stmt, i);
    if (data == NULL) {
        n = 0;
    }
    Object *blob = kl_bytes_from_data((uint8_t *)data, (uint32_t)n);
    return obj_value(blob);
}

/// Entry point called by the runtime after `dlopen("libkoala_sqlite3.so")`.
void sqlite3_native_lib_init(NativeLib *lib)
{
    kl_reg_func(lib, "sqlite_libversion", _sqlite_libversion);
    kl_reg_func(lib, "sqlite_errstr", _sqlite_errstr);
    kl_reg_func(lib, "db_open", _db_open);
    kl_reg_func(lib, "db_close", _db_close);
    kl_reg_func(lib, "db_exec", _db_exec);
    kl_reg_func(lib, "db_errcode", _db_errcode);
    kl_reg_func(lib, "db_errmsg", _db_errmsg);
    kl_reg_func(lib, "db_changes", _db_changes);
    kl_reg_func(lib, "db_total_changes", _db_total_changes);
    kl_reg_func(lib, "db_last_insert_rowid", _db_last_insert_rowid);
    kl_reg_func(lib, "db_autocommit", _db_autocommit);
    kl_reg_func(lib, "stmt_prepare", _stmt_prepare);
    kl_reg_func(lib, "stmt_finalize", _stmt_finalize);
    kl_reg_func(lib, "stmt_has_tail", _stmt_has_tail);
    kl_reg_func(lib, "stmt_is_dml", _stmt_is_dml);
    kl_reg_func(lib, "stmt_readonly", _stmt_readonly);
    kl_reg_func(lib, "stmt_reset", _stmt_reset);
    kl_reg_func(lib, "stmt_clear_bindings", _stmt_clear_bindings);
    kl_reg_func(lib, "stmt_bind_parameter_count", _stmt_bind_parameter_count);
    kl_reg_func(lib, "stmt_bind_parameter_name", _stmt_bind_parameter_name);
    kl_reg_func(lib, "stmt_bind_null", _stmt_bind_null);
    kl_reg_func(lib, "stmt_bind_int64", _stmt_bind_int64);
    kl_reg_func(lib, "stmt_bind_double", _stmt_bind_double);
    kl_reg_func(lib, "stmt_bind_text", _stmt_bind_text);
    kl_reg_func(lib, "stmt_bind_blob", _stmt_bind_blob);
    kl_reg_func(lib, "stmt_step", _stmt_step);
    kl_reg_func(lib, "stmt_column_count", _stmt_column_count);
    kl_reg_func(lib, "stmt_column_name", _stmt_column_name);
    kl_reg_func(lib, "stmt_column_type", _stmt_column_type);
    kl_reg_func(lib, "stmt_column_int64", _stmt_column_int64);
    kl_reg_func(lib, "stmt_column_double", _stmt_column_double);
    kl_reg_func(lib, "stmt_column_text", _stmt_column_text);
    kl_reg_func(lib, "stmt_column_blob", _stmt_column_blob);
}
