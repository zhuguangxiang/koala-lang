/* hot instructions */

/* Move & Load */

TARGET(OP_MOVE) {
    rd = I_VAL(inst, 12, 12);
    rs = I_VAL(inst, 0, 12);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd] = regs[rs];
    DISPATCH();
}

TARGET(OP_LOAD_INT_IMM) {
    rd = I_VAL(inst, 16, 8);
    int ti = I_VAL(inst, 12, 4);
    imm = ((int)(inst << 20)) >> 20;

    CHECK_REG_ID(rd);
    ASSERT((ti & 0b1100) == 0b1000);

    regs[rd].tag = ti;
    regs[rd].ival = imm;
    DISPATCH();
}

TARGET(OP_LOADK) {
    rd = I_VAL(inst, 16, 8);
    idx = I_VAL(inst, 0, 16);

    CHECK_REG_ID(rd);

    TValue *val = CP(idx);
    regs[rd].tag = val->tag;
    regs[rd].ival = val->ival;
    DISPATCH();
}

/* Int Arithmetic */

TARGET(OP_INT_ADD) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd].ival = regs[rs].ival + regs[rt].ival;
    regs[rd].tag = regs[rs].tag | 0b11;

    ASSERT(regs[rd].tag == TAG_INT64 || regs[rd].tag == TAG_UINT64);

    DISPATCH();
}

TARGET(OP_INT_ADD_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_INT(rs);

    regs[rd].ival = regs[rs].ival + imm;
    regs[rd].tag = TAG_INT64;
    DISPATCH();
}

TARGET(OP_UINT_ADD_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival + (uint64_t)imm;
    regs[rd].tag = TAG_UINT64;
    DISPATCH();
}

TARGET(OP_INT_SUB) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd].ival = regs[rs].ival - regs[rt].ival;
    regs[rd].tag = regs[rs].tag | 0b11;

    ASSERT(regs[rd].tag == TAG_INT64 || regs[rd].tag == TAG_UINT64);

    DISPATCH();
}

TARGET(OP_INT_SUB_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_INT(rs);

    regs[rd].ival = regs[rs].ival - imm;
    regs[rd].tag = TAG_INT64;
    DISPATCH();
}

TARGET(OP_UINT_SUB_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival - (uint64_t)imm;
    regs[rd].tag = TAG_UINT64;
    DISPATCH();
}

/* Branching */

TARGET(OP_JMP) {
    off = I_SVAL(inst, 0, 16);
    pc += off;
    DISPATCH();
}

TARGET(OP_JMP_INT_LE_IMM) {
    rs = I_VAL(inst, 16, 8);
    imm = I_SVAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_IS_INT(rs);

    if (regs[rs].ival <= imm) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_INT_GT_IMM) {
    rs = I_VAL(inst, 16, 8);
    imm = I_SVAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_IS_INT(rs);

    if (regs[rs].ival > imm) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_INT_LT_IMM) {
    rs = I_VAL(inst, 16, 8);
    imm = I_SVAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_IS_INT(rs);

    if (regs[rs].ival < imm) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_INT_GE_IMM) {
    rs = I_VAL(inst, 16, 8);
    imm = I_SVAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_IS_INT(rs);

    if (regs[rs].ival >= imm) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_INT_LE) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_INT(rs);
    CHECK_IS_INT(rt);

    if (regs[rs].ival <= regs[rt].ival) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_INT_GT) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_INT(rs);
    CHECK_IS_INT(rt);

    if (regs[rs].ival > regs[rt].ival) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_INT_LT) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_INT(rs);
    CHECK_IS_INT(rt);

    if (regs[rs].ival < regs[rt].ival) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_INT_GE) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_INT(rs);
    CHECK_IS_INT(rt);

    if (regs[rs].ival >= regs[rt].ival) {
        pc += off;
    }
    DISPATCH();
}

/* Return */

TARGET(OP_RET) {
    rs = I_VAL(inst, 0, 12);
    CHECK_REG_ID(rs);
    result = regs[rs];
    goto done;
}

TARGET(OP_RET_VOID) {
    result = none_value;
    goto done;
}

TARGET(OP_RET_INT_IMM) {
    int ti = I_VAL(inst, 16, 8);
    imm = I_SVAL(inst, 0, 16);

    ASSERT((ti & 0b1100) == 0b1000);

    result.ival = imm;
    result.tag = ti;
    goto done;
}

/* warm instructions */

/* Int Logical Branches */

TARGET(OP_JMP_INT_EQ_IMM) {
    rs = I_VAL(inst, 16, 8);
    imm = I_SVAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    ASSERT(is_int(regs + rs) || is_bool(regs + rs));

    if (regs[rs].ival == imm) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_INT_NE_IMM) {
    rs = I_VAL(inst, 16, 8);
    imm = I_SVAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    ASSERT(is_int(regs + rs) || is_bool(regs + rs));

    if (regs[rs].ival != imm) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_INT_EQ) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_INT64 || regs[rs].tag == TAG_UINT64 || is_bool(regs + rs));
    ASSERT(regs[rs].tag == regs[rt].tag);

    if (regs[rs].ival == regs[rt].ival) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_INT_NE) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_INT64 || regs[rs].tag == TAG_UINT64 || is_bool(regs + rs));
    ASSERT(regs[rs].tag == regs[rt].tag);

    if (regs[rs].ival != regs[rt].ival) {
        pc += off;
    }
    DISPATCH();
}

