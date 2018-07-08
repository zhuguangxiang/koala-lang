
#include "ast.h"
#include "log.h"

/*-------------------------------------------------------------------------*/

Expression *expr_new(int kind)
{
	Expression *exp = calloc(1, sizeof(Expression));
	exp->kind = kind;
	return exp;
}

Expression *expr_from_id(char *id)
{
	Expression *expr = expr_new(ID_KIND);
	expr->id = id;
	return expr;
}

Expression *expr_from_int(int64 ival)
{
	Expression *expr = expr_new(INT_KIND);
	expr->desc = &Int_Type;
	expr->ival = ival;
	return expr;
}

Expression *expr_from_float(float64 fval)
{
	Expression *expr = expr_new(FLOAT_KIND);
	expr->desc = &Float_Type;
	expr->fval = fval;
	return expr;
}

Expression *expr_from_string(char *str)
{
	Expression *expr = expr_new(STRING_KIND);
	expr->desc = &String_Type;
	expr->str = str;
	return expr;
}

Expression *expr_from_bool(int bval)
{
	Expression *expr = expr_new(BOOL_KIND);
	expr->desc = &Bool_Type;
	expr->bval = bval;
	return expr;
}

Expression *expr_from_self(void)
{
	Expression *expr = expr_new(SELF_KIND);
	expr->desc = NULL;
	return expr;
}

Expression *expr_from_super(void)
{
	Expression *expr = expr_new(SUPER_KIND);
	expr->desc = NULL;
	return expr;
}

Expression *expr_from_typeof(void)
{
	Expression *expr = expr_new(TYPEOF_KIND);
	expr->desc = NULL;
	return expr;
}

Expression *expr_from_expr(Expression *exp)
{
	Expression *expr = expr_new(EXP_KIND);
	expr->desc = NULL;
	expr->exp  = exp;
	return expr;
}

Expression *expr_from_nil(void)
{
	Expression *expr = expr_new(NIL_KIND);
	expr->desc = NULL;
	return expr;
}

Expression *expr_from_array(TypeDesc *desc, Vector *dseq, Vector *tseq)
{
	Expression *expr = expr_new(ARRAY_KIND);
	expr->desc = desc;
	expr->array.dseq = dseq;
	expr->array.tseq = tseq;
	return expr;
}

Expression *expr_from_array_with_tseq(Vector *tseq)
{
	UNUSED_PARAMETER(tseq);
	Expression *e = expr_new(SEQ_KIND);
	//e->vec = tseq;
	return e;
}

Expression *expr_from_anonymous_func(Vector *pvec, Vector *rvec, Vector *body)
{
	Expression *expr = expr_new(ANONYOUS_FUNC_KIND);
	expr->anonyous_func.pvec = pvec;
	expr->anonyous_func.rvec = rvec;
	expr->anonyous_func.body = body;
	return expr;
}

Expression *expr_from_trailer(enum expr_kind kind, void *trailer,
															 Expression *left)
{
	Expression *expr = expr_new(kind);
	switch (kind) {
		case ATTRIBUTE_KIND: {
			expr->ctx = EXPR_LOAD;
			expr->attribute.left = left;
			expr->attribute.id = trailer;
			break;
		}
		case SUBSCRIPT_KIND: {
			expr->subscript.left = left;
			expr->subscript.index = trailer;
			break;
		}
		case CALL_KIND: {
			expr->call.left = left;
			expr->call.args = trailer;
			break;
		}
		default: {
			kassert(0, "unkown expression kind %d\n", kind);
		}
	}
	return expr;
}

Expression *expr_from_binary(enum binary_op_kind kind,
															Expression *left, Expression *right)
{
	Expression *exp = expr_new(BINARY_KIND);
	exp->desc = left->desc;
	exp->binary.left = left;
	exp->binary.op = kind;
	exp->binary.right = right;
	return exp;
}

Expression *expr_from_unary(enum unary_op_kind kind, Expression *expr)
{
	Expression *exp = expr_new(UNARY_KIND);
	exp->desc = expr->desc;
	exp->unary.op = kind;
	exp->unary.operand = expr;
	return exp;
}

/*--------------------------------------------------------------------------*/

static inline Statement *stmt_new(int kind)
{
	Statement *stmt = calloc(1, sizeof(Statement));
	stmt->kind = kind;
	return stmt;
}

static inline void stmt_free(Statement *stmt)
{
	free(stmt);
}

Statement *stmt_from_expr(Expression *exp)
{
	Statement *stmt = stmt_new(EXPR_KIND);
	stmt->exp = exp;
	return stmt;
}

Statement *stmt_from_vardecl(char *id, TypeDesc *desc, int k, Expression *exp)
{
	Statement *stmt = stmt_new(VARDECL_KIND);
	stmt->vardecl.id = id;
	stmt->vardecl.desc = desc;
	stmt->vardecl.bconst = k;
	stmt->vardecl.exp = exp;
	if (!desc && exp->desc) stmt->vardecl.desc = Type_Dup(exp->desc);
	return stmt;
}

Statement *stmt_from_funcdecl(char *id, Vector *pvec, Vector *rvec,
	Vector *body)
{
	Statement *stmt = stmt_new(FUNCDECL_KIND);
	stmt->funcdecl.id = id;
	stmt->funcdecl.pvec = pvec;
	stmt->funcdecl.rvec = rvec;
	stmt->funcdecl.body = body;
	return stmt;
}

Statement *stmt_from_block(Vector *block)
{
	UNUSED_PARAMETER(block);
	return NULL; //stmt_from_vector(BLOCK_KIND, block);
}

