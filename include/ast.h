/*
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#include "symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* source code token position for error handler */
typedef struct position {
  int row;
  int col;
} Position;

/* identifier */
typedef struct ident {
  char *name;
  Position pos;
} Ident;

Ident *New_Ident(String name);
void Free_IdentList(Vector *vec);

/* typedesc wrapper with position */
typedef struct typewrapper {
  TypeDesc *desc;
  Position pos;
} TypeWrapper;

/* idtype for parameter-list in AST */
typedef struct idtype {
  Ident id;
  TypeWrapper type;
} IdType;

IdType *New_IdType(Ident *id, TypeWrapper *type);
void Free_IdType(IdType *idtype);
void Free_IdTypeList(Vector *vec);

/* unary operator kind */
typedef enum unaryopkind {
  /* +, - */
  UNARY_PLUS = 1, UNARY_MINUS,
  /* ~ */
  UNARY_BIT_NOT,
  /* ! */
  UNARY_LNOT
} UnaryOpKind;

/* binary operator kind */
typedef enum binaryopkind {
  /* +, -, *, /, % */
  BINARY_ADD = 1, BINARY_SUB,
  BINARY_MULT, BINARY_DIV, BINARY_MOD, BINARY_QUOT, BINARY_POWER,
  /* <<, >> */
  BINARY_LSHIFT, BINARY_RSHIFT,
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
  /* basic expr */
  INT_KIND, FLOAT_KIND, BOOL_KIND, STRING_KIND, CHAR_KIND, ID_KIND,
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
  /* array, map, set, anony func */
  ARRAY_KIND, MAP_KIND, SET_KIND, ANONY_FUNC_KIND,
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
  Expr *right;    \
  Symbol *sym;    \
  int argc;       \
  char *supername;

struct expr {
  EXPR_HEAD
};

Expr *Expr_From_Nil(void);
Expr *Expr_From_Self(void);
Expr *Expr_From_Super(void);

/* base expression */
typedef struct baseexpr {
  EXPR_HEAD
  union {
    uchar ch;
    int64 ival;
    float64 fval;
    int bval;
    char *str;
    char *id;
  };
} BaseExpr;

Expr *Expr_From_Integer(int64 val);
Expr *Expr_From_Float(float64 val);
Expr *Expr_From_Bool(int val);
Expr *Expr_From_String(char *val);
Expr *Expr_From_Char(uchar val);
Expr *Expr_From_Id(char *val);

/* unary expression */
typedef struct unaryexpr {
  EXPR_HEAD
  UnaryOpKind op;
  Expr *exp;
} UnaryExpr;

/* binary expression */
typedef struct binaryexpr {
  EXPR_HEAD
  BinaryOpKind op;
  Expr *lexp;
  Expr *rexp;
} BinaryExpr;

Expr *Expr_From_Unary(UnaryOpKind op, Expr *exp);
Expr *Expr_From_Binary(BinaryOpKind op, Expr *left, Expr *right);

/* attribute expression */
typedef struct attributeexpr {
  EXPR_HEAD
  Ident id;
  Expr *left;
} AttributeExpr;

/* subscript expression */
typedef struct subscriptexpr {
  EXPR_HEAD
  Expr *index;
  Expr *left;
} SubScriptExpr;

/* function call expression */
typedef struct callexpr {
  EXPR_HEAD
  Vector *args; /* arguments list */
  Expr *left;
} CallExpr;

/* slice expression */
typedef struct sliceexpr {
  EXPR_HEAD
  Expr *start;
  Expr *end;
  Expr *left;
} SliceExpr;

Expr *Expr_From_Attribute(Ident id, Expr *left);
Expr *Expr_From_SubScript(Expr *index, Expr *left);
Expr *Expr_From_Call(Vector *args, Expr *left);
Expr *Expr_From_Slice(Expr *start, Expr *end, Expr *left);

/* array or map list expression */
typedef struct listexpr {
  EXPR_HEAD
  int nesting;  /* nesting count  */
  Vector *vec;  /* expressions */
} ListExpr;

Expr *Expr_From_ArrayListExpr(Vector *vec);

/* new array expression */
typedef struct arrayexpr {
  EXPR_HEAD
  Vector *dims;
  TypeDesc *base;
  Position basePos;
  ListExpr *listExp;
} ArrayExpr;

Expr *Parser_New_Array(Vector *vec, int dims, TypeDesc *desc, Expr *listExp);

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
  TypeDesc *mapDesc;
  Position mapPos;
  ListExpr *listExp;
} MapExpr;

Expr *Expr_From_MapListExpr(Vector *vec);
Expr *Expr_From_Map(TypeDesc *desc, Expr *listExp);

/* new set expression */
typedef struct setexpr {
  EXPR_HEAD
  TypeDesc *setDesc;
  Position setPos;
  ListExpr *listExp;
} SetExpr;

Expr *Expr_From_Set(TypeDesc *desc, Expr *listExp);

/* new anonymous function expression */
typedef struct anonyexpr {
  EXPR_HEAD
  Vector *args; /* idtype */
  Vector *rets; /* idtype */
  Vector *body; /* statements */
} AnonyExpr;

Expr *Expr_From_Anony(Vector *args, Vector *rets, Vector *body);