/* Ref Logical Branches */

TARGET(OP_JMP_REF_EQ_NULL) {
    rs = I_VAL(inst, 16, 8);
    off = I_SVAL(inst, 0, 16);

    CHECK_REG_ID(rs);

    if (is_none(&regs[rs])) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_REF_NE_NULL) {
    rs = I_VAL(inst, 16, 8);
    off = I_SVAL(inst, 0, 16);

    CHECK_REG_ID(rs);

    if (!is_none(&regs[rs])) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_REF_NE_NULL) {
    rd = I_VAL(inst, 12, 12);
    rs = I_VAL(inst, 0, 12);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd] = is_none(regs + rs) ? BOOL_FALSE : BOOL_TRUE;
    DISPATCH();
}

TARGET(OP_REF_EQ_NULL) {
    rd = I_VAL(inst, 12, 12);
    rs = I_VAL(inst, 0, 12);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd] = is_none(regs + rs) ? BOOL_TRUE : BOOL_FALSE;
    DISPATCH();
}

TARGET(OP_REF_EQ) {
    OP_NYI(OP_REF_EQ);
}

TARGET(OP_REF_NE) {
    OP_NYI(OP_REF_NE);
}

/* Calls */

TARGET(OP_CALL) {
    int flg = I_VAL(inst, 20, 4);
    rd = I_VAL(inst, 8, 12);
    imm = I_VAL(inst, 0, 8);

    if (flg == 1) {
        uint32_t index = *pc++;
        ImportEntry *e = IMPORT_ENTRY(index);
        ASSERT(e->kind == IMPORT_KIND_FUNC || e->kind == IMPORT_KIND_METHOD);
        Object *target = e->address;
        ASSERT(target);
        TValue val = obj_value(target);
        TValue ret = kl_do_call(&val, ks->stack_top, imm);
        if (rd != 0xFFFu) {
            ASSERT(rd < max_regs);
            regs[rd] = ret;
        }
        DISPATCH();
    }

    if (flg == 2) {
        uint32_t index = *pc++;
        TValue *callable = ks->stack_top;
        ASSERT(is_intf(callable));
        IntfTable *itab = callable->itab;
        ASSERT(index < itab->num_funcs);
        Object *fn = itab->methods[index];
        ASSERT(fn);
        ASSERT(IS_CFUNC(fn) || IS_CODE(fn));

        TValue val = obj_value(fn);
        TValue ret = kl_do_call(&val, ks->stack_top, imm);
        if (rd != 0xFFFu) {
            ASSERT(rd < max_regs);
            regs[rd] = ret;
        }
        DISPATCH();
    }

    int32_t local_index = *(int32_t *)pc++;
    // uint32_t *target_pc = pc + local_index;
    // ASSERT(target_pc < codes + code->cs.code_size);
    // uint32_t f_idx = *target_pc;
    ASSERT(local_index >= 0 && local_index < entry_size);
    FuncEntry *e = ENTRY(local_index);
    Object *obj = e->obj;
    TValue ret;

    if (IS_CFUNC(obj)) {
        CFuncObject *cfunc = (CFuncObject *)obj;
        NativeFunc func = cfunc->func;
        Object *owner = cfunc->owner;
        TValue *args = ks->stack_top;
        if (IS_MODULE(owner)) {
            TValue val = obj_value(obj);
            ret = func(&val, args, imm);
        } else {
            ASSERT(IS_TYPE(owner, &type_type));
            ASSERT(imm > 0);
            ret = func(args, args + 1, imm - 1);
        }
    } else {
        ASSERT(IS_CODE(e->obj));
        TValue val = obj_value(e->obj);
        ret = kl_eval_code(&val, NULL, 0);
    }

    if (rd != 0xFFFu) {
        ASSERT(rd < max_regs);
        regs[rd] = ret;
    }
    DISPATCH();
}

TARGET(OP_TAIL_CALL) {
#ifndef NDEBUG
    rd = I_VAL(inst, 8, 12);
    int flg = I_VAL(inst, 20, 4);
#endif

    ASSERT(rd == 0xFFFu); // tail call does not have return value
    ASSERT(flg == 0);

    pc = codes + code->cs.start_pc;
    goto main_loop;
}

TARGET(OP_SET_FIELD) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    off = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    Object *obj = to_obj(regs + rd);
    InstObject *inst_obj = (InstObject *)obj;
    ASSERT(off < NR_FIELDS(inst_obj));
    inst_obj->fields[off] = regs[rs];
    DISPATCH();
}

