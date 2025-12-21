/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) 2023 zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#ifndef _KOALA_TYPESPEC_H_
#define _KOALA_TYPESPEC_H_

#include "buffer.h"
#include "loc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
// 对应 class Foo[T] { T data; }

// 1. 获取当前正在解析的类定义
ClassDef *currentClass = ...; // 指向 Foo 的 ClassDef

// 2. 为字段 'data' 创建 TypeSpec
TypeSpec *dataType = malloc(sizeof(TypeSpec));
dataType->kind = TYPE_GENERIC_VAR;
dataType->as.generic_var.name = strdup("T");

// 3. 核心关联：将此引用绑定回类定义的形参列表
// 这样在语义分析时，你可以验证 "T" 是否确实存在于 Foo 的 type_params 中
dataType->as.generic_var.owner = currentClass;

// 4. 创建字段定义
FieldDef *field = createField("data", dataType);
-------------------------------------------------------------------------
class Foo[T] { Bar[T] data;}

// 1. 定义 Foo 类
ClassDef *fooDef = createClassDef("Foo", {"T"});

// 2. 解析字段 Bar[T] data
// 这对应一个嵌套的 TypeSpec 结构：
TypeSpec *fieldTypespec = malloc(sizeof(TypeSpec));
fieldTypespec->kind = TYPE_SPECIALIZED;
fieldTypespec->as.specialized.base_class = findClass("Bar"); // 指向 Bar 的 ClassDef

// 3. 关键点：Bar[T] 的参数 T
TypeSpec *tArg = malloc(sizeof(TypeSpec));
tArg->kind = TYPE_GENERIC_VAR;
tArg->as.generic_var.name = "T";
// 建立回溯关联：告诉编译器这个 T 是谁定义的
tArg->as.generic_var.owner = fooDef;

// 4. 将 T 放入 Bar 的参数列表
fieldTypespec->as.specialized.args = malloc(sizeof(TypeSpec*));
fieldTypespec->as.specialized.args[0] = tArg;
fieldTypespec->as.specialized.count = 1;

ClassDef(Foo)
  |-- TypeParams: ["T"]
  |-- Fields:
        |-- FieldDef("data")
              |-- TypeSpec(kind: SPECIALIZED)
                    |-- base_class: ClassDef(Bar)
                    |-- args:
                          |-- [0]: TypeSpec(kind: GENERIC_VAR)
                                     |-- name: "T"
                                     |-- owner: ClassDef(Foo) <--- [反向引用]
*/
typedef enum _TypeKind {
    TYPE_UNRESOLVED,
    TYPE_SPECIALIZED_UNRESOLVED,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BFLOAT16,
    TYPE_BOOL,
    TYPE_STR,
    TYPE_BASE,
    TYPE_CLASS,
    TYPE_TRAIT,
    TYPE_GENERIC_VAR,
    TYPE_SPECIALIZED,
} TypeKind;

typedef struct _TypeSpec {
    TypeKind kind;
    Loc loc;
    union {
        // int/float width
        struct {
            int width;
            int sign;
        } int_flt_info;

        char *base;

        struct {
            char *name;
            char *pkg_name;
            char *class_trait_name;
        } unresolved;

        // T extends Animal
        struct {
            char *name;
            struct _TypeSpec **bounds;
            int count;
        } generic_var;

        // class info
        struct {
            char *pkg_name;
            char *class_name;
            // struct _ClassDef *def;
            void *def;
        } class_info;

        // trait info
        struct {
            char *full_name;
            // struct _TraitDef *def;
            void *def;
        } trait_info;

        // List<int>
        struct {
            struct _TypeSpec *base_class;
            struct _TypeSpec **args;
            int count;
        } specialized;

        // Bar[T]
        struct {
            char *base_name;
            struct _TypeSpec **args;
            int count;
        } specialized_unresolved;
    };
} TypeSpec;

#define type_spec_loc(ty, _loc) (ty)->loc = (_loc)

TypeSpec *int_type_spec(int width, int sign);

int type_spec_is_compatible(TypeSpec *dst, TypeSpec *src);

int type_spec_to_str(TypeSpec *ts, Buffer *buf);

void type_spec_print(TypeSpec *ts, Buffer *buf);
void type_spec_str_print(char *s, Buffer *buf);

#ifdef __cplusplus
}
#endif

#endif /* _KOALA_TYPESPEC_H_ */