typedef enum stmtkind {
  /*
   * one var and one return's expr
   * examples:
   * var hello = "hello"
   * var h1, h2 = 100, 200
   * hello := "hello"
   * h1, h2 := 100, 200 + 300
   */
  VAR_KIND = 1,
  /*
   * many vars and one function's return with multi values
   * number of left expr > number of right expr
   * examples:
   * var a, b = AddAndSub(100, 200)
   * a, b := AddAndSub(100, 200)
   */
  VARLIST_EXPR_KIND,
  /*
   * one left, one right
   * examples:
   * a = 100
   * a, b = 100, 200
   */
  ASSIGN_KIND,
  /*
   * many left, one right
   * examples:
   * a, b = AddAndSub(100, 200)
   */
  ASSIGNLIST_KIND,
  /* function declaration */
  FUNC_KIND,
  /* proto or native function */
  PROTO_KIND,
  /*
   * examples:
   * a + b;
   * a.b();
   */
  EXPR_KIND,
  /* return */
  RETURN_KIND,
  /* list for block and vardecl-list */
  LIST_KIND,
  /* type alias */
  TYPEALIAS_KIND,
  /* class */
  CLASS_KIND,
  /* trait */
  TRAIT_KIND,

  IF_KIND, WHILE_KIND, SWITCH_KIND, FOR_TRIPLE_KIND,
  FOR_EACH_KIND, BREAK_KIND, CONTINUE_KIND, GO_KIND,

  STMT_KIND_MAX
} StmtKind;

#define STMT_HEAD \
  StmtKind kind;  \
  int native;

typedef struct stmt {
  STMT_HEAD
} Stmt;

/* variable & constant declaration, see VAR_KIND */
typedef struct vardeclstmt {
  STMT_HEAD
  Ident id;
  TypeDesc *desc;
  Position descPos;
  Expr *exp;
  int konst;
} VarDeclStmt;

/* variable list declaration, see VARLIST_EXPR_KIND */
typedef struct varlistdeclstmt {
  STMT_HEAD
  Vector *ids;
  TypeDesc *desc;
  Position descPos;
  Expr *exp;
  int konst;
} VarListDeclStmt;

/* assignment operators */
typedef enum assignopkind {
  OP_ASSIGN = 1,
  OP_PLUS_ASSIGN, OP_MINUS_ASSIGN,
  OP_MULT_ASSIGN, OP_DIV_ASSIGN, OP_MOD_ASSIGN,
  OP_AND_ASSIGN, OP_OR_ASSIGN, OP_XOR_ASSIGN,
  OP_RSHIFT_ASSIGN, OP_LSHIFT_ASSIGN,
} AssignOpKind;

/* assignment statement, see ASSIGN_KIND */
typedef struct assignstmt {
  STMT_HEAD
  AssignOpKind op;
  Expr *left;
  Expr *right;
} AssignStmt;

/* assignment list statement, see ASSIGNLIST_KIND */
typedef struct assignliststmt {
  STMT_HEAD
  Vector *left; /* expression list */
  Expr *right;
} AssignListStmt;

/* function declaration */
typedef struct funcdeclstmt {
  STMT_HEAD
  Ident id;
  Vector *args; /* idtype statement */
  Vector *rets; /* idtype statement */
  Vector *body; /* body statements */
} FuncDeclStmt;

/* expression statement */
typedef struct exprstmt {
  STMT_HEAD
  Expr *exp; /* e.g. list.append(cat); */
} ExprStmt;

/* return statement */
typedef struct returnstmt {
  STMT_HEAD
  Vector *exps; /* expression */
} ReturnStmt;

/* list statement */
typedef struct liststmt {
  STMT_HEAD
  Vector *vec; /* statement */
} ListStmt;

void Free_Stmt_Func(void *item, void *arg);
Stmt *__Stmt_From_VarDecl(Ident *id, TypeDesc *desc, Expr *exp, int k);
Stmt *__Stmt_From_VarListDecl(Vector *ids, TypeDesc *desc, Expr *exp, int k);
#define Stmt_From_VarDecl(id, desc, exp) \
  __Stmt_From_VarDecl(&(id), desc, exp, 0)
#define Stmt_From_VarListDecl(ids, desc, exp) \
  __Stmt_From_VarListDecl(ids, desc, exp, 0)
#define Stmt_From_ConstDecl(id, desc, exp) \
  __Stmt_From_VarDecl(&(id), desc, exp, 1)
#define Stmt_From_ConstListDecl(ids, desc, exp) \
  __Stmt_From_VarListDecl(ids, desc, exp, 1)
Stmt *Stmt_From_Assign(AssignOpKind op, Expr *left, Expr *right);
Stmt *Stmt_From_AssignList(Vector *left, Expr *right);
Stmt *Stmt_From_FuncDecl(Ident id, Vector *args, Vector *rets, Vector *stmts);
Stmt *Stmt_From_Expr(Expr *exp);
Stmt *Stmt_From_Return(Vector *exps);
Stmt *Stmt_From_List(Vector *vec);

/* typealias statement */
typedef struct typealiasstmt {
  STMT_HEAD
  Ident id;
  TypeDesc *desc;
  Position descPos;
} TypeAliasStmt;

/* class or trait statement */
typedef struct klassstmt {
  STMT_HEAD
  Ident id;
  Vector *super;
  Vector *body;
} KlassStmt;

/* proto declaration, used only in trait */
typedef struct protodescstmt {
  STMT_HEAD
  Ident id;
  Vector *args; /* idtype statement */
  Vector *rets; /* idtype statement */
} ProtoDeclStmt;

Stmt *Stmt_From_TypeAlias(Ident id, TypeDesc *desc);
Stmt *Stmt_From_Klass(Ident id, StmtKind kind, Vector *super, Vector *body);
Stmt *Stmt_From_ProtoDecl(Ident id, Vector *args, Vector *rets);

#ifdef __cplusplus
}
#endif
#endif /* _KOALA_AST_H_ */