TARGET(OP_GET_FIELD) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    off = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    Object *obj = to_obj(regs + rs);
    InstObject *inst_obj = (InstObject *)obj;
    ASSERT(off < NR_FIELDS(inst_obj));
    regs[rd] = inst_obj->fields[off];
    DISPATCH();
}

TARGET(OP_GET_FIELD_EXT) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    off = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    ImportEntry *e = IMPORT_ENTRY(off);
    ASSERT(e->kind == IMPORT_KIND_FIELD);
    FieldObject *fld = e->address;
    ASSERT(fld);
    int offset = fld->index;
    Object *obj = to_obj(regs + rs);
    if (fld->kind == FIELD_OFFSET) {
        regs[rd] = *(TValue *)((char *)obj + offset);
    } else {
        ASSERT(fld->kind == FIELD_INDEX);
        InstObject *inst_obj = (InstObject *)obj;
        regs[rd] = inst_obj->fields[offset];
    }
    DISPATCH();
}

TARGET(OP_NEW) {
    rd = I_VAL(inst, 16, 8);
    idx = I_VAL(inst, 0, 16);

    CHECK_REG_ID(rd);
    CHECK_TYPE_INDEX(idx);

    TypeObject *tp = TYPE(idx);
    Object *obj = kl_new_instance(tp);
    regs[rd] = obj_value(obj);
    DISPATCH();
}

TARGET(OP_NEW_EXT) {
    rd = I_VAL(inst, 16, 8);
    off = I_VAL(inst, 0, 16);

    CHECK_REG_ID(rd);

    ImportEntry *e = IMPORT_ENTRY(off);
    ASSERT(e->kind == IMPORT_KIND_TYPE);
    TypeObject *tp = e->address;
    ASSERT(tp);

    Object *obj = kl_new_instance(tp);
    regs[rd] = obj_value(obj);
    DISPATCH();
}

TARGET(OP_BUILD_INTERN) {
    rd = I_VAL(inst, 16, 8);
    int tag = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);

    Object *obj = do_build_intern(ks->stack_top, tag, imm);
    if (rd != 0xFFFu) regs[rd] = obj_value(obj);
    DISPATCH();
}

TARGET(OP_MOVE_TRUE) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    if (regs[rs].ival) {
        regs[rd] = regs[rt];
    }
    DISPATCH();
}

/* Comparisons */

TARGET(OP_INT_CMPEQ) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_INT64 || regs[rs].tag == TAG_UINT64 || regs[rs].tag == TAG_BOOL);
    ASSERT(regs[rs].tag == regs[rt].tag);

    regs[rd].ival = regs[rs].ival == regs[rt].ival;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_INT_CMPEQ_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_INT(rs);

    regs[rd].ival = regs[rs].ival == imm;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_UINT_CMPEQ_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    regs[rd].ival = ((uint64_t)regs[rs].ival == (uint64_t)imm);
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_INT_CMPNE) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_INT64 || regs[rs].tag == TAG_UINT64 || regs[rs].tag == TAG_BOOL);
    ASSERT(regs[rs].tag == regs[rt].tag);

    regs[rd].ival = regs[rs].ival != regs[rt].ival;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_INT_CMPNE_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_INT(rs);

    regs[rd].ival = regs[rs].ival != imm;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_UINT_CMPNE_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    regs[rd].ival = ((uint64_t)regs[rs].ival != (uint64_t)imm);
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_INT_CMPLT) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_INT(rs);
    CHECK_IS_INT(rt);

    regs[rd].ival = regs[rs].ival < regs[rt].ival;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_INT_CMPLT_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_INT(rs);

    regs[rd].ival = regs[rs].ival < imm;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_INT_CMPLE) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_INT(rs);
    CHECK_IS_INT(rt);

    regs[rd].ival = regs[rs].ival <= regs[rt].ival;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_INT_CMPLE_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_INT(rs);

    regs[rd].ival = regs[rs].ival <= imm;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_INT_CMPGT) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_INT(rs);
    CHECK_IS_INT(rt);

    regs[rd].ival = regs[rs].ival > regs[rt].ival;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_INT_CMPGT_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_INT(rs);

    regs[rd].ival = regs[rs].ival > imm;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_INT_CMPGE) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_INT(rs);
    CHECK_IS_INT(rt);

    regs[rd].ival = regs[rs].ival >= regs[rt].ival;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_INT_CMPGE_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_INT(rs);

    regs[rd].ival = regs[rs].ival >= imm;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

/* Bitwise & Logic */

TARGET(OP_INT_AND) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd].ival = regs[rs].ival & regs[rt].ival;
    regs[rd].tag = regs[rs].tag | 0b11;

    DISPATCH();
}

TARGET(OP_INT_AND_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival & imm;
    regs[rd].tag = TAG_INT64;

    DISPATCH();
}

TARGET(OP_UINT_AND_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival & (uint64_t)imm;
    regs[rd].tag = TAG_UINT64;

    DISPATCH();
}

