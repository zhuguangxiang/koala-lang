/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "cgen.h"
#include "codebuffer.h"
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif

static void dump_const_pool(KlMachModule *m)
{
    printf("Constant Pool:\n");
    KlMachConst *kc;
    vector_foreach(kc, &m->const_pool) {
        printf("  #%d: ", i__);
        switch (kc->tag) {
            case KL_MACH_CONST_I64:
                printf("int: %ld\n", kc->i64);
                break;
            case KL_MACH_CONST_U64:
                printf("uint: %lu\n", kc->u64);
                break;
            case KL_MACH_CONST_F64:
                printf("float: %f\n", kc->f64);
                break;
            case KL_MACH_CONST_STR:
                printf("string: \"%s\"\n", kc->str);
                break;
            default:
                UNREACHABLE();
        }
    }
}

static void dump_import_table(KlMachModule *m)
{
    printf("Import Table:\n");
    KlMachImport *imp;
    vector_foreach(imp, &m->import_table) {
        printf("  #%d: %s.%s\n", i__, imp->path, imp->name);
    }
}

static int __mach_const_eq__(void *a, void *b)
{
    KlMachConst *ka = (KlMachConst *)a;
    KlMachConst *kb = (KlMachConst *)b;
    if (ka->tag != kb->tag) return 0;
    switch (ka->tag) {
        case KL_MACH_CONST_I64:
            return ka->i64 == kb->i64;
        case KL_MACH_CONST_U64:
            return ka->u64 == kb->u64;
        case KL_MACH_CONST_F64:
            return ka->f64 == kb->f64;
        case KL_MACH_CONST_STR:
            return strcmp(ka->str, kb->str) == 0;
        default:
            UNREACHABLE();
    }
}

static unsigned int mach_const_hash(void *key)
{
    KlMachConst *kc = (KlMachConst *)key;
    switch (kc->tag) {
        case KL_MACH_CONST_I64:
            return mem_hash(&kc->i64, sizeof(kc->i64));
        case KL_MACH_CONST_U64:
            return mem_hash(&kc->u64, sizeof(kc->u64));
        case KL_MACH_CONST_F64: {
            union {
                double f;
                uint64_t u;
            } u = { .f = kc->f64 };
            return mem_hash(&u, sizeof(u));
        }
        case KL_MACH_CONST_STR:
            return str_hash(kc->str);
        default:
            UNREACHABLE();
    }
}

int kl_mach_const_add_int(KlMachModule *m, int64_t v)
{
    KlMachConst key = { .tag = KL_MACH_CONST_I64, .i64 = v };
    hashmap_entry_init(&key, mach_const_hash(&key));

    KlMachConst *entry = hashmap_get(&m->cp_map, &key);
    if (entry) {
        printf("Found existing const entry for int: %ld (index: %d)\n", v, entry->index);
        return entry->index;
    }

    KlMachConst *new_entry = mm_alloc_obj(new_entry);
    new_entry->tag = KL_MACH_CONST_I64;
    new_entry->i64 = v;
    hashmap_entry_init(new_entry, mach_const_hash(new_entry));
    hashmap_put(&m->cp_map, new_entry);
    vector_push_back(&m->const_pool, &new_entry);
    new_entry->index = vector_size(&m->const_pool) - 1;
    printf("Added new const entry for int: %ld (index: %d)\n", v, new_entry->index);
    return new_entry->index;
}

int kl_mach_const_add_uint(KlMachModule *m, uint64_t v)
{
    KlMachConst key = { .tag = KL_MACH_CONST_U64, .u64 = v };
    hashmap_entry_init(&key, mach_const_hash(&key));

    KlMachConst *entry = hashmap_get(&m->cp_map, &key);
    if (entry) {
        printf("Found existing const entry for uint: %lu (index: %d)\n", v, entry->index);
        return entry->index;
    }

    KlMachConst *new_entry = mm_alloc_obj(new_entry);
    new_entry->tag = KL_MACH_CONST_U64;
    new_entry->u64 = v;
    hashmap_entry_init(new_entry, mach_const_hash(new_entry));
    hashmap_put(&m->cp_map, new_entry);
    vector_push_back(&m->const_pool, &new_entry);
    new_entry->index = vector_size(&m->const_pool) - 1;
    printf("Added new const entry for uint: %lu (index: %d)\n", v, new_entry->index);
    return new_entry->index;
}