Statement *stmt_from_return(Vector *list)
{
	Statement *stmt = stmt_new(RETURN_KIND);
	stmt->returns = list;
	return stmt;
}

Statement *stmt_from_assign(Expression *l, AssignOpKind op, Expression *r)
{
	Statement *stmt = stmt_new(ASSIGN_KIND);
	stmt->assign.left  = l;
	stmt->assign.op    = op;
	stmt->assign.right = r;
	return stmt;
}

Statement *stmt_from_trait(char *id, Vector *traits, Vector *body)
{
	Statement *stmt = stmt_new(TRAIT_KIND);
	stmt->class_info.id = id;
	stmt->class_info.traits = traits;
	stmt->class_info.body = body;
	return stmt;
}

Statement *stmt_from_funcproto(char *id, Vector *pvec, Vector *rvec)
{
	Statement *stmt = stmt_new(FUNCPROTO_KIND);
	stmt->funcproto.id = id;
	stmt->funcproto.pvec = pvec;
	stmt->funcproto.rvec = rvec;
	return stmt;
}

Statement *stmt_from_jump(int kind, int level)
{
	if (level <= 0) {
		error("break or continue level must be > 0");
		return NULL;
	}
	Statement *stmt = stmt_new(kind);
	stmt->jump_stmt.level = level;
	return stmt;
}

Statement *stmt_from_if(Expression *test, Vector *body,
	Statement *orelse)
{
	Statement *stmt = stmt_new(IF_KIND);
	stmt->if_stmt.test = test;
	stmt->if_stmt.body = body;
	stmt->if_stmt.orelse = orelse;
	return stmt;
}

struct test_block *new_test_block(Expression *test, Vector *body)
{
	struct test_block *tb = malloc(sizeof(struct test_block));
	tb->test = test;
	tb->body = body;
	return tb;
}

Statement *stmt_from_while(Expression *test, Vector *body, int btest)
{
	Statement *stmt = stmt_new(WHILE_KIND);
	stmt->while_stmt.btest = btest;
	stmt->while_stmt.test  = test;
	stmt->while_stmt.body  = body;
	return stmt;
}

Statement *stmt_from_switch(Expression *expr, Vector *case_seq)
{
	Statement *stmt = stmt_new(SWITCH_KIND);
	stmt->switch_stmt.expr = expr;
	stmt->switch_stmt.case_seq = case_seq;
	return stmt;
}

Statement *stmt_from_for(Statement *init, Statement *test,
													 Statement *incr, Vector *body)
{
	Statement *stmt = stmt_new(FOR_TRIPLE_KIND);
	stmt->for_triple_stmt.init = init;
	stmt->for_triple_stmt.test = test;
	stmt->for_triple_stmt.incr = incr;
	stmt->for_triple_stmt.body = body;
	return stmt;
}

Statement *stmt_from_foreach(struct var *var, Expression *expr,
															 Vector *body, int bdecl)
{
	Statement *stmt = stmt_new(FOR_EACH_KIND);
	stmt->for_each_stmt.bdecl = bdecl;
	stmt->for_each_stmt.var   = var;
	stmt->for_each_stmt.expr  = expr;
	stmt->for_each_stmt.body  = body;
	return stmt;
}

Statement *stmt_from_go(Expression *expr)
{
	if (expr->kind != CALL_KIND) {
		kassert(0, "syntax error:not a func call\n");
		exit(0);
	}

	Statement *stmt = stmt_new(GO_KIND);
	stmt->go_stmt = expr;
	return stmt;
}

Statement *stmt_from_typealias(char *id, TypeDesc *desc)
{
	Statement *stmt = stmt_new(TYPEALIAS_KIND);
	stmt->typealias.id = id;
	stmt->typealias.desc = desc;
	return stmt;
}

Statement *stmt_from_list(void)
{
	Statement *stmt = stmt_new(LIST_KIND);
	Vector_Init(&stmt->list);
	return stmt;
}

void stmt_free_list(Statement *stmt)
{
	Statement *s;
	Vector_ForEach(s, &stmt->list) {
		stmt_free(s);
	}
	stmt_free(stmt);
}

/*---------------------------------------------------------------------------*/

int binop_arithmetic(int op)
{
	if (op >= BINARY_ADD && op <= BINARY_RSHIFT)
		return 1;
	else
		return 0;
}

int binop_relation(int op)
{
	if (op >= BINARY_GT && op <= BINARY_NEQ)
		return 1;
	else
		return 0;
}

int binop_logic(int op)
{
	if (op == BINARY_LAND || op == BINARY_LOR)
		return 1;
	else
		return 0;
}

int binop_bit(int op)
{
	if (op == BINARY_BIT_AND || op == BINARY_BIT_XOR || op == BINARY_BIT_OR)
		return 1;
	else
		return 0;
}

/*--------------------------------------------------------------------------*/

struct var *new_var(char *id, TypeDesc *desc)
{
	struct var *v = malloc(sizeof(struct var));
	v->id = id;
	v->bconst = 0;
	v->desc = desc;
	return v;
}

void free_var(struct var *v)
{
	free(v);
}

/*--------------------------------------------------------------------------*/

static void item_stmt_free(void *item, void *arg)
{
	UNUSED_PARAMETER(arg);
	stmt_free(item);
}

void vec_stmt_free(Vector *stmts)
{
	if (!stmts) return;
	Vector_Free(stmts, item_stmt_free, NULL);
}

void vec_stmt_fini(Vector *stmts)
{
	if (!stmts) return;
	Vector_Fini(stmts, item_stmt_free, NULL);
}