TARGET(OP_INT_OR) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd].ival = regs[rs].ival | regs[rt].ival;
    regs[rd].tag = regs[rs].tag | 0b11;

    DISPATCH();
}

TARGET(OP_INT_OR_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival | imm;
    regs[rd].tag = TAG_INT64;

    DISPATCH();
}

TARGET(OP_UINT_OR_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival | (uint64_t)imm;
    regs[rd].tag = TAG_UINT64;

    DISPATCH();
}

TARGET(OP_INT_XOR) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd].ival = regs[rs].ival ^ regs[rt].ival;
    regs[rd].tag = regs[rs].tag | 0b11;

    DISPATCH();
}

TARGET(OP_INT_XOR_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival ^ imm;
    regs[rd].tag = TAG_INT64;

    DISPATCH();
}

TARGET(OP_UINT_XOR_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival ^ (uint64_t)imm;
    regs[rd].tag = TAG_UINT64;

    DISPATCH();
}

TARGET(OP_INT_SHL) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd].ival = regs[rs].ival << regs[rt].ival;
    regs[rd].tag = regs[rs].tag | 0b11;

    DISPATCH();
}

TARGET(OP_INT_SHL_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival << imm;
    regs[rd].tag = TAG_INT64;

    DISPATCH();
}

TARGET(OP_UINT_SHL_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival << (uint64_t)imm;
    regs[rd].tag = TAG_UINT64;

    DISPATCH();
}

TARGET(OP_INT_SHR) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd].ival = regs[rs].ival >> regs[rt].ival;
    regs[rd].tag = regs[rs].tag | 0b11;

    DISPATCH();
}

TARGET(OP_INT_SHR_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival >> imm;
    regs[rd].tag = TAG_INT64;

    DISPATCH();
}

TARGET(OP_LAND) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    ASSERT(regs[rs].tag == TAG_BOOL);
    ASSERT(regs[rt].tag == TAG_BOOL);

    regs[rd].ival = regs[rs].ival && regs[rt].ival;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_LOR) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    ASSERT(regs[rs].tag == TAG_BOOL);
    ASSERT(regs[rt].tag == TAG_BOOL);

    regs[rd].ival = regs[rs].ival || regs[rt].ival;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_LNOT) {
    rd = I_VAL(inst, 12, 12);
    rs = I_VAL(inst, 0, 12);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    ASSERT(regs[rs].tag == TAG_BOOL);

    regs[rd].ival = !regs[rs].ival;
    regs[rd].tag = TAG_BOOL;
    DISPATCH();
}

TARGET(OP_JMP_TRUE) {
    rs = I_VAL(inst, 16, 8);
    off = I_SVAL(inst, 0, 16);

    CHECK_REG_ID(rs);
    ASSERT(regs[rs].tag == TAG_BOOL);

    if (regs[rs].ival != 0) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_FALSE) {
    rs = I_VAL(inst, 16, 8);
    off = I_SVAL(inst, 0, 16);

    CHECK_REG_ID(rs);
    ASSERT(regs[rs].tag == TAG_BOOL);

    if (regs[rs].ival == 0) {
        pc += off;
    }
    DISPATCH();
}

/* Complex Int Arithmetic */

TARGET(OP_INT_MUL) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd].ival = regs[rs].ival * regs[rt].ival;
    regs[rd].tag = regs[rs].tag | 0b11;

    DISPATCH();
}

TARGET(OP_INT_MUL_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival * imm;
    regs[rd].tag = TAG_INT64;

    DISPATCH();
}

TARGET(OP_UINT_MUL_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival * (uint64_t)imm;
    regs[rd].tag = TAG_UINT64;

    DISPATCH();
}

TARGET(OP_INT_DIV) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd].ival = regs[rs].ival / regs[rt].ival;
    regs[rd].tag = regs[rs].tag | 0b11;

    DISPATCH();
}

TARGET(OP_INT_DIV_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival / imm;
    regs[rd].tag = TAG_INT64;

    DISPATCH();
}

TARGET(OP_INT_MOD) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd].ival = regs[rs].ival % regs[rt].ival;
    regs[rd].tag = regs[rs].tag | 0b11;

    DISPATCH();
}

TARGET(OP_INT_MOD_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_INT(rs);

    regs[rd].ival = regs[rs].ival % imm;
    regs[rd].tag = TAG_INT64;

    DISPATCH();
}

/* Number Operations */

TARGET(OP_NUM_EQ) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    TValue *v1 = regs + rs;
    TValue *v2 = regs + rt;
    TValue ret;

    if (is_int64(v1) && is_int64(v2)) {
        // int64 doesn't have cmp function
        ret = bool_value(v1->ival == v2->ival);
    } else if (is_float64(v1) && is_float64(v2)) {
        // float64 doesn't have cmp function
        ret = bool_value(v1->fval == v2->fval);
    } else if (is_uint64(v1) && is_uint64(v2)) {
        // uint64 doesn't have cmp function
        ret = bool_value((uint64_t)v1->ival == (uint64_t)v2->ival);
    } else if (is_bool(v1) && is_bool(v2)) {
        // bool doesn't have cmp function
        ret = bool_value(v1->ival == v2->ival);
    } else {
        ret = kl_slot_call_one_arg(regs + rs, regs + rt, CMP_EQ);
    }

    regs[rd] = ret;

    DISPATCH();
}