int kl_mach_const_add_float(KlMachModule *m, double v)
{
    KlMachConst key = { .tag = KL_MACH_CONST_F64, .f64 = v };
    hashmap_entry_init(&key, mach_const_hash(&key));

    KlMachConst *entry = hashmap_get(&m->cp_map, &key);
    if (entry) {
        printf("Found existing const entry for float: %f (index: %d)\n", v, entry->index);
        return entry->index;
    }

    KlMachConst *new_entry = mm_alloc_obj(new_entry);
    new_entry->tag = KL_MACH_CONST_F64;
    new_entry->f64 = v;
    hashmap_entry_init(new_entry, mach_const_hash(new_entry));
    hashmap_put(&m->cp_map, new_entry);
    vector_push_back(&m->const_pool, &new_entry);
    int index = vector_size(&m->const_pool) - 1;
    new_entry->index = index;
    printf("Added new const entry for float: %f (index: %d)\n", v, new_entry->index);
    return index;
}

int kl_mach_const_add_str(KlMachModule *m, char *v)
{
    KlMachConst key = { .tag = KL_MACH_CONST_STR, .str = v };
    hashmap_entry_init(&key, mach_const_hash(&key));

    KlMachConst *entry = hashmap_get(&m->cp_map, &key);
    if (entry) {
        printf("Found existing const entry for string: %s (index: %d)\n", v,
               entry->index);
        return entry->index;
    }

    KlMachConst *new_entry = mm_alloc_obj(new_entry);
    new_entry->tag = KL_MACH_CONST_STR;
    new_entry->str = v;
    hashmap_entry_init(new_entry, mach_const_hash(new_entry));
    hashmap_put(&m->cp_map, new_entry);
    vector_push_back(&m->const_pool, &new_entry);
    int index = vector_size(&m->const_pool) - 1;
    new_entry->index = index;
    printf("Added new const entry for string: %s (index: %d)\n", v, index);
    return index;
}

static int __mach_import_eq__(void *a, void *b)
{
    KlMachImport *ia = (KlMachImport *)a;
    KlMachImport *ib = (KlMachImport *)b;
    return strcmp(ia->path, ib->path) == 0 && strcmp(ia->name, ib->name) == 0;
}

static unsigned int mach_import_hash(void *key)
{
    KlMachImport *imp = (KlMachImport *)key;
    unsigned int h1 = str_hash(imp->path);
    unsigned int h2 = str_hash(imp->name);
    return h1 ^ h2;
}

static int mach_import_add(KlMachModule *m, char *path, char *name)
{
    KlMachImport key = { .path = path, .name = name };
    hashmap_entry_init(&key, mach_import_hash(&key));

    KlMachImport *entry = hashmap_get(&m->import_map, &key);
    if (entry) {
        printf("Found existing import entry for %s.%s (index: %d)\n", path, name,
               entry->index);
        return entry->index;
    }

    KlMachImport *new_entry = mm_alloc_obj(new_entry);
    new_entry->path = path;
    new_entry->name = name;
    hashmap_entry_init(new_entry, mach_import_hash(new_entry));
    hashmap_put(&m->import_map, new_entry);
    vector_push_back(&m->import_table, &new_entry);
    int import_index = vector_size(&m->import_table) - 1;
    new_entry->index = import_index;
    printf("Added new import entry for %s.%s (index: %d)\n", path, name, import_index);
    return import_index;
}

