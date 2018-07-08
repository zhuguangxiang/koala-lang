
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

typedef struct expr Expression;

struct expr {
  ExprKind kind;
  TypeDesc *desc;
  ExprContext ctx;
  Symbol *sym;
  int argc;
  Expression *right;  //for super(); super.name;
  char *supername;
  LineInfo line;
  union {
    char *id;
    int64 ival;
    float64 fval;
    char *str;
    int bval;
    Expression *exp;
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
      Expression *left;
      char *id;
    } attribute;
    struct {
      Expression *left;
      Expression *index;
    } subscript;
    struct {
      Expression *left;
      Vector *args;     /* arguments list */
    } call;
    /* arithmetic operation */
    struct {
      UnaryOpKind op;
      Expression *operand;
    } unary;
    struct {
      Expression *left;
      BinaryOpKind op;
      Expression *right;
    } binary;
    Vector list;
  };
};

Expression *expr_from_trailer(enum expr_kind kind, void *trailer,
                               Expression *left);
Expression *expr_from_id(char *id);
Expression *expr_from_int(int64 ival);
Expression *expr_from_float(float64 fval);
Expression *expr_from_string(char *str);
Expression *expr_from_bool(int bval);
Expression *expr_from_self(void);
Expression *expr_from_super(void);
Expression *expr_from_typeof(void);
Expression *expr_from_expr(Expression *exp);
Expression *expr_from_nil(void);
Expression *expr_from_array(TypeDesc *desc, Vector *dseq, Vector *tseq);
Expression *expr_from_array_with_tseq(Vector *tseq);
Expression *expr_from_anonymous_func(Vector *pvec, Vector *rvec, Vector *body);
Expression *expr_from_binary(enum binary_op_kind kind,
                              Expression *left, Expression *right);
Expression *expr_from_unary(enum unary_op_kind kind, Expression *expr);
void expr_traverse(Expression *exp);

/*-------------------------------------------------------------------------*/

struct test_block {
  Expression *test;
  Vector *body;
};

struct test_block *new_test_block(Expression *test, Vector *body);

typedef enum assign_op_kind {
  OP_ASSIGN = 1,
  OP_PLUS_ASSIGN, OP_MINUS_ASSIGN,
  OP_MULT_ASSIGN, OP_DIV_ASSIGN,
  OP_MOD_ASSIGN, OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
  OP_RSHIFT_ASSIGN, OP_LSHIFT_ASSIGN,
} AssignOpKind;

typedef enum stmt_kind {
  VARDECL_KIND = 1, FUNCDECL_KIND, FUNCPROTO_KIND, CLASS_KIND,
  TRAIT_KIND, EXPR_KIND, ASSIGN_KIND,
  RETURN_KIND, IF_KIND, WHILE_KIND, SWITCH_KIND, FOR_TRIPLE_KIND,
  FOR_EACH_KIND, BREAK_KIND, CONTINUE_KIND, GO_KIND, BLOCK_KIND,
  TYPEALIAS_KIND, LIST_KIND, STMT_KIND_MAX
} StmtKind;

struct assign {
  Expression *left;
  Expression *right;
};

typedef struct stmt Statement;

struct stmt {
  StmtKind kind;
  LineInfo line;
  union {
    struct {
      char *id;
      int bconst;
      TypeDesc *desc;
      Expression *exp;
    } vardecl;
    struct {
      char *id;
      Vector *args;
      Vector *rets;
      Vector *body;
    } funcdecl;
    struct {
      Expression *left;
      AssignOpKind op;
      Expression *right;
    } assign;
    Vector *returns;
    struct {
      char *id;
      TypeDesc *super;
      Vector *traits;
      Vector *body;
    } class_info;
    struct {
      char *id;
      Vector *pvec;
      Vector *rvec;
    } funcproto;
    struct {
      char *id;
      TypeDesc *desc;
    } user_typedef;
    struct {
      char *id;
      TypeDesc *desc;
    } typealias;
    struct {
      int belse;
      Expression *test;
      Vector *body;
      Statement *orelse;
    } if_stmt;
    struct {
      int btest;
      Expression *test;
      Vector *body;
    } while_stmt;
    struct {
      int level;
    } jump_stmt;
    struct {
      Expression *expr;
      Vector *case_seq;
    } switch_stmt;
    struct {
      Statement *init;
      Statement *test;
      Statement *incr;
      Vector *body;
    } for_triple_stmt;
    struct {
      int bdecl;
      struct var *var;
      Expression *expr;
      Vector *body;
    } for_each_stmt;
    Expression *go_stmt;
    Expression *exp;
    Vector list;
  };
};

Statement *stmt_from_expr(Expression *exp);
Statement *stmt_from_vardecl(char *id, TypeDesc *desc, int k, Expression *exp);
Statement *stmt_from_funcdecl(char *id, Vector *args, Vector *rets,
                              Vector *body);
Statement *stmt_from_assign(Expression *l, AssignOpKind op, Expression *r);
Statement *stmt_from_block(Vector *vec);
Statement *stmt_from_return(Vector *vec);
Statement *stmt_from_empty(void);
Statement *stmt_from_trait(char *id, Vector *traits, Vector *body);
Statement *stmt_from_funcproto(char *id, Vector *pvec, Vector *rvec);
Statement *stmt_from_jump(int kind, int level);
Statement *stmt_from_if(Expression *test, Vector *body, Statement *orelse);
Statement *stmt_from_while(Expression *test, Vector *body, int btest);
Statement *stmt_from_switch(Expression *expr, Vector *case_seq);
Statement *stmt_from_for(Statement *init, Statement *test, Statement *incr,
                         Vector *body);
Statement *stmt_from_foreach(struct var *var, Expression *expr, Vector *body,
                             int bdecl);
Statement *stmt_from_go(Expression *expr);
Statement *stmt_from_typealias(char *id, TypeDesc *desc);
Statement *stmt_from_list(void);
void vec_stmt_free(Vector *stmts);
void vec_stmt_fini(Vector *stmts);
void stmt_free_vardecl_list(Statement *stmt);

/*-------------------------------------------------------------------------*/


#ifdef __cplusplus
}
#endif
#endif /* _KOALA_AST_H_ */