TARGET(OP_NUM_NE) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    TValue *v1 = regs + rs;
    TValue *v2 = regs + rt;
    TValue ret;

    if (is_int64(v1) && is_int64(v2)) {
        // int64 doesn't have cmp function
        ret = bool_value(v1->ival != v2->ival);
    } else if (is_float64(v1) && is_float64(v2)) {
        // float64 doesn't have cmp function
        ret = bool_value(v1->fval != v2->fval);
    } else if (is_uint64(v1) && is_uint64(v2)) {
        // uint64 doesn't have cmp function
        ret = bool_value((uint64_t)v1->ival != (uint64_t)v2->ival);
    } else if (is_bool(v1) && is_bool(v2)) {
        // bool doesn't have cmp function
        ret = bool_value(v1->ival != v2->ival);
    } else {
        ret = kl_slot_call_one_arg(regs + rs, regs + rt, CMP_NE);
    }

    regs[rd] = ret;

    DISPATCH();
}

TARGET(OP_NUM_LT) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    TValue *v1 = regs + rs;
    TValue *v2 = regs + rt;
    TValue ret;

    if (is_int64(v1) && is_int64(v2)) {
        // int64 doesn't have cmp function
        ret = bool_value(v1->ival < v2->ival);
    } else if (is_float64(v1) && is_float64(v2)) {
        // float64 doesn't have cmp function
        ret = bool_value(v1->fval < v2->fval);
    } else if (is_uint64(v1) && is_uint64(v2)) {
        // uint64 doesn't have cmp function
        ret = bool_value((uint64_t)v1->ival < (uint64_t)v2->ival);
    } else {
        ret = kl_slot_call_one_arg(regs + rs, regs + rt, CMP_LT);
    }

    regs[rd] = ret;

    DISPATCH();
}

TARGET(OP_NUM_LE) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    TValue *v1 = regs + rs;
    TValue *v2 = regs + rt;
    TValue ret;

    if (is_int64(v1) && is_int64(v2)) {
        // int64 doesn't have cmp function
        ret = bool_value(v1->ival <= v2->ival);
    } else if (is_float64(v1) && is_float64(v2)) {
        // float64 doesn't have cmp function
        ret = bool_value(v1->fval <= v2->fval);
    } else if (is_uint64(v1) && is_uint64(v2)) {
        // uint64 doesn't have cmp function
        ret = bool_value((uint64_t)v1->ival <= (uint64_t)v2->ival);
    } else {
        ret = kl_slot_call_one_arg(regs + rs, regs + rt, CMP_LE);
    }

    regs[rd] = ret;

    DISPATCH();
}

TARGET(OP_NUM_GT) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    TValue *v1 = regs + rs;
    TValue *v2 = regs + rt;
    TValue ret;

    if (is_int64(v1) && is_int64(v2)) {
        // int64 doesn't have cmp function
        ret = bool_value(v1->ival > v2->ival);
    } else if (is_float64(v1) && is_float64(v2)) {
        // float64 doesn't have cmp function
        ret = bool_value(v1->fval > v2->fval);
    } else if (is_uint64(v1) && is_uint64(v2)) {
        // uint64 doesn't have cmp function
        ret = bool_value((uint64_t)v1->ival > (uint64_t)v2->ival);
    } else {
        ret = kl_slot_call_one_arg(regs + rs, regs + rt, CMP_GT);
    }

    regs[rd] = ret;

    DISPATCH();
}

TARGET(OP_NUM_GE) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    TValue *v1 = regs + rs;
    TValue *v2 = regs + rt;
    TValue ret;

    if (is_int64(v1) && is_int64(v2)) {
        // int64 doesn't have cmp function
        ret = bool_value(v1->ival >= v2->ival);
    } else if (is_float64(v1) && is_float64(v2)) {
        // float64 doesn't have cmp function
        ret = bool_value(v1->fval >= v2->fval);
    } else if (is_uint64(v1) && is_uint64(v2)) {
        // uint64 doesn't have cmp function
        ret = bool_value((uint64_t)v1->ival >= (uint64_t)v2->ival);
    } else {
        ret = kl_slot_call_one_arg(regs + rs, regs + rt, CMP_GE);
    }

    regs[rd] = ret;

    DISPATCH();
}

TARGET(OP_NUM_ADD) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd] = kl_slot_call_one_arg(regs + rs, regs + rt, SLOT_ADD);

    DISPATCH();
}

TARGET(OP_NUM_SUB) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd] = kl_slot_call_one_arg(regs + rs, regs + rt, SLOT_SUB);

    DISPATCH();
}

