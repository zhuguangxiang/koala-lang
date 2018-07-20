
#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#include "symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lineinfo {
  char *line;
  int row;
  int col;
} LineInfo;

/*-------------------------------------------------------------------------*/

typedef enum unary_op_kind {
  UNARY_PLUS = 1, UNARY_MINUS = 2, UNARY_BIT_NOT = 3, UNARY_LNOT = 4
} UnaryOpKind;

typedef enum binary_op_kind {
  BINARY_ADD = 1, BINARY_SUB,
  BINARY_MULT, BINARY_DIV, BINARY_MOD, BINARY_QUOT, BINARY_POWER,
  BINARY_LSHIFT, BINARY_RSHIFT,
  BINARY_GT, BINARY_GE, BINARY_LT, BINARY_LE,
  BINARY_EQ, BINARY_NEQ,
  BINARY_BIT_AND,
  BINARY_BIT_XOR,
  BINARY_BIT_OR,
  BINARY_LAND,
  BINARY_LOR,
} BinaryOpKind;

int binop_arithmetic(int op);
int binop_relation(int op);
int binop_logic(int op);
int binop_bit(int op);

typedef enum expr_kind {
  ID_KIND = 1, INT_KIND = 2, FLOAT_KIND = 3, BOOL_KIND = 4,
  STRING_KIND = 5, SELF_KIND = 6, SUPER_KIND = 7, TYPEOF_KIND = 8,
  NIL_KIND = 9, EXP_KIND = 10, ARRAY_KIND = 11, ANONYOUS_FUNC_KIND = 12,
  ATTRIBUTE_KIND = 13, SUBSCRIPT_KIND = 14, CALL_KIND = 15,
  UNARY_KIND = 16, BINARY_KIND = 17, SEQ_KIND = 18,
  EXPR_KIND_MAX
} ExprKind;

typedef enum expr_ctx {
  EXPR_INVALID = 0, EXPR_LOAD = 1, EXPR_STORE = 2,
  EXPR_CALL_FUNC = 3, EXPR_LOAD_FUNC = 4,
} ExprContext;

typedef struct expr expr_t;

struct expr {
  ExprKind kind;
  TypeDesc *desc;
  ExprContext ctx;
  Symbol *sym;
  int argc;
  expr_t *right;  //for super(); super.name;
  char *supername;
  LineInfo line;
  union {
    char *id;
    int64 ival;
    float64 fval;
    char *str;
    int bval;
    expr_t *exp;
    struct {
      /* one of pointers is null, and another is not null */
      Vector *dseq;
      Vector *tseq;
    } array;
    struct {
      Vector *pvec;
      Vector *rvec;
      Vector *body;
    } anonyous_func;
    /* Trailer */
    struct {
      expr_t *left;
      char *id;
    } attribute;
    struct {
      expr_t *left;
      expr_t *index;
    } subscript;
    struct {
      expr_t *left;
      Vector *args;     /* arguments list */
    } call;
    /* arithmetic operation */
    struct {
      UnaryOpKind op;
      expr_t *operand;
    } unary;
    struct {
      expr_t *left;
      BinaryOpKind op;
      expr_t *right;
    } binary;
    Vector list;
  };
};

expr_t *expr_from_trailer(enum expr_kind kind, void *trailer,
                               expr_t *left);
expr_t *expr_from_id(char *id);
expr_t *expr_from_int(int64 ival);
expr_t *expr_from_float(float64 fval);
expr_t *expr_from_string(char *str);
expr_t *expr_from_bool(int bval);
expr_t *expr_from_self(void);
expr_t *expr_from_super(void);
expr_t *expr_from_typeof(void);
expr_t *expr_from_expr(expr_t *exp);
expr_t *expr_from_nil(void);
expr_t *expr_from_array(TypeDesc *desc, Vector *dseq, Vector *tseq);
expr_t *expr_from_array_with_tseq(Vector *tseq);
expr_t *expr_from_anonymous_func(Vector *pvec, Vector *rvec, Vector *body);
expr_t *expr_from_binary(enum binary_op_kind kind,
                              expr_t *left, expr_t *right);
expr_t *expr_from_unary(enum unary_op_kind kind, expr_t *expr);
void expr_traverse(expr_t *exp);
expr_t *expr_rright_exp(expr_t *exp);

/*-------------------------------------------------------------------------*/

struct test_block {
  expr_t *test;
  Vector *body;
};

struct test_block *new_test_block(expr_t *test, Vector *body);

