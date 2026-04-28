
static char *int_type_names[] = {
    "int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64",
};

typedef struct {
    int64_t min;
    int64_t max;
} IntRange;

static const IntRange int_ranges[] = {
    { INT8_MIN, INT8_MAX },   { INT16_MIN, INT16_MAX }, { INT32_MIN, INT32_MAX },
    { INT64_MIN, INT64_MAX }, { 0, UINT8_MAX },         { 0, UINT16_MAX },
    { 0, UINT32_MAX },        { 0, UINT64_MAX },
};

static int do_int_trap(int tag, int64_t *val)
{
    int64_t v = *val;
    tag = tag - TAG_INT8; // normalize to 0-based index for easier handling

    switch (tag) {
        case 0:
            if (v < INT8_MIN || v > INT8_MAX) return 0;
            *val = (int8_t)v;
            return 1;
        case 1:
            if (v < INT16_MIN || v > INT16_MAX) return 0;
            *val = (int16_t)v;
            return 1;
        case 2:
            if (v < INT32_MIN || v > INT32_MAX) return 0;
            *val = (int32_t)v;
            return 1;
        case 3:
            // int64: always ok, no change
            return 1;
        case 4:
            if (v < 0 || v > UINT8_MAX) return 0;
            *val = (uint8_t)v;
            return 1;
        case 5:
            if (v < 0 || v > UINT16_MAX) return 0;
            *val = (uint16_t)v;
            return 1;
        case 6:
            if (v < 0 || v > UINT32_MAX) return 0;
            *val = (uint32_t)v;
            return 1;
        case 7:
            // uint64: trap on negative, otherwise keep payload as-is
            return v >= 0;
        default:
            UNREACHABLE();
    }
}

static void do_int_wrap(int tag, int64_t *val)
{
    int64_t v = *val;
    tag = tag - TAG_INT8; // normalize to 0-based index for easier handling

    switch (tag) {
        case 0:
            *val = (int8_t)v;
            return;
        case 1:
            *val = (int16_t)v;
            return;
        case 2:
            *val = (int32_t)v;
            return;
        case 3:
            // int64 can represent all int64 values, no wrapping needed
            return;
        case 4:
            *val = (uint8_t)v;
            return;
        case 5:
            *val = (uint16_t)v;
            return;
        case 6:
            *val = (uint32_t)v;
            return;
        case 7:
            // int64/uint64 can represent all int64/uint64 values, no wrapping needed
            return;
        default:
            UNREACHABLE();
    }
}

static void do_int_cast(TValue *regs, int rd, int rs, int mode, int dst_ti)
{
    ASSERT(dst_ti >= TAG_INT8 && dst_ti <= TAG_UINT64);

    int src_tag = regs[rs].tag;
    int64_t val = regs[rs].ival;
    ASSERT(src_tag >= TAG_INT8 && src_tag <= TAG_UINT64);

    if (mode == 0) { // trap
        if (!do_int_trap(dst_ti, &val)) {
            int src_idx = src_tag - TAG_INT8;
            int dst_idx = dst_ti - TAG_INT8;
            fprintf(stderr, "panic: %s → %s cast overflow (%lld not in [%lld, %lld])\n",
                    int_type_names[src_idx], int_type_names[dst_idx], (long long)val,
                    (long long)int_ranges[dst_idx].min,
                    (long long)int_ranges[dst_idx].max);
            abort();
        }
    } else if (mode == 1) { // wrap
        do_int_wrap(dst_ti, &val);
    } else {
        NYI();
    }

    regs[rd].ival = val;
    regs[rd].tag = dst_ti;
}

static char *float_type_names[] = {
    "float16",
    "float32",
    "float64",
};

static int do_float_trap(int tag, double *val)
{
    double v = *val;

    tag = tag - TAG_FLOAT16; // normalize to 0-based index for easier handling

    switch (tag) {
        case 0: {
            _Float16 h = (_Float16)v;
            double rt = (double)h;
            if (rt != v) return 0;
            *val = rt;
            return 1;
        }
        case 1: {
            float f = (float)v;
            double rt = (double)f;
            if (rt != v) return 0;
            *val = rt;
            return 1;
        }
        case 2:
            return 1;
        default:
            UNREACHABLE();
    }
}

static void do_float_wrap(int tag, double *val)
{
    double v = *val;

    tag = tag - TAG_FLOAT16; // normalize to 0-based index for easier handling

    switch (tag) {
        case 0:
            _Float16 h = (_Float16)v;
            *val = (double)h;
            return;
        case 1:
            float f = (float)v;
            *val = (double)f;
            return;
        case 2:
            return;
        default:
            UNREACHABLE();
    }
}

static void do_float_cast(TValue *regs, int rd, int rs, int mode, int dst_ti)
{
    ASSERT(dst_ti >= TAG_FLOAT16 && dst_ti <= TAG_FLOAT64);

    int src_tag = regs[rs].tag;
    double val = regs[rs].fval;
    ASSERT(src_tag >= TAG_FLOAT16 && src_tag <= TAG_FLOAT64);

    if (mode == 0) { // trap
        if (!do_float_trap(dst_ti, &val)) {
            int src_idx = src_tag - TAG_FLOAT16;
            int dst_idx = dst_ti - TAG_FLOAT16;
            fprintf(stderr,
                    "panic: runtime error: %s to %s cast failed (value %.17g cannot be "
                    "represented exactly)\n",
                    float_type_names[src_idx], float_type_names[dst_idx], val);
            abort();
        }
    } else if (mode == 1) { // wrap
        do_float_wrap(dst_ti, &val);
    } else {
        NYI();
    }

    regs[rd].fval = val;
    regs[rd].tag = dst_ti;
}