static void dump_func_byte_code(KlMachFunc *mfn, const uint8_t *code)
{
    for (size_t pc = mfn->start_pc; pc < mfn->start_pc + mfn->total_insns; pc++) {
        uint32_t insn = *(uint32_t *)(code + pc * 4);
        OpCode opcode = (insn >> 24) & 0xFFu;
        OpFormat fmt = op_format(opcode);
        char *s = op_name(opcode);
        ASSERT(s);
        printf("%04zu:  %08X   %s ", pc, insn, s);
        switch (fmt) {
            case FORMAT_Rx: {
                int Rx = insn & 0xFFFu;
                printf("r%d", Rx);
                break;
            }

            case FORMAT_RxImm: {
                int Rx = (insn >> 8) & 0xFFFu;
                int imm = insn & 0xFFu;
                printf("r%d, #%d", Rx, imm);
                break;
            }

            case FORMAT_ROff2: {
                int R = (insn >> 16) & 0xFFu;
                int data = (int16_t)(insn & 0xFFFFu);
                printf("r%d, %d", R, data);
                break;
            }

            case FORMAT_RImm2:
            case FORMAT_RIdx2: {
                int R = (insn >> 16) & 0xFFu;
                int data = (int16_t)(insn & 0xFFFFu);
                printf("r%d, #%d", R, data);
                break;
            }

            case FORMAT_RImmOff: {
                int R = (insn >> 16) & 0xFFu;
                int imm = (int8_t)((insn >> 8) & 0xFFu);
                int off = (int8_t)(insn & 0xFFu);
                printf("r%d, #%d, %d", R, imm, off);
                break;
            }

            case FORMAT_RxRx: {
                int Rx1 = (insn >> 12) & 0xFFFu;
                int Rx2 = insn & 0xFFFu;
                printf("r%d, r%d", Rx1, Rx2);
                break;
            }

            case FORMAT_RRImm: {
                int R1 = (insn >> 16) & 0xFFu;
                int R2 = (insn >> 8) & 0xFFu;
                int data = (int8_t)(insn & 0xFFu);
                printf("r%d, r%d, #%d", R1, R2, data);
                break;
            }

            case FORMAT_RROff: {
                int R1 = (insn >> 16) & 0xFFu;
                int R2 = (insn >> 8) & 0xFFu;
                int data = (int8_t)(insn & 0xFFu);
                printf("r%d, r%d, %d", R1, R2, data);
                break;
            }

            case FORMAT_RRR: {
                int R1 = (insn >> 16) & 0xFFu;
                int R2 = (insn >> 8) & 0xFFu;
                int R3 = insn & 0xFFu;
                printf("r%d, r%d, r%d", R1, R2, R3);
                break;
            }

            case FORMAT_JMP: {
                int data = (int16_t)(insn & 0xFFFFu);
                printf("%d", data);
                break;
            }

            case FORMAT_Imm2:
            case FORMAT_Idx2: {
                int data = (int16_t)(insn & 0xFFFFu);
                printf("#%d", data);
                break;
            }

            case FORMAT_Op: {
                // no operands
                break;
            }

            case FORMAT_CALL: {
                int flag = (insn >> 20) & 0xFu;
                int Rx = (insn >> 8) & 0xFFFu;
                int nargs = insn & 0xFFu;
                printf("flg=%d, ", flag);
                if (Rx != 0xFFFu) printf("r%d, ", Rx);
                printf("#%d\n", nargs);
                pc++; // skip the next FORMAT_DATA entry
                insn = *(uint32_t *)(code + pc * 4);
                printf("%04zu:  %08X   data ", pc, insn);
                if (flag == 0) {
                    printf("(rel32=%d)", (int)insn);
                } else if (flag == 1) {
                    printf("(import_index=%d)", (int)insn);
                } else if (flag == 2) {
                    NYI();
                }
                break;
            }

            default: {
                printf("(unknown format)");
                break;
            }
        }
        printf("\n");
    }
}

static void dump_byte_code(KlMachModule *m)
{
    const CodeBuffer *cb = &m->codes;
    const uint8_t *ptr = cb->data;
    size_t len = cb->size;

    printf("\n====== Emitted Bytecode (insns: %zu) ======\n\n", len / 4);

    KlMachFunc *mfn;
    vector_foreach(mfn, &m->funcs) {
        printf("\n@%s:\n", mfn->origin->name);
        printf("[start_pc: %d, insns: %d]\n", mfn->start_pc, mfn->total_insns);
    }

    printf("\n");

    vector_foreach(mfn, &m->funcs) {
        printf("@%s[%d,%d]:\n", mfn->origin->name, mfn->start_pc,
               mfn->start_pc + mfn->total_insns - 1);
        dump_func_byte_code(mfn, ptr);
    }

    printf("\n");

    printf("=== End of Emitted Bytecode ====\n\n");
}

