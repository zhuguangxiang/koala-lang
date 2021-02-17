/*----------------------------------------------------------------------------*\
|* This file is part of the koala project, under the MIT License.             *|
|* Copyright (c) 2021-2021 James <zhuguangxiang@gmail.com>                    *|
\*----------------------------------------------------------------------------*/

#ifndef _KOALA_KLVM_H_
#define _KOALA_KLVM_H_

#include "common.h"
#include "hashmap.h"
#include "list.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*\
|* builtin types                                                              *|
\*----------------------------------------------------------------------------*/

typedef enum klvm_type_kind {
    KLVM_TYPE_INT8 = 1,
    KLVM_TYPE_INT16,
    KLVM_TYPE_INT32,
    KLVM_TYPE_INT64,
    KLVM_TYPE_FLOAT32,
    KLVM_TYPE_FLOAT64,
    KLVM_TYPE_BOOL,
    KLVM_TYPE_CHAR,
    KLVM_TYPE_STRING,
    KLVM_TYPE_ANY,
    KLVM_TYPE_ARRAY,
    KLVM_TYPE_DICT,
    KLVM_TYPE_TUPLE,
    KLVM_TYPE_VALIST,
    KLVM_TYPE_CLASS,
    KLVM_TYPE_INTF,
    KLVM_TYPE_ENUM,
    KLVM_TYPE_CLOSURE,
    KLVM_TYPE_PROTO,
} klvm_type_kind_t;

typedef struct klvm_type {
    klvm_type_kind_t kind;
} klvm_type_t;

extern klvm_type_t klvm_type_int32;
extern klvm_type_t klvm_type_bool;
klvm_type_t *klvm_type_proto(klvm_type_t *ret, klvm_type_t **params);
vector_t *klvm_proto_params(klvm_type_t *ty);
klvm_type_t *klvm_proto_return(klvm_type_t *ty);
int klvm_type_check(klvm_type_t *ty1, klvm_type_t *ty2);
char *klvm_type_string(klvm_type_t *ty);

/*----------------------------------------------------------------------------*\
|* values: constant, variable, function, class, intf, enum, closure           *|
\*----------------------------------------------------------------------------*/

typedef enum klvm_value_kind {
    KLVM_VALUE_CONST = 1,
    KLVM_VALUE_VAR,
    KLVM_VALUE_FUNC,
    KLVM_VALUE_INST,
    KLVM_VALUE_CLASS,
    KLVM_VALUE_INTF,
    KLVM_VALUE_ENUM,
    KLVM_VALUE_CLOSURE,
} klvm_value_kind_t;

// clang-format off

#define KLVM_VALUE_HEAD \
    klvm_value_kind_t kind; klvm_type_t *type; char *name; int tag;

#define KLVM_INIT_VALUE_HEAD(val, _kind, _type, _name) \
    val->kind = _kind; val->type = _type; val->name = _name; val->tag = -1

// clang-format on

typedef struct klvm_value {
    KLVM_VALUE_HEAD
} klvm_value_t;

typedef struct klvm_const {
    KLVM_VALUE_HEAD
    union {
        int8_t i8val;
        int16_t i16val;
        int32_t i32val;
        int64_t i64val;
        float f32val;
        double f64val;
        void *obj;
    };
} klvm_const_t;

#define KLVM_ATTR_PUB   1
#define KLVM_ATTR_FINAL 2
#define KLVM_ATTR_LOCAL 4

typedef struct klvm_var {
    KLVM_VALUE_HEAD
    int attr;
} klvm_var_t;

typedef struct klvm_block {
    list_node_t bbnode;
    klvm_value_t *fn;
    const char *label;
    // 'start' and 'end' blocks are dummy block
    short dummy;
    short tag;
    int visited;
    int num_insts;
    list_t insts;
    list_t in_edges;
    list_t out_edges;
} klvm_block_t;

typedef struct klvm_edge {
    list_node_t link;
    klvm_block_t *src;
    klvm_block_t *dst;
    list_node_t in_link;
    list_node_t out_link;
} klvm_edge_t;

typedef struct klvm_builder {
    klvm_block_t *bb;
    klvm_value_t *rover;
} klvm_builder_t;

