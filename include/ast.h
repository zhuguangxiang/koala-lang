/*
 * MIT License
 * Copyright (c) 2018 James, https://github.com/zhuguangxiang
 */

#ifndef _KOALA_AST_H_
#define _KOALA_AST_H_

#ifdef  __cplusplus
extern "C" {
#endif

enum stmtkind {
  IMPORT_STMT = 1,
  CONST_STMT, VAR_STMT, ASSIGN_STMT, EXPR_STMT, FUNC_STMT
};

#define STMT_HEAD int kind;

struct stmt {
  STMT_HEAD
};

#ifdef  __cplusplus
}
#endif

#endif /* _KOALA_AST_H_ */