static void dump_mach_insn(KlMachInsn *mi)
{
    KlMachFunc *fn = mi->bb->fn;

    char *s = op_name(mi->op);
    int used = printf("%4d:  %s ", mi->pc, s);

    switch (mi->format) {
        case FORMAT_Rx: {
            used += printf("r%d", mi->opers[0]);
            break;
        }

        case FORMAT_ROff2: {
            used += printf("r%d, %d", mi->opers[0], mi->opers[1]);
            break;
        }

        case FORMAT_RxImm:
        case FORMAT_RImm2:
        case FORMAT_RIdx2: {
            used += printf("r%d, #%d", mi->opers[0], mi->opers[1]);
            break;
        }

        case FORMAT_RImmOff: {
            used += printf("r%d, #%d, %d", mi->opers[0], mi->opers[1], mi->opers[2]);
            break;
        }

        case FORMAT_RxRx: {
            used += printf("r%d, r%d", mi->opers[0], mi->opers[1]);
            break;
        }

        case FORMAT_RRImm: {
            used += printf("r%d, r%d, #%d", mi->opers[0], mi->opers[1], mi->opers[2]);
            break;
        }

        case FORMAT_RROff: {
            used += printf("r%d, r%d, %d", mi->opers[0], mi->opers[1], mi->opers[2]);
            break;
        }

        case FORMAT_RRR: {
            used += printf("r%d, r%d, r%d", mi->opers[0], mi->opers[1], mi->opers[2]);
            break;
        }

        case FORMAT_JMP: {
            used += printf("%d", mi->opers[0]);
            break;
        }

        case FORMAT_Imm2:
        case FORMAT_Idx2: {
            used += printf("#%d", mi->opers[0]);
            break;
        }

        case FORMAT_CALL: {
            printf("flg=%d, ", mi->opers[0]);
            int ret_reg = mi->opers[1];
            if (ret_reg != -1) used += printf("r%d, ", ret_reg);
            used += printf("#%d", mi->opers[2]);
            break;
        }

        case FORMAT_Op:
            // no operand
            break;

        case FORMAT_DATA:
            // data payload, no fixed fields
            break;

        default:
            used += printf("(unknown format)");
            break;
    }

    if (mi->target) {
        KlrBasicBlock *origin = mi->target->origin;
        printf("%*s;; -> %%%s", 36 - used, "", klr_block_name(origin));
    }

    printf("\n");
}

static void dump_mach_block(KlMachBlock *mb)
{
    printf("%%%s:\n", klr_block_name(mb->origin));

    KlMachInsn *mi;
    vector_foreach(mi, &mb->insns) {
        dump_mach_insn(mi);
    }

    printf("\n");
}

static void dump_mach_func(KlMachFunc *fn)
{
    printf("\n====== Linearization @%s(start_pc: %d, insns: %d) ======\n\n",
           fn->origin->name, fn->start_pc, fn->total_insns);

    KlMachBlock *mb;
    list_foreach(mb, link, &fn->bb_list) {
        dump_mach_block(mb);
    }

    printf("=== End of Linearization @%s ====\n\n", fn->origin->name);
}

/* Extract integer value from a raw operand. */
static inline int get_mach_oper(KlrInsn *insn, KlrRawOper *op)
{
    switch (op->kind) {
        case RAW_OPER_REG: {
            /* Use the instruction's result vreg. */
            return op->vreg;
        }

        case RAW_OPER_IMM: {
            return op->imm;
        }

        case RAW_OPER_CONST: {
            return op->index;
        }

        default: {
            UNREACHABLE();
        }
    }
}

static inline void *get_mach_oper_ptr(KlrInsn *insn, KlrRawOper *op)
{
    switch (op->kind) {
        case RAW_OPER_BLOCK: {
            return op->ptr;
        }
        case RAW_OPER_FUNC: {
            return op->ptr;
        }
        default: {
            UNREACHABLE();
        }
    }
}

static KlMachInsn *build_mach_insn(OpCode op, KlrInsn *insn, KlMachBlock *mb)
{
    KlMachInsn *mi = mm_alloc_obj(mi);
    mi->op = op;
    mi->format = op_format(op);
    mi->pc = -1;
    mi->origin = insn;
    mi->bb = mb;
    return mi;
}

static KlMachInsn *build_data_mach_insn(KlMachBlock *mb)
{
    KlMachInsn *mi = mm_alloc_obj(mi);
    mi->op = OP_DATA;
    mi->format = op_format(mi->op);
    mi->pc = -1;
    mi->bb = mb;
    return mi;
}

