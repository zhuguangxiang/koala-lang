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
    imm = I_SVAL(inst, 0, 16);

    CHECK_REG_ID(rd);

    regs[rd].tag = TAG_INT64;
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
    regs[rd].tag = regs[rs].tag;
    DISPATCH();
}

TARGET(OP_INT_ADD_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival + imm;
    regs[rd].tag = regs[rs].tag;
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
    regs[rd].tag = regs[rs].tag;
    DISPATCH();
}

TARGET(OP_INT_SUB_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival - imm;
    regs[rd].tag = regs[rs].tag;
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
    imm = I_SVAL(inst, 0, 16);
    result.ival = imm;
    result.tag = TAG_INT64;
    goto done;
}

/* warm instructions */

/* Int Logical Branches */

TARGET(OP_JMP_INT_EQ_IMM) {
    rs = I_VAL(inst, 16, 8);
    imm = I_SVAL(inst, 8, 8);
    off = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rs);

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

    if (regs[rs].ival != regs[rt].ival) {
        pc += off;
    }
    DISPATCH();
}

/* Calls */

TARGET(OP_CALL) {
    int flg = I_VAL(inst, 20, 4);
    rd = I_VAL(inst, 8, 12);
    imm = I_VAL(inst, 0, 8);

    if (flg == 1) {
        uint32_t index = *pc++;
        ImportEntry *e = IMPORT_ENTRY(index);
        ASSERT(e->kind == IMPORT_KIND_FUNC);
        Object *target = e->address;
        ASSERT(target);
        TValue val = obj_value(target);
        TValue ret = kl_do_call(&val, ks->stack_top, imm);
        if (rd != 0xFFFu) {
            ASSERT(rd < max_regs);
            regs[rd] = ret;
        }
        // if (has_push) {
        //     SHRINK(imm);
        //     has_push = 0;
        // }
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
        TValue val = obj_value(obj);
        NativeFunc func = cfunc->func;
        ret = func(&val, ks->stack_top, imm);
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
    int flg = I_VAL(inst, 20, 4);
    // imm = I_VAL(inst, 0, 8);

    // if (flg == 1) {
    //     // external function call
    //     NYI();
    //     goto ext_tailcall;
    // }

    if (flg == 3) {
        pc = codes + code->cs.start_pc;
        goto main_loop;
    }

    // local function call
    int32_t local_index = *(int32_t *)pc++;
    ASSERT(local_index >= 0 && local_index < entry_size);
    FuncEntry *e = ENTRY(local_index);
    Object *obj = e->obj;

    if (obj == (Object *)cf->code) {
        pc = codes + code->cs.start_pc;
        goto main_loop;
    }

    TValue ret;
    if (IS_CFUNC(obj)) {
        // CFuncObject *cfunc = (CFuncObject *)obj;
        // TValue val = obj_value(obj);
        // NativeFunc func = cfunc->func;
        // ret = func(&val, regs, imm);
        // // TODO: tail call does not have return value.
        // // if (rd != 0xFFFu) {
        // //     ASSERT(rd < cf->nlocals);
        // //     regs[rd] = ret;
        // // }
        // SHRINK(imm);
        DISPATCH();
    } else {
        ASSERT(IS_CODE(e->obj));
        cf->code = (CodeObject *)e->obj;
        cf->nlocals = cf->code->cs.nlocals;
        ks->stack_top = cf->locals + cf->nlocals;
        goto local_tailcall;
    }
}

/* Comparisons*/

TARGET(OP_INT_CMPEQ) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    rt = I_VAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    CHECK_REG_ID(rt);

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

    regs[rd].ival = regs[rs].ival == imm;
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

    regs[rd].ival = regs[rs].ival != imm;
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
    regs[rd].tag = regs[rs].tag;

    DISPATCH();
}

TARGET(OP_INT_AND_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival & imm;
    regs[rd].tag = regs[rs].tag;

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
    regs[rd].tag = regs[rs].tag;

    DISPATCH();
}

TARGET(OP_INT_OR_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival | imm;
    regs[rd].tag = regs[rs].tag;

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
    regs[rd].tag = regs[rs].tag;

    DISPATCH();
}

TARGET(OP_INT_XOR_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival ^ imm;
    regs[rd].tag = regs[rs].tag;

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
    regs[rd].tag = regs[rs].tag;

    DISPATCH();
}

TARGET(OP_INT_SHL_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival << imm;
    regs[rd].tag = regs[rs].tag;

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
    regs[rd].tag = regs[rs].tag;

    DISPATCH();
}

TARGET(OP_INT_SHR_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival >> imm;
    regs[rd].tag = regs[rs].tag;

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

    if (regs[rs].bval != 0) {
        pc += off;
    }
    DISPATCH();
}