typedef struct klvm_func {
    KLVM_VALUE_HEAD
    int attr;
    int tag_index;
    int bb_tag_index;
    int num_params;
    int num_bbs;
    list_t bblist;
    list_t edgelist;
    vector_t locals;
    klvm_block_t sbb;
    klvm_block_t ebb;
} klvm_func_t;

typedef struct klvm_class {
    KLVM_VALUE_HEAD
    int attr;
    vector_t paratypes;
    struct klvm_class *base;
    vector_t intfs;
    vector_t functions;
    vector_t variables;
} klvm_class_t;

typedef struct klvm_intf {
    KLVM_VALUE_HEAD
    int attr;
    vector_t paratypes;
    vector_t functions;
} klvm_intf_t;

typedef struct klvm_enum {
    KLVM_VALUE_HEAD
    int attr;
    vector_t labels;
    vector_t functions;
} klvm_enum_t;

typedef struct klvm_module {
    const char *name;
    hashmap_t map;
    vector_t funcs;
    vector_t vars;
    klvm_value_t *_init_;
    list_t passes;
} klvm_module_t;

/* Create a new module */
klvm_module_t *klvm_create_module(char *name);

/* Destroy a module */
void klvm_destroy_module(klvm_module_t *m);

void klvm_print_module(klvm_module_t *m, FILE *fp);

/* Dump a module */
#define klvm_dump_module(m)       \
    klvm_print_module(m, stdout); \
    fflush(stdout)

/* Get '__init__' func */
klvm_value_t *klvm_init_func(klvm_module_t *m);

/* New constant */
klvm_value_t *klvm_const_int8(int8_t ival);
klvm_value_t *klvm_const_int16(int16_t ival);
klvm_value_t *klvm_const_int32(int32_t ival);
klvm_value_t *klvm_const_int64(int64_t ival);
klvm_value_t *klvm_const_float32(double fval);
klvm_value_t *klvm_const_float64(double fval);
klvm_value_t *klvm_const_bool(int8_t bval);

/* Get value type */
#define klvm_value_type(val) (val)->type

/* Set value name */
#define klvm_set_name(val, _name) (val)->name = _name

/* Add a variable to a module */
klvm_value_t *klvm_new_var(klvm_module_t *m, klvm_type_t *ty, char *name);

/* Add a function to a module */
klvm_value_t *klvm_new_func(klvm_module_t *m, klvm_type_t *ty, char *name);

/* Add a local variable */
klvm_value_t *klvm_new_local(klvm_value_t *fn, klvm_type_t *ty, char *name);

/* Get function param by index */
klvm_value_t *klvm_get_param(klvm_value_t *fn, int index);

/* Append a basic block to the end of a function */
klvm_block_t *klvm_append_block(klvm_value_t *fn, char *name);

/* Add a basic block after 'bb' */
klvm_block_t *klvm_add_block(klvm_block_t *bb, char *name);

/* Add a basic block before 'bb' */
klvm_block_t *kvlm_add_block_before(klvm_block_t *bb, char *name);

/* Delete a basic block */
void klvm_delete_block(klvm_block_t *bb);

/* Add an edge */
void klvm_link_edge(klvm_block_t *src, klvm_block_t *dst);

// clang-format off

/* Initial an instruction builder */
#define klvm_builder_init(bldr, _bb) \
    ({ (bldr)->bb = _bb; (bldr)->rover = NULL; })

// clang-format on

/* Set builder at end */
void klvm_builder_set_end(klvm_builder_t *bldr);
/* Set builder at head */
void klvm_builder_set_head(klvm_builder_t *bldr);
/* set builder at 'inst' */
void klvm_builder_set_point(klvm_builder_t *bldr, klvm_value_t *inst);
/* set builder before 'inst' */
void klvm_builder_set_before(klvm_builder_t *bldr, klvm_value_t *inst);

/*----------------------------------------------------------------------------*\
|* instructions                                                               *|
\*----------------------------------------------------------------------------*/