TARGET(OP_NUM_MUL) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd] = kl_slot_call_one_arg(regs + rs, regs + rt, SLOT_MUL);

    DISPATCH();
}

TARGET(OP_NUM_DIV) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd] = kl_slot_call_one_arg(regs + rs, regs + rt, SLOT_DIV);

    DISPATCH();
}

TARGET(OP_NUM_MOD) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd] = kl_slot_call_one_arg(regs + rs, regs + rt, SLOT_MOD);

    DISPATCH();
}

/* Sequence&Map Operations */

TARGET(OP_SEQ_GET) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    regs[rd] = kl_slot_call_one_arg(regs + rs, regs + rt, SLOT_GET_ITEM);

    DISPATCH();
}

TARGET(OP_SEQ_GET_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    TValue index = int64_value(imm);
    regs[rd] = kl_slot_call_one_arg(regs + rs, &index, SLOT_GET_ITEM);

    DISPATCH();
}

TARGET(OP_SEQ_LEN) {
    rd = I_VAL(inst, 12, 12);
    rs = I_VAL(inst, 0, 12);

    CHECK_REG_ID(rs);

    TValue ret = kl_slot_call_no_arg(regs + rs, SLOT_LEN);
    if (rd != 0xFFFu) regs[rd] = ret;

    DISPATCH();
}

TARGET(OP_SEQ_SET) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

    kl_slot_call_two_args(regs + rd, regs + rt, regs + rs, SLOT_SET_ITEM);

    DISPATCH();
}

TARGET(OP_SEQ_SET_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    TValue index = int64_value(imm);
    kl_slot_call_two_args(regs + rd, &index, regs + rs, SLOT_SET_ITEM);

    DISPATCH();
}

/* Float Basic */

TARGET(OP_FLOAT_ADD) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    regs[rd].fval = regs[rs].fval + regs[rt].fval;
    regs[rd].tag = TAG_FLOAT64;

    DISPATCH();
}

TARGET(OP_FLOAT_SUB) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    regs[rd].fval = regs[rs].fval - regs[rt].fval;
    regs[rd].tag = TAG_FLOAT64;

    DISPATCH();
}

TARGET(OP_FLOAT_MUL) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    regs[rd].fval = regs[rs].fval * regs[rt].fval;
    regs[rd].tag = TAG_FLOAT64;

    DISPATCH();
}

TARGET(OP_JMP_FLOAT_EQ) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    if (regs[rs].fval == regs[rt].fval) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_FLOAT_NE) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    if (regs[rs].fval != regs[rt].fval) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_FLOAT_LT) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    if (regs[rs].fval < regs[rt].fval) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_FLOAT_LE) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    if (regs[rs].fval <= regs[rt].fval) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_FLOAT_GT) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    if (regs[rs].fval > regs[rt].fval) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_FLOAT_GE) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    if (regs[rs].fval >= regs[rt].fval) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_MAKE_INTF) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    idx = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    TypeObject *tp = kl_typeof(regs + rs);
    IntfTable *itab = vector_get_ptr(&tp->itables, idx);
    TValue v = regs[rs];
    v.itab = itab;
    regs[rd] = v;

    DISPATCH();
}

TARGET(OP_UPCAST_INTF) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    idx = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    TValue v = regs[rs];
    ASSERT(is_intf(&v));
    IntfTable *itab = v.itab;
    ASSERT(idx < itab->num_parents);
    v.itab = itab->parents[idx];
    regs[rd] = v;

    DISPATCH();
}

/* cold instructions */

/* Unsigned Ops */

TARGET(OP_LOAD_UINT_IMM) {
    rd = I_VAL(inst, 16, 8);
    int ti = I_VAL(inst, 12, 4);
    imm = I_VAL(inst, 0, 12);

    CHECK_REG_ID(rd);
    ASSERT((ti & 0b1100) == 0b1100);

    regs[rd].tag = ti;
    regs[rd].ival = imm;
    DISPATCH();
}

TARGET(OP_RET_UINT_IMM) {
    int ti = I_VAL(inst, 16, 8);
    imm = I_VAL(inst, 0, 16);

    ASSERT((ti & 0b1100) == 0b1100);

    result.ival = imm;
    result.tag = ti;
    goto done;
}

TARGET(OP_UINT_DIV) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_UINT(rs);
    CHECK_IS_UINT(rt);

    regs[rd].ival = (uint64_t)regs[rs].ival / (uint64_t)regs[rt].ival;
    regs[rd].tag = TAG_UINT64;

    DISPATCH();
}

TARGET(OP_UINT_DIV_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival / (uint64_t)imm;
    regs[rd].tag = TAG_UINT64;

    DISPATCH();
}

TARGET(OP_UINT_MOD) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_UINT(rs);
    CHECK_IS_UINT(rt);

    regs[rd].ival = (uint64_t)regs[rs].ival % (uint64_t)regs[rt].ival;
    regs[rd].tag = TAG_UINT64;

    DISPATCH();
}