typedef enum assign_op_kind {
  OP_ASSIGN = 1, OP_PLUS_ASSIGN, OP_MINUS_ASSIGN,
  OP_MULT_ASSIGN, OP_DIV_ASSIGN,
  OP_MOD_ASSIGN, OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
  OP_RSHIFT_ASSIGN, OP_LSHIFT_ASSIGN,
} AssignOpKind;

typedef enum stmt_kind {
  VAR_KIND = 1, VARLIST_KIND, FUNC_KIND, PROTO_KIND, ASSIGN_KIND,
  ASSIGNS_KIND, RETURN_KIND, EXPR_KIND, BLOCK_KIND,
  CLASS_KIND, TRAIT_KIND,
  IF_KIND, WHILE_KIND, SWITCH_KIND, FOR_TRIPLE_KIND,
  FOR_EACH_KIND, BREAK_KIND, CONTINUE_KIND, GO_KIND,
  TYPEALIAS_KIND, LIST_KIND, STMT_KIND_MAX
} StmtKind;

typedef struct stmt stmt_t;

struct stmt {
  StmtKind kind;
  LineInfo line;
  union {
    struct {
      char *id;
      int bconst;
      TypeDesc *desc;
      expr_t *exp;
    } var;
    struct {
      int bconst;
      TypeDesc *desc;
      Vector *ids;
      expr_t *exp;
    } vars;
    struct {
      char *id;
      Vector *args;
      Vector *rets;
      Vector *body;
    } func;
    struct {
      char *id;
      Vector *args;
      Vector *rets;
    } proto;
    struct {
      expr_t *left;
      AssignOpKind op;
      expr_t *right;
    } assign;
    struct {
      Vector *left;
      expr_t *right;
    } assigns;
    Vector *returns;
    expr_t *exp;
    Vector *block;
    struct {
      char *id;
      TypeDesc *super;
      Vector *traits;
      Vector *body;
    } class_info;
    struct {
      char *id;
      TypeDesc *desc;
    } usrdef;
    struct {
      char *id;
      TypeDesc *desc;
    } typealias;
    struct {
      int belse;
      expr_t *test;
      Vector *body;
      stmt_t *orelse;
    } if_stmt;
    struct {
      int btest;
      expr_t *test;
      Vector *body;
    } while_stmt;
    struct {
      int level;
    } jump_stmt;
    struct {
      expr_t *expr;
      Vector *case_seq;
    } switch_stmt;
    struct {
      stmt_t *init;
      stmt_t *test;
      stmt_t *incr;
      Vector *body;
    } for_triple_stmt;
    struct {
      int bdecl;
      struct var *var;
      expr_t *expr;
      Vector *body;
    } for_each_stmt;
    expr_t *go_stmt;
    Vector list;
  };
};

stmt_t *stmt_from_var(char *id, TypeDesc *desc, expr_t *exp, int bconst);
stmt_t *stmt_from_varlist(Vector *ids, TypeDesc *desc, expr_t *exp, int bconst);
stmt_t *stmt_from_func(char *id, Vector *args, Vector *rets, Vector *body);
stmt_t *stmt_from_proto(char *id, Vector *args, Vector *rets);
stmt_t *stmt_from_assign(expr_t *l, AssignOpKind op, expr_t *r);
stmt_t *stmt_from_assigns(Vector *left, expr_t *right);
stmt_t *stmt_from_return(Vector *list);
stmt_t *stmt_from_expr(expr_t *exp);
stmt_t *stmt_from_block(Vector *block);

stmt_t *stmt_from_trait(char *id, Vector *traits, Vector *body);
stmt_t *stmt_from_jump(int kind, int level);
stmt_t *stmt_from_if(expr_t *test, Vector *body, stmt_t *orelse);
stmt_t *stmt_from_while(expr_t *test, Vector *body, int btest);
stmt_t *stmt_from_switch(expr_t *expr, Vector *case_seq);
stmt_t *stmt_from_for(stmt_t *init, stmt_t *test, stmt_t *incr,
                         Vector *body);
stmt_t *stmt_from_go(expr_t *expr);
stmt_t *stmt_from_typealias(char *id, TypeDesc *desc);
void vec_stmt_free(Vector *stmts);
void vec_stmt_fini(Vector *stmts);
stmt_t *stmt_from_list(void);
void stmt_free_list(stmt_t *stmt);

/*-------------------------------------------------------------------------*/


#ifdef __cplusplus
}
#endif
#endif /* _KOALA_AST_H_ */