TARGET(OP_JMP_FALSE) {
    rs = I_VAL(inst, 16, 8);
    off = I_SVAL(inst, 0, 16);

    CHECK_REG_ID(rs);
    ASSERT(regs[rs].tag == TAG_BOOL);

    if (regs[rs].bval == 0) {
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
    regs[rd].tag = regs[rs].tag;

    DISPATCH();
}

TARGET(OP_INT_MUL_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival * imm;
    regs[rd].tag = regs[rs].tag;

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
    regs[rd].tag = regs[rs].tag;

    DISPATCH();
}

TARGET(OP_INT_DIV_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = regs[rs].ival / imm;
    regs[rd].tag = regs[rs].tag;

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
    regs[rd].tag = regs[rs].tag;

    DISPATCH();
}

TARGET(OP_INT_MOD_IMM) {
    rd = I_VAL(inst, 16, 8);
    rs = I_VAL(inst, 8, 8);
    imm = I_SVAL(inst, 0, 8);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);
    ASSERT(regs[rs].tag == TAG_INT64 || regs[rs].tag == TAG_UINT64);

    regs[rd].ival = regs[rs].ival % imm;
    regs[rd].tag = regs[rs].tag;

    DISPATCH();
}

/* Float Basic */

TARGET(OP_FLOAT_ADD) {
    OP_NYI(OP_FLOAT_ADD);
}

TARGET(OP_FLOAT_SUB) {
    OP_NYI(OP_FLOAT_SUB);
}

TARGET(OP_FLOAT_MUL) {
    OP_NYI(OP_FLOAT_MUL);
}

TARGET(OP_FLOAT_DIV) {
    OP_NYI(OP_FLOAT_DIV);
}

/* cold instructions */

/* Unsigned Ops */

TARGET(OP_UINT_DIV) {
    OP_NYI(OP_UINT_DIV);
}

TARGET(OP_UINT_DIV_IMM) {
    OP_NYI(OP_UINT_DIV_IMM);
}

TARGET(OP_UINT_MOD) {
    OP_NYI(OP_UINT_MOD);
}

TARGET(OP_UINT_MOD_IMM) {
    OP_NYI(OP_UINT_MOD_IMM);
}

TARGET(OP_UINT_SHR) {
    OP_NYI(OP_UINT_SHR);
}

TARGET(OP_UINT_SHR_IMM) {
    OP_NYI(OP_UINT_SHR_IMM);
}

TARGET(OP_UINT_CMPLT) {
    OP_NYI(OP_UINT_CMPLT);
}

TARGET(OP_UINT_CMPLT_IMM) {
    OP_NYI(OP_UINT_CMPLT_IMM);
}

TARGET(OP_UINT_CMPLE) {
    OP_NYI(OP_UINT_CMPLE);
}

TARGET(OP_UINT_CMPLE_IMM) {
    OP_NYI(OP_UINT_CMPLE_IMM);
}

TARGET(OP_UINT_CMPGT) {
    OP_NYI(OP_UINT_CMPGT);
}

TARGET(OP_UINT_CMPGT_IMM) {
    OP_NYI(OP_UINT_CMPGT_IMM);
}

TARGET(OP_UINT_CMPGE) {
    OP_NYI(OP_UINT_CMPGE);
}

TARGET(OP_UINT_CMPGE_IMM) {
    OP_NYI(OP_UINT_CMPGE_IMM);
}

TARGET(OP_JMP_UINT_LT) {
    OP_NYI(OP_JMP_UINT_LT);
}

TARGET(OP_JMP_UINT_LT_IMM) {
    OP_NYI(OP_JMP_UINT_LT_IMM);
}

TARGET(OP_JMP_UINT_LE) {
    OP_NYI(OP_JMP_UINT_LE);
}

TARGET(OP_JMP_UINT_LE_IMM) {
    OP_NYI(OP_JMP_UINT_LE_IMM);
}

TARGET(OP_JMP_UINT_GT) {
    OP_NYI(OP_JMP_UINT_GT);
}

TARGET(OP_JMP_UINT_GT_IMM) {
    OP_NYI(OP_JMP_UINT_GT_IMM);
}

TARGET(OP_JMP_UINT_GE) {
    OP_NYI(OP_JMP_UINT_GE);
}

TARGET(OP_JMP_UINT_GE_IMM) {
    OP_NYI(OP_JMP_UINT_GE_IMM);
}

/* Type Conversion */

TARGET(OP_FLT_TO_INT) {
    OP_NYI(OP_FLT_TO_INT);
}

TARGET(OP_INT_TO_FLT) {
    OP_NYI(OP_INT_TO_FLT);
}

/* Float Complex */

TARGET(OP_FLOAT_MOD) {
    OP_NYI(OP_FLOAT_MOD);
}

TARGET(OP_FLOAT_CMPL) {
    OP_NYI(OP_FLOAT_CMPL);
}

TARGET(OP_FLOAT_CMPG) {
    OP_NYI(OP_FLOAT_CMPG);
}

TARGET(OP_FLOAT_NEG) {
    OP_NYI(OP_FLOAT_NEG);
}

/* miscellaneous instructions */

TARGET(OP_INT_NOT) {
    rd = I_VAL(inst, 12, 12);
    rs = I_VAL(inst, 0, 12);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = ~regs[rs].ival;
    regs[rd].tag = regs[rs].tag;

    DISPATCH();
}

TARGET(OP_INT_NEG) {
    rd = I_VAL(inst, 12, 12);
    rs = I_VAL(inst, 0, 12);

    CHECK_REG_ID(rd);
    CHECK_REG_ID(rs);

    regs[rd].ival = -regs[rs].ival;
    regs[rd].tag = regs[rs].tag;

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

TARGET(OP_NOP) {
    /* do nothing, just move to next instruction */
    DISPATCH();
}