static void emit_mach_insn(KlMachInsn *mi, CodeBuffer *buf)
{
    uint32_t bytecode = 0;
    OpCode op = mi->op;
    KlrInsn *insn = mi->origin;

    switch (mi->format) {
        case FORMAT_Op: {
            // | op:8 | ----:24 |
            bytecode |= (op & 0xFFu) << 24;
            break;
        }

        case FORMAT_Rx: {
            // | op:8 | ----:12 | Rx:12 |
            uint32_t Rx = mi->opers[0];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (Rx & 0xFFFu);
            break;
        }

        case FORMAT_RxImm: {
            // | op:8 | ----:4 | Rx:12 | imm:8 |
            uint32_t Rx = mi->opers[0];
            int imm8 = mi->opers[1];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (Rx & 0xFFFu) << 8;
            bytecode |= (imm8 & 0xFFu);
            break;
        }

        case FORMAT_RImm2:
        case FORMAT_ROff2:
        case FORMAT_RIdx2: {
            // | op:8 | R:8 | imm2/off2/idx2:16 |
            uint32_t R = mi->opers[0];
            int data16 = mi->opers[1];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (R & 0xFFu) << 16;
            bytecode |= (data16 & 0xFFFFu);
            break;
        }

        case FORMAT_RImmOff: {
            // | op:8 | R:8 | imm:8 | off:8 |
            uint32_t R = mi->opers[0];
            int imm8 = mi->opers[1];
            int off8 = mi->opers[2];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (R & 0xFFu) << 16;
            bytecode |= (imm8 & 0xFFu) << 8;
            bytecode |= (off8 & 0xFFu);
            break;
        }

        case FORMAT_RxRx: {
            // | op:8 | Rx1:12 | Rx2:12 |
            uint32_t Rx1 = mi->opers[0];
            uint32_t Rx2 = mi->opers[1];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (Rx1 & 0xFFFu) << 12;
            bytecode |= (Rx2 & 0xFFFu);
            break;
        }

        case FORMAT_RRR: {
            // | op:8 | R1:8 | R2:8 | R3:8 |
            uint32_t R1 = mi->opers[0];
            uint32_t R2 = mi->opers[1];
            uint32_t R3 = mi->opers[2];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (R1 & 0xFFu) << 16;
            bytecode |= (R2 & 0xFFu) << 8;
            bytecode |= (R3 & 0xFFu);
            break;
        }

        case FORMAT_RRImm:
        case FORMAT_RROff: {
            // | op:8 | R1:8 | R2:8 | imm/off:8 |
            uint32_t R1 = mi->opers[0];
            uint32_t R2 = mi->opers[1];
            int data8 = mi->opers[2];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (R1 & 0xFFu) << 16;
            bytecode |= (R2 & 0xFFu) << 8;
            bytecode |= (data8 & 0xFFu);
            break;
        }

        case FORMAT_Imm2:
        case FORMAT_JMP:
        case FORMAT_Idx2: {
            // | op:8 | ----:8 | imm2/jmp/idx2:16 |
            int data16 = mi->opers[0];
            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (data16 & 0xFFFFu);
            break;
        }

        case FORMAT_DATA: {
            bytecode = 0;
            break;
        }

        case FORMAT_CALL: {
            // | op:8 | flag:4 | Rx:12 | nargs:8 |
            uint32_t flag = mi->opers[0];
            uint32_t Rx = mi->opers[1];
            uint32_t nargs = mi->opers[2];

            bytecode |= (op & 0xFFu) << 24;
            bytecode |= (flag & 0xFu) << 20;
            bytecode |= (Rx & 0xFFFu) << 8;
            bytecode |= (nargs & 0xFFu);
            break;
        }

        default: {
            UNREACHABLE();
            break;
        }
    }

    // emit 4-byte instruction
    codebuf_put_u32(buf, bytecode);
}

/* Populate machine instruction fields from raw operands.
 * isel must fully define raw_opers[]; this function only
 * maps them into physical encoding fields based on format.
 */