TARGET(OP_UINT_MOD_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival % (uint64_t)imm;
    regs[rd].tag = TAG_UINT64;

    DISPATCH();
}

TARGET(OP_UINT_SHR) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_UINT(rs);
    CHECK_IS_UINT(rt);

    regs[rd].ival = (uint64_t)regs[rs].ival >> (uint64_t)regs[rt].ival;
    regs[rd].tag = TAG_UINT64;

    DISPATCH();
}

TARGET(OP_UINT_SHR_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival >> (uint64_t)imm;
    regs[rd].tag = TAG_UINT64;

    DISPATCH();
}

TARGET(OP_UINT_CMPLT) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_UINT(rs);
    CHECK_IS_UINT(rt);

    regs[rd].ival = (uint64_t)regs[rs].ival < (uint64_t)regs[rt].ival;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

TARGET(OP_UINT_CMPLT_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival < (uint64_t)imm;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

TARGET(OP_UINT_CMPLE) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_UINT(rs);
    CHECK_IS_UINT(rt);

    regs[rd].ival = (uint64_t)regs[rs].ival <= (uint64_t)regs[rt].ival;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

TARGET(OP_UINT_CMPLE_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival <= (uint64_t)imm;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

TARGET(OP_UINT_CMPGT) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_UINT(rs);
    CHECK_IS_UINT(rt);

    regs[rd].ival = (uint64_t)regs[rs].ival > (uint64_t)regs[rt].ival;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

TARGET(OP_UINT_CMPGT_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival > (uint64_t)imm;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

TARGET(OP_UINT_CMPGE) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_UINT(rs);
    CHECK_IS_UINT(rt);

    regs[rd].ival = (uint64_t)regs[rs].ival >= (uint64_t)regs[rt].ival;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

TARGET(OP_UINT_CMPGE_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    regs[rd].ival = (uint64_t)regs[rs].ival >= (uint64_t)imm;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

TARGET(OP_JMP_UINT_LT) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_UINT(rs);
    CHECK_IS_UINT(rt);

    if ((uint64_t)regs[rs].ival < (uint64_t)regs[rt].ival) {
        pc += off;
    }

    DISPATCH();
}

TARGET(OP_JMP_UINT_LT_IMM) {
    rs = I_VAL(inst, 16, 8);
    imm = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    if ((uint64_t)regs[rs].ival < (uint64_t)imm) {
        pc += off;
    }

    DISPATCH();
}

TARGET(OP_JMP_UINT_LE) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_UINT(rs);
    CHECK_IS_UINT(rt);

    if ((uint64_t)regs[rs].ival <= (uint64_t)regs[rt].ival) {
        pc += off;
    }

    DISPATCH();
}

TARGET(OP_JMP_UINT_LE_IMM) {
    rs = I_VAL(inst, 16, 8);
    imm = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    if ((uint64_t)regs[rs].ival <= (uint64_t)imm) {
        pc += off;
    }

    DISPATCH();
}

TARGET(OP_JMP_UINT_GT) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_UINT(rs);
    CHECK_IS_UINT(rt);

    if ((uint64_t)regs[rs].ival > (uint64_t)regs[rt].ival) {
        pc += off;
    }

    DISPATCH();
}

TARGET(OP_JMP_UINT_GT_IMM) {
    rs = I_VAL(inst, 16, 8);
    imm = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    if ((uint64_t)regs[rs].ival > (uint64_t)imm) {
        pc += off;
    }

    DISPATCH();
}

TARGET(OP_JMP_UINT_GE) {
    rs = I_VAL(inst, 16, 8);
    rt = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    CHECK_IS_UINT(rs);
    CHECK_IS_UINT(rt);

    if ((uint64_t)regs[rs].ival >= (uint64_t)regs[rt].ival) {
        pc += off;
    }

    DISPATCH();
}

TARGET(OP_JMP_UINT_GE_IMM) {
    rs = I_VAL(inst, 16, 8);
    imm = I_VAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);
    CHECK_IS_UINT(rs);

    if ((uint64_t)regs[rs].ival >= (uint64_t)imm) {
        pc += off;
    }

    DISPATCH();
}

/* global */
TARGET(OP_GLOBAL_SET) {
    rd = I_VAL(inst, 12, 12);
    idx = I_VAL(inst, 0, 12);

    CHECK_REG_ID(rd);
    ASSERT(idx < m->num_values);

    m->values[idx] = regs[rd];

    DISPATCH();
}

TARGET(OP_GLOBAL_GET) {
    rs = I_VAL(inst, 12, 12);
    idx = I_VAL(inst, 0, 12);

    CHECK_REG_ID(rs);
    ASSERT(idx < m->num_values);

    regs[rs] = m->values[idx];

    DISPATCH();
}