typedef enum klvm_inst_kind {
    KLVM_INST_INVALID,

    KLVM_INST_COPY,

    KLVM_INST_ADD,
    KLVM_INST_SUB,
    KLVM_INST_MUL,
    KLVM_INST_DIV,
    KLVM_INST_MOD,
    KLVM_INST_POW,

    KLVM_INST_CMP_EQ,
    KLVM_INST_CMP_NE,
    KLVM_INST_CMP_GT,
    KLVM_INST_CMP_GE,
    KLVM_INST_CMP_LT,
    KLVM_INST_CMP_LE,

    KLVM_INST_AND,
    KLVM_INST_OR,

    KLVM_INST_BIT_AND,
    KLVM_INST_BIT_OR,
    KLVM_INST_BIT_XOR,
    KLVM_INST_BIT_SHL,
    KLVM_INST_BIT_SHR,

    KLVM_INST_NEG,
    KLVM_INST_NOT,
    KLVM_INST_BIT_NOT,

    KLVM_INST_CALL,

    KLVM_INST_RET,
    KLVM_INST_RET_VOID,
    KLVM_INST_JMP,
    KLVM_INST_BRANCH,

    KLVM_INST_INDEX,
    KLVM_INST_NEW,
    KLVM_INST_FIELD,
} klvm_inst_kind_t;

// clang-format off

#define KLVM_INST_HEAD \
    KLVM_VALUE_HEAD list_node_t node; klvm_inst_kind_t op;

#define KLVM_INIT_INST_HEAD(inst, type, _op, _name) \
    KLVM_INIT_VALUE_HEAD(inst, KLVM_VALUE_INST, type, _name); \
    list_node_init(&inst->node); inst->op = _op

// clang-format on

typedef struct klvm_inst {
    KLVM_INST_HEAD
} klvm_inst_t;

typedef struct klvm_copy {
    KLVM_INST_HEAD
    klvm_value_t *lhs;
    klvm_value_t *rhs;
} klvm_copy_t;

typedef struct klvm_binary {
    KLVM_INST_HEAD
    klvm_value_t *lhs;
    klvm_value_t *rhs;
} klvm_binary_t;

typedef struct klvm_unary {
    KLVM_INST_HEAD
    klvm_value_t *val;
} klvm_unary_t;

typedef struct klvm_call {
    KLVM_INST_HEAD
    klvm_value_t *fn;
    vector_t args;
} klvm_call_t;

typedef struct klvm_jmp {
    KLVM_INST_HEAD
    klvm_block_t *dst;
} klvm_jmp_t;

typedef struct klvm_branch {
    KLVM_INST_HEAD
    klvm_value_t *cond;
    klvm_block_t *_then;
    klvm_block_t *_else;
} klvm_branch_t;

typedef struct klvm_ret {
    KLVM_INST_HEAD
    klvm_value_t *ret;
} klvm_ret_t;

void klvm_build_copy(
    klvm_builder_t *bldr, klvm_value_t *rhs, klvm_value_t *lhs);

klvm_value_t *klvm_build_binary(klvm_builder_t *bldr, klvm_inst_kind_t op,
    klvm_value_t *lhs, klvm_value_t *rhs, char *name);

#define klvm_build_add(bldr, lhs, rhs, name) \
    klvm_build_binary(bldr, KLVM_INST_ADD, lhs, rhs, name)

#define klvm_build_sub(bldr, lhs, rhs, name) \
    klvm_build_binary(bldr, KLVM_INST_SUB, lhs, rhs, name)

#define klvm_build_cmple(bldr, lhs, rhs, name) \
    klvm_build_binary(bldr, KLVM_INST_CMP_LE, lhs, rhs, name)

klvm_value_t *klvm_build_call(
    klvm_builder_t *bldr, klvm_value_t *fn, klvm_value_t **args, char *name);

void klvm_build_jmp(klvm_builder_t *bldr, klvm_block_t *dst);
void klvm_build_branch(klvm_builder_t *bldr, klvm_value_t *cond,
    klvm_block_t *_then, klvm_block_t *_else);

void klvm_build_ret(klvm_builder_t *bldr, klvm_value_t *v);
void klvm_build_ret_void(klvm_builder_t *bldr);

/*----------------------------------------------------------------------------*\
|* pass                                                                       *|
\*----------------------------------------------------------------------------*/

typedef void (*klvm_pass_run_t)(klvm_func_t *, void *);

typedef struct klvm_pass {
    klvm_pass_run_t run;
    void *data;
} klvm_pass_t;

void klvm_register_pass(klvm_module_t *m, klvm_pass_t *pass);
void klvm_run_passes(klvm_module_t *m);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_KLVM_H_ */