static void fill_mach_insn(KlMachInsn *mi, KlrInsn *insn, KlMachModule *m)
{
    KlrRawOper *op0 = &insn->raws[0];
    KlrRawOper *op1 = &insn->raws[1];
    KlrRawOper *op2 = &insn->raws[2];

    /* Map raw operands into physical encoding fields. */
    switch (mi->format) {
        case FORMAT_Rx:
        case FORMAT_Imm2:
        case FORMAT_Idx2: {
            mi->opers[0] = get_mach_oper(insn, op0);
            break;
        }

        case FORMAT_RxImm:
        case FORMAT_RImm2:
        case FORMAT_ROff2:
        case FORMAT_RIdx2:
        case FORMAT_RxRx: {
            mi->opers[0] = get_mach_oper(insn, op0);
            mi->opers[1] = get_mach_oper(insn, op1);
            break;
        }

        case FORMAT_RImmOff:
        case FORMAT_RRImm:
        case FORMAT_RROff:
        case FORMAT_RRR: {
            mi->opers[0] = get_mach_oper(insn, op0);
            mi->opers[1] = get_mach_oper(insn, op1);
            mi->opers[2] = get_mach_oper(insn, op2);
            break;
        }

        case FORMAT_JMP: {
            void *ptr = get_mach_oper_ptr(insn, op0);
            ASSERT(klr_is_block(ptr));
            KlrBasicBlock *target_bb = (KlrBasicBlock *)ptr;
            mi->target = target_bb->mach;
            break;
        }

        case FORMAT_CALL: {
            mi->opers[1] = get_mach_oper(insn, op0);
            mi->opers[2] = get_mach_oper(insn, op1);
            void *ptr = get_mach_oper_ptr(insn, op2);
            ASSERT(klr_is_func(ptr) || klr_is_extfunc(ptr));
            KlrFunc *fn = (KlrFunc *)ptr;

            if (klr_is_extfunc(ptr)) {
                log_info("  call target: (external)");
                // create an import entry for this external function, and record the
                // import index in the call instruction's import_index field.
                int index = mach_import_add(m, "std/builtin", fn->name);
                mi->import_index = index;
                // insert 4 bytes: import_index
                KlMachBlock *mb = mi->bb;
                KlMachInsn *data = build_data_mach_insn(mb);
                vector_push_back(&mb->insns, &data);
                vector_push_back(&m->fixups, &mi);
                mi->fixup_flag = KL_MACH_FIXUP_IMPORT;
                mi->opers[0] = 1; // set flag for external function
            } else {
                mi->target_fn = fn->mach;
                log_info("  call target: %s(local)", fn->name);
                // insert 4 bytes: rel32
                KlMachBlock *mb = mi->bb;
                KlMachInsn *data = build_data_mach_insn(mb);
                vector_push_back(&mb->insns, &data);
                vector_push_back(&m->fixups, &mi);
                mi->fixup_flag = KL_MACH_FIXUP_REL32;
                mi->opers[0] = 0; // set flag for local function
            }
            break;
        }

        case FORMAT_Op: {
            // no operand, nothing to fill
            break;
        }

        default: {
            UNREACHABLE();
            break;
        }
    }
}

static KlMachFunc *linearize(KlrFunc *fn, KlMachModule *m)
{
    KlMachFunc *mfn = mm_alloc_obj(mfn);
    mfn->origin = fn;
    mfn->m = m;
    init_list(&mfn->bb_list);
    vector_init_ptr(&mfn->branches);
    vector_push_back(&m->funcs, &mfn);
    fn->mach = mfn;

    // build machine blocks
    int index = 0;
    KlrBasicBlock *bb;
    basic_block_foreach(bb, fn) {
        // build MachBlock for each basic block
        KlMachBlock *mb = mm_alloc_obj(mb);
        mb->origin = bb;
        mb->fn = mfn;
        init_list(&mb->link);
        vector_init_ptr(&mb->insns);
        list_push_back(&mfn->bb_list, &mb->link);
        bb->mach = mb;
        if (index == 0) mfn->entry = mb;
        ++index;
    }

    // get entry block and add DATA(fid) at the beginning for func.
    KlMachBlock *entry_mb = mfn->entry;
    KlMachInsn *data = build_data_mach_insn(entry_mb);
    data->fixup_flag = KL_MACH_FIXUP_FUNCID;
    data->target_fn = mfn;
    vector_push_back(&entry_mb->insns, &data);
    vector_push_back(&m->fixups, &data);

    // build machine insns
    KlMachBlock *mb;
    list_foreach(mb, link, &mfn->bb_list) {
        KlMachInsn *mi;
        KlrBasicBlock *bb = mb->origin;
        KlrInsn *insn;
        insn_foreach(insn, bb) {
            if (klr_is_local((KlrValue *)insn)) {
                // pseudo instruction, skip it.
                continue;
            }

            if (insn_is(insn, OP_IR_JMP_COND)) {
                // don't do sel for jmp_cond here, linearize() only does build linear
                // MachInsn, without any transformation. Do it in lower_branches()
                mi = build_mach_insn(insn->code, insn, mb);
                vector_push_back(&mb->insns, &mi);
                vector_push_back(&mfn->branches, &mi);
                continue;
            }

            // general case
            mi = build_mach_insn(insn->code, insn, mb);
            vector_push_back(&mb->insns, &mi);
            fill_mach_insn(mi, insn, m);
        }
    }

    return mfn;
}

