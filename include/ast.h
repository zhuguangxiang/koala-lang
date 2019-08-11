/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#include "symbol.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

/* identifier */
typedef struct ident {
  char *name;
  Position pos;
} Ident;

/* typedesc with position */
typedef struct postype {
  TypeDesc *desc;
  Position pos;
} PosType;

/* idtype for parameter-list in AST */
typedef struct idtype {
  Ident id;
  PosType type;
} IdType;

/* parameter type */
typedef struct typepara {
  Ident id;       /* T: Foo & Bar */
  Vector *types;  /* Foo, Bar and etc, PosType */
} TypePara;

void TypePara_IsDuplicated(Vector *typeparas, Ident *id);

/* unary operator kind */
typedef enum unaryopkind {
  /* + */
  UNARY_PLUS = 1,
  /* - */
  UNARY_NEG,
  /* ~ */
  UNARY_BIT_NOT,
  /* ! */
  UNARY_LNOT
} UnaryOpKind;

/* binary operator kind */
typedef enum binaryopkind {
  /* +, -, *, /, % */
  BINARY_ADD = 1, BINARY_SUB, BINARY_MULT, BINARY_DIV, BINARY_MOD,
  /* >, >=, <, <=, ==, != */
  BINARY_GT, BINARY_GE, BINARY_LT, BINARY_LE, BINARY_EQ, BINARY_NEQ,
  /* &, ^, | */
  BINARY_BIT_AND, BINARY_BIT_XOR, BINARY_BIT_OR,
  /* &&, || */
  BINARY_LAND, BINARY_LOR,
} BinaryOpKind;

/* expression kind */
typedef enum exprkind {
  /* expr head only */
  NIL_KIND = 1, SELF_KIND, SUPER_KIND,
  /* literal expr */
  LITERAL_KIND,
  /* identifier expression */
  ID_KIND,
  /* unary, binary op */
  UNARY_KIND, BINARY_KIND,
  /* dot, [], (), [:] access */
  ATTRIBUTE_KIND, SUBSCRIPT_KIND, CALL_KIND, SLICE_KIND,
  /* array list */
  ARRAY_LIST_KIND,
  /* map list */
  MAP_LIST_KIND,
  /* map entry */
  MAP_ENTRY_KIND,
  /* array, map, anonymous func */
  ARRAY_KIND, MAP_KIND, ANONY_FUNC_KIND,
  /* import */
  IMPORT_KIND,
  /* const */
  CONST_KIND,
  /* variable */
  VAR_KIND,
  /* assignment */
  ASSIGN_KIND,
  /* function */
  FUNC_KIND,
  /* return */
  RETURN_KIND,
  EXPR_KIND_MAX
} ExprKind;

/* expression context */
typedef enum exprctx {
  EXPR_INVALID,
  /* left or right value indicator */
  EXPR_LOAD, EXPR_STORE,
  /* call or load function indicator */
  EXPR_CALL_FUNC, EXPR_LOAD_FUNC
} ExprCtx;

typedef struct expr Expr;

#define EXPR_HEAD \
  ExprKind kind;  \
  Position pos;   \
  TypeDesc *desc; \
  ExprCtx ctx;    \
  Symbol *sym;    \
  Expr *right;

struct expr {
  EXPR_HEAD
};

Expr *Expr_From_Nil(void);
Expr *Expr_From_Self(void);
Expr *Expr_From_Super(void);

/* literal expression */
typedef struct constexpr {
  EXPR_HEAD
  ConstValue value;
} ConstExpr;

Expr *expr_from_integer(int64_t val);
Expr *expr_from_float(double val);
Expr *expr_from_bool(int val);
Expr *expr_from_string(char *val);
Expr *expr_from_char(wchar val);

/* identifier expression */
typedef struct identexpr {
  EXPR_HEAD
  /* identifier name */
  char *name;
  /*
   * where is the identifier?
   */
  int where;
#define CURRENT_SCOPE 1
#define UP_SCOPE      2
#define EXT_SCOPE     3
#define EXTDOT_SCOPE  4
  /* scope pointer */
  void *scope;
} IdentExpr;

Expr *expr_from_ident(char *val);
int Expr_Is_Const(Expr *exp);

/* unary expression */
typedef struct unaryexpr {
  EXPR_HEAD
  UnaryOpKind op;
  Expr *exp;
  ConstValue val;
} UnaryExpr;

/* binary expression */
typedef struct binaryexpr {
  EXPR_HEAD
  BinaryOpKind op;
  Expr *lexp;
  Expr *rexp;
  ConstValue val;
} BinaryExpr;

Expr *Expr_From_Unary(UnaryOpKind op, Expr *exp);
Expr *Expr_From_Binary(BinaryOpKind op, Expr *left, Expr *right);

/* attribute expression */
typedef struct attributeexpr {
  EXPR_HEAD
  Ident id;
  Expr *lexp;
} AttributeExpr;