TARGET(OP_GLOBAL_SET_EXT) {
    rs = I_VAL(inst, 16, 8);
    idx = I_VAL(inst, 0, 16);

    CHECK_REG_ID(rs);

    ImportEntry *e = IMPORT_ENTRY(idx);
    ASSERT(e->kind == IMPORT_KIND_GLOBAL);
    GlobalObject *gobj = e->address;
    ASSERT(gobj);
    int index = gobj->index;
    ModuleObject *ext = (ModuleObject *)gobj->module;
    ASSERT(index < ext->num_values);
    ext->values[index] = regs[rs];

    DISPATCH();
}

TARGET(OP_GLOBAL_GET_EXT) {
    rd = I_VAL(inst, 16, 8);
    idx = I_VAL(inst, 0, 16);

    CHECK_REG_ID(rd);

    ImportEntry *e = IMPORT_ENTRY(idx);
    ASSERT(e->kind == IMPORT_KIND_GLOBAL);
    GlobalObject *gobj = e->address;
    ASSERT(gobj);
    int index = gobj->index;
    ModuleObject *ext = (ModuleObject *)gobj->module;
    ASSERT(index < ext->num_values);
    regs[rd] = ext->values[index];

    DISPATCH();
}

/* Float Complex */

TARGET(OP_FLOAT_DIV) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    regs[rd].fval = regs[rs].fval / regs[rt].fval;
    regs[rd].tag = TAG_FLOAT64;

    DISPATCH();
}

TARGET(OP_FLOAT_MOD) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    regs[rd].fval = fmod(regs[rs].fval, regs[rt].fval);
    regs[rd].tag = TAG_FLOAT64;

    DISPATCH();
}

TARGET(OP_FLOAT_CMPEQ) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    regs[rd].ival = regs[rs].fval == regs[rt].fval;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

TARGET(OP_FLOAT_CMPNE) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    regs[rd].ival = regs[rs].fval != regs[rt].fval;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

TARGET(OP_FLOAT_CMPLT) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    regs[rd].ival = regs[rs].fval < regs[rt].fval;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

TARGET(OP_FLOAT_CMPLE) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    regs[rd].ival = regs[rs].fval <= regs[rt].fval;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

TARGET(OP_FLOAT_CMPGT) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    regs[rd].ival = regs[rs].fval > regs[rt].fval;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

TARGET(OP_FLOAT_CMPGE) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);
    ASSERT(regs[rs].tag == TAG_FLOAT64);
    ASSERT(regs[rt].tag == TAG_FLOAT64);

    regs[rd].ival = regs[rs].fval >= regs[rt].fval;
    regs[rd].tag = TAG_BOOL;

    DISPATCH();
}

/* miscellaneous instructions */

TARGET(OP_INT_NOT) {
    rd = I_VAL(inst, 12, 12);
    rs = I_VAL(inst, 0, 12);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    ASSERT(regs[rs].tag == TAG_INT64 || regs[rs].tag == TAG_UINT64);

    regs[rd].ival = ~regs[rs].ival;
    regs[rd].tag = regs[rs].tag;

    DISPATCH();
}

TARGET(OP_INT_NEG) {
    rd = I_VAL(inst, 12, 12);
    rs = I_VAL(inst, 0, 12);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    ASSERT(regs[rs].tag == TAG_INT64);

    regs[rd].ival = -regs[rs].ival;
    regs[rd].tag = TAG_INT64;

    DISPATCH();
}

TARGET(OP_FLOAT_NEG) {
    rd = I_VAL(inst, 12, 12);
    rs = I_VAL(inst, 0, 12);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    ASSERT(regs[rs].tag == TAG_FLOAT64);

    regs[rd].fval = -regs[rs].fval;
    regs[rd].tag = TAG_FLOAT64;

    DISPATCH();
}

TARGET(OP_LOAD_TAG) {
    rd = I_VAL(inst, 8, 12);
    int tag = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);

    regs[rd] = TAG_VALUE(tag);
    DISPATCH();
}

TARGET(OP_RET_TAG) {
    int tag = I_VAL(inst, 0, 8);
    result = TAG_VALUE(tag);
    goto done;
}

TARGET(OP_RET_CONST) {
    idx = I_VAL(inst, 0, 16);
    result = *CP(idx);
    goto done;
}

/* Type Conversion */

TARGET(OP_INT_CAST) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    int flag = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    int mode = flag & 0x3;
    int dst_ti = (flag >> 2) & 0x3F;

    do_int_cast(regs, rd, rs, mode, dst_ti);
    DISPATCH();
}

TARGET(OP_FLOAT_CAST) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    int flag = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    int mode = flag & 0x3;
    int dst_ti = (flag >> 2) & 0x3F;

    do_float_cast(regs, rd, rs, mode, dst_ti);
    DISPATCH();
}

TARGET(OP_FLOAT_TO_INT) {
    OP_NYI(OP_FLOAT_TO_INT);
}

TARGET(OP_INT_TO_FLOAT) {
    OP_NYI(OP_INT_TO_FLOAT);
}

TARGET(OP_NOP) {
    /* do nothing, just move to next instruction */
    DISPATCH();
}