/**
 * Lower OP_IR_JMP_COND into concrete machine-level jumps.
 * This pseudo instruction cannot be assigned a PC or encoded
 * directly. We must eliminate it here based solely on block layout
 * (fallthrough).
 *
 *   br cond, bb_true, bb_false
 *
 * If bb_true is the fallthrough block, we invert the condition:
 *
 *   if (!cond) goto bb_false
 *
 * Otherwise:
 *
 *   if (cond) goto bb_true
 *   goto bb_false
 *
 * After this lowering, the instruction stream contains only real,
 * PC-carrying jump instructions, allowing patch_pc() and
 * patch_branches() to compute correct offsets.
 */
static void lower_jmp_cond(KlMachInsn *mi)
{
    KlMachBlock *mb = mi->bb;
    KlrInsn *insn = mi->origin;

    KlrValue *cond = insn_oper_value(insn, 0);

    KlrValue *val = insn_oper_value(insn, 1);
    ASSERT(klr_is_block(val));
    KlrBasicBlock *bb_true = (KlrBasicBlock *)val;

    val = insn_oper_value(insn, 2);
    ASSERT(klr_is_block(val));
    KlrBasicBlock *bb_false = (KlrBasicBlock *)val;

    // next block is fallthrough (may be NULL)
    KlMachBlock *next = NEXT_BLOCK(mb);
    int fallthrough = (next && next->origin == bb_true);

    if (fallthrough) {
        // if (!cond) goto false
        // inplace update
        mi->op = OP_JMP_FALSE;
        mi->format = FORMAT_ROff2;
        mi->origin = insn;
        mi->opers[0] = cond->vreg;
        mi->target = bb_false->mach;
    } else {
        // if (cond) goto true
        // inplace update
        mi->op = OP_JMP_TRUE;
        mi->format = FORMAT_ROff2;
        mi->origin = insn;
        mi->opers[0] = cond->vreg;
        mi->target = bb_true->mach;

        // goto false
        KlMachInsn *mi_false = build_mach_insn(OP_JMP, insn, mb);
        mi_false->target = bb_false->mach;
        vector_push_back(&mb->insns, &mi_false);
    }
}

static void lower_branches(KlMachFunc *mfn)
{
    KlMachInsn *mi;
    vector_foreach(mi, &mfn->branches) {
        ASSERT(mach_insn_is(mi, OP_IR_JMP_COND));
        /* Lower jmp_cond into concrete machine-level jumps. */
        lower_jmp_cond(mi);
    }
}

static void assign_pc_and_patch_branches(KlMachFunc *mfn)
{
    KlMachBlock *mb;
    KlMachModule *m = mfn->m;

    // assign pc
    mfn->start_pc = m->pc;
    int pc = mfn->start_pc;

    list_foreach(mb, link, &mfn->bb_list) {
        // Record the starting PC of this block.
        mb->start_pc = pc;

        // Assign PC to each instruction in this block.
        KlMachInsn *mi;
        vector_foreach(mi, &mb->insns) {
            mi->pc = pc;
            pc++;
        }

        mb->end_pc = pc;
    }

    mfn->total_insns = pc - mfn->start_pc;
    m->pc += mfn->total_insns;

    // patch branch/jmp
    list_foreach(mb, link, &mfn->bb_list) {
        KlMachInsn *mi;
        vector_foreach(mi, &mb->insns) {
            if (mach_insn_is(mi, OP_JMP)) {
                ASSERT(mi->format == FORMAT_JMP);
                ASSERT(mi->target);
                int target_pc = mi->target->start_pc;
                ASSERT(target_pc >= 0);
                /* Relative offset: target - (current + 1) */
                mi->opers[0] = target_pc - (mi->pc + 1);
            } else if (mach_insn_or(mi, OP_JMP_TRUE, OP_JMP_FALSE)) {
                ASSERT(mi->format == FORMAT_ROff2);
                ASSERT(mi->target);
                int target_pc = mi->target->start_pc;
                ASSERT(target_pc >= 0);
                /* Relative offset: target - (current + 1) */
                mi->opers[1] = target_pc - (mi->pc + 1);
            } else {
                // do nothing for non-jump instructions
            }
        }
    }
}