/* subscript expression */
typedef struct subscriptexpr {
  EXPR_HEAD
  Expr *index;
  Expr *lexp;
} SubScriptExpr;

/* function call expression */
typedef struct callexpr {
  EXPR_HEAD
  /* arguments list */
  Vector *args;
  Expr *lexp;
} CallExpr;

/* slice expression */
typedef struct sliceexpr {
  EXPR_HEAD
  Expr *start;
  Expr *end;
  Expr *lexp;
} SliceExpr;

Expr *Expr_From_Attribute(Ident id, Expr *left);
Expr *Expr_From_SubScript(Expr *index, Expr *left);
Expr *Expr_From_Call(Vector *args, Expr *left);
Expr *Expr_From_Slice(Expr *start, Expr *end, Expr *left);

/* array, set or map list expression */
typedef struct listexpr {
  EXPR_HEAD
  /* nesting count  */
  int nesting;
  /* expressions */
  Vector *vec;
} ListExpr;

Expr *Expr_From_ArrayListExpr(Vector *vec);

/* new array expression */
typedef struct arrayexpr {
  EXPR_HEAD
  Vector *dims;
  PosType base;
  ListExpr *listExp;
} ArrayExpr;

Expr *Parser_New_Array(Vector *vec, int dims, PosType type, Expr *listExp);

/* map entry expression */
typedef struct mapentryexpr {
  EXPR_HEAD
  Expr *key;
  Expr *val;
} MapEntryExpr;

Expr *Expr_From_MapEntry(Expr *k, Expr *v);

/* new map expression */
typedef struct mapexpr {
  EXPR_HEAD
  PosType type;
  ListExpr *listExp;
} MapExpr;

Expr *Expr_From_MapListExpr(Vector *vec);
Expr *Expr_From_Map(PosType type, Expr *listExp);

/* new anonymous function expression */
typedef struct anonyexpr {
  EXPR_HEAD
  /* idtype */
  Vector *args;
  /* return type */
  TypeDesc *ret;
  /* block */
  Vector *body;
} AnonyExpr;

Expr *Expr_From_Anony(Vector *args, TypeDesc *ret, Vector *body);

/* constant/variable declaration */
typedef struct vardeclexpr {
  EXPR_HEAD
  Ident id;
  PosType type;
  Expr *exp;
} VarDeclExpr;

/* assignment operators */
typedef enum assignopkind {
  OP_ASSIGN = 1,
  OP_PLUS_ASSIGN, OP_MINUS_ASSIGN,
  OP_MULT_ASSIGN, OP_DIV_ASSIGN, OP_MOD_ASSIGN,
  OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
} AssignOpKind;

/* assignment */
typedef struct assignexpr {
  EXPR_HEAD
  AssignOpKind op;
  Expr *lexp;
  Expr *rexp;
} AssignExpr;

/* function/proto/native declaration */
typedef struct funcdeclexpr {
  EXPR_HEAD
  Ident id;
  /* native flag */
  int native;
  /* type parameters */
  Vector *typeparas;
  /* idtype */
  Vector *args;
  /* return type */
  PosType ret;
  /* body */
  Vector *body;
} FuncDeclExpr;

/* return */
typedef struct returnstmt {
  EXPR_HEAD
  /* expression */
  Expr *exp;
} ReturnExpr;

Expr *Expr_From_ConstDecl(Ident id, PosType type, Expr *exp);
Expr *Expr_From_VarDecl(Ident id, PosType type, Expr *exp);
Expr *Expr_From_Assign(AssignOpKind op, Expr *left, Expr *right);
Expr *Expr_From_FuncDecl(Ident id, Vector *typeparams, Vector *args,
                         PosType ret, Vector *stmts);
Expr *expr_from_return(Expr *exp);

void free_expr(Expr *exp);
int expr_maybe_stored(Expr *exp);
#define expr_iscall(exp) \
  (((exp) != NULL && (exp)->kind == CALL_KIND) ? 1 : 0)

/*
typedef struct klassstmt {
  STMT_HEAD
  Ident id;
  Vector *typeparas;
  Vector *super;
  Vector *body;
} KlassStmt;

Stmt *Stmt_From_Class(Ident id, Vector *types, Vector *super, Vector *body);
Stmt *Stmt_From_Trait(Ident id, Vector *types, Vector *super, Vector *body);

typedef struct enumvaluestmt {
  STMT_HEAD
  Ident id;
  Vector *types;
  Expr *exp;
} EnumValStmt;

Stmt *New_EnumValue(Ident id, Vector *types, Expr *exp);

typedef struct enumstmt {
  STMT_HEAD
  Ident id;
  Vector *typeparas;
  Vector *body;
} EnumStmt;

Stmt *Stmt_From_Enum(Ident id, Vector *types, Vector *body);
*/

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_AST_H_ */