static void emit_mach_func(KlMachFunc *mfn)
{
    KlMachModule *m = mfn->m;

    KlMachBlock *mb;
    list_foreach(mb, link, &mfn->bb_list) {
        KlMachInsn *mi;
        vector_foreach(mi, &mb->insns) {
            emit_mach_insn(mi, &m->codes);
        }
    }
}

static void patch_fixups(KlMachModule *m)
{
    CodeBuffer *codes = &m->codes;
    KlMachInsn *mi;
    vector_foreach(mi, &m->fixups) {
        if (mach_insn_is(mi, OP_CALL)) {
            if (mi->fixup_flag == KL_MACH_FIXUP_REL32) {
                ASSERT(mi->format == FORMAT_CALL);
                ASSERT(mi->target_fn);
                int target_pc = mi->target_fn->start_pc;
                // the following DATA insn
                int payload_pc = mi->pc + 1;
                printf("fixup call '%s' at pc %d(relative), to 'target %s' at pc %d\n",
                       mi->origin->bb->func->name, payload_pc,
                       mi->target_fn->origin->name, target_pc);
                ASSERT(target_pc >= 0);
                /* Relative offset: target - (payload + 1) */
                int rel32 = target_pc - (payload_pc + 1);
                /* Patch the placeholder data instruction following the call. */
                int *patch = (int *)codes->data + payload_pc;
                *patch = rel32;
                printf("  patched position at pc %d with relative offset %d\n",
                       payload_pc, rel32);
            } else {
                ASSERT(mi->fixup_flag == KL_MACH_FIXUP_IMPORT);
                ASSERT(mi->format == FORMAT_CALL);
                ASSERT(mi->import_index >= 0);
                // the following DATA insn
                int payload_pc = mi->pc + 1;
                int *patch = (int *)codes->data + payload_pc;
                *patch = mi->import_index;
                printf("fixup call '%s' at pc %d(import), with import index %d\n",
                       mi->origin->bb->func->name, payload_pc, mi->import_index);
            }
        } else if (mach_insn_is(mi, OP_DATA)) {
            ASSERT(mi->fixup_flag == KL_MACH_FIXUP_FUNCID);
            ASSERT(mi->format == FORMAT_DATA);
            // the following DATA insn is for function ID, which is the index of this
            // function in the module's function list.
            int payload_pc = mi->pc;
            KlMachFunc *target_fn = mi->target_fn;
            int func_index = target_fn->index;
            int *patch = (int *)codes->data + payload_pc;
            *patch = func_index;
            printf("fixup function '%s' at pc %d(funcid), with function index %d\n",
                   target_fn->origin->name, payload_pc, func_index);
        } else {
            UNREACHABLE();
        }
    }
}

static void init_mach_context(KlMachModule *m, KlrModule *origin)
{
    m->origin = origin;
    vector_init_ptr(&m->funcs);
    vector_init_ptr(&m->fixups);
    vector_init_ptr(&m->import_table);
    vector_init_ptr(&m->const_pool);
    codebuf_init(&m->codes);
    hashmap_init(&m->cp_map, __mach_const_eq__);
    hashmap_init(&m->import_map, __mach_import_eq__);
    m->pc = 0;
}

void kl_do_codegen(KlrModule *origin)
{
    KlMachModule m;
    init_mach_context(&m, origin);

    // Linearize each function and assign PCs.
    KlMachFunc *mfn;
    KlrFunc *fn;
    vector_foreach(fn, &origin->functions) {
        kl_lower_operands(fn, &m);
        mfn = linearize(fn, &m);
        mfn->index = i__;
        lower_branches(mfn);
        assign_pc_and_patch_branches(mfn);
        emit_mach_func(mfn);
        dump_mach_func(mfn);
    }

    patch_fixups(&m);

    dump_byte_code(&m);

    dump_const_pool(&m);
    dump_import_table(&m);

    vector_fini(&m.funcs);
    vector_fini(&m.fixups);
}

#ifdef __cplusplus
}
#endif
