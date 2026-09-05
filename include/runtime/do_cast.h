
static char *int_type_names[] = {
    "int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64",
};

typedef struct {
    int64_t min;
    int64_t max;
} IntRange;

typedef struct {
    uint64_t min;
    uint64_t max;
} UIntRange;

static const IntRange int_ranges[] = {
    { INT8_MIN, INT8_MAX },
    { INT16_MIN, INT16_MAX },
    { INT32_MIN, INT32_MAX },
    { INT64_MIN, INT64_MAX },
};

static const UIntRange uint_ranges[] = {
    { 0, UINT8_MAX },
    { 0, UINT16_MAX },
    { 0, UINT32_MAX },
    { 0, UINT64_MAX },
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

            uint64_t low = 0;
            uint64_t high = 0;
            const char *fmt;

            if (dst_ti >= TAG_UINT8) {
                int uint_idx = dst_ti - TAG_UINT8;
                low = uint_ranges[uint_idx].min;
                high = uint_ranges[uint_idx].max;
                fmt = "panic: %s → %s cast overflow (%lld not in [%llu, %llu])\n";
            } else {
                low = (uint64_t)int_ranges[dst_idx].min;
                high = (uint64_t)int_ranges[dst_idx].max;
                fmt = "panic: %s → %s cast overflow (%lld not in [%lld, %lld])\n";
            }

            fprintf(stderr, fmt, int_type_names[src_idx], int_type_names[dst_idx], (long long)val,
                    low, high);

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
        case 0: {
            _Float16 h = (_Float16)v;
            *val = (double)h; // SMALL FIX: Write rounded back to pointer to enable wrap
            return;
        }
        case 1: {
            float f = (float)v;
            *val = (double)f; // SMALL FIX: Write rounded back to pointer to enable wrap
            return;
        }
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

// Map tag cleanly to 0-based index for int_type_names (0 to 7)
static int map_tag_to_int_names_idx(int tag)
{
    ASSERT(tag >= TAG_INT8 && tag <= TAG_UINT64);
    return tag - TAG_INT8;
}

#define FLOAT64_UPPER_BOUND_FOR_INT64  9223372036854775808.0
#define FLOAT64_LOWER_BOUND_FOR_INT64  -9223372036854775809.0
#define FLOAT64_UPPER_BOUND_FOR_UINT64 18446744073709551616.0
#define FLOAT32_UPPER_BOUND_FOR_UINT64 18446744073709551616.0f

static void do_int_to_float(TValue *regs, int rd, int rs, int mode, int dst_ti)
{
    ASSERT(dst_ti >= TAG_FLOAT16 && dst_ti <= TAG_FLOAT64);

    int src_tag = regs[rs].tag;
    ASSERT(src_tag >= TAG_INT8 && src_tag <= TAG_UINT64);

    // Extract exact 64-bit bits natively to avoid intermediate double precision loss
    uint64_t raw_bits = (uint64_t)regs[rs].ival;
    int is_unsigned = (src_tag >= TAG_UINT8);

    double final_fval = 0.0;
    int has_precision_loss = 0;

    // Simulate target hardware width precision natively
    if (dst_ti == TAG_FLOAT16) {
        _Float16 h;
        if (is_unsigned) {
            h = (_Float16)raw_bits;
            has_precision_loss = (h >= FLOAT32_UPPER_BOUND_FOR_UINT64 || (uint64_t)h != raw_bits);
        } else {
            int64_t sval = (int64_t)raw_bits;
            h = (_Float16)sval;
            has_precision_loss = (isinf(h) || (int64_t)h != sval);
        }
        final_fval = (double)h;
    } else if (dst_ti == TAG_FLOAT32) {
        float f;
        if (is_unsigned) {
            f = (float)raw_bits;
            has_precision_loss = (f >= FLOAT32_UPPER_BOUND_FOR_UINT64 || (uint64_t)f != raw_bits);
        } else {
            int64_t sval = (int64_t)raw_bits;
            f = (float)sval;
            has_precision_loss = (isinf(f) || (int64_t)f != sval);
        }
        final_fval = (double)f;
    } else { // TAG_FLOAT64
        double d;
        if (is_unsigned) {
            d = (double)raw_bits;
            has_precision_loss = (d >= FLOAT64_UPPER_BOUND_FOR_UINT64 || (uint64_t)d != raw_bits);
        } else {
            int64_t sval = (int64_t)raw_bits;
            d = (double)sval;
            has_precision_loss = (isinf(d) || (int64_t)d != sval);
        }
        final_fval = d;
    }

    // Handle strict mathematical contract under runtime trap mode
    if (mode == 0 && has_precision_loss) {
        int src_idx = map_tag_to_int_names_idx(src_tag);
        int dst_idx = dst_ti - TAG_FLOAT16;
        fprintf(stderr, "panic: runtime error: %s to %s cast precision loss under trap mode\n",
                int_type_names[src_idx], float_type_names[dst_idx]);
        abort();
    }

    regs[rd].fval = final_fval;
    regs[rd].tag = dst_ti;
}

static void do_float_to_int(TValue *regs, int rd, int rs, int mode, int dst_ti)
{
    ASSERT(dst_ti >= TAG_INT8 && dst_ti <= TAG_UINT64);

    int src_tag = regs[rs].tag;
    ASSERT(src_tag >= TAG_FLOAT16 && src_tag <= TAG_FLOAT64);

    double val = regs[rs].fval;
    int is_unsigned = (dst_ti >= TAG_UINT8);

    if (mode == 0) { // trap on overflow
        // NaN check
        int overflow = (val != val);

        if (!overflow) {
            if (is_unsigned) {
                int dst_idx = dst_ti - TAG_UINT8; // Correct uint_ranges mapping
                if (val < 0.0 || val >= FLOAT64_UPPER_BOUND_FOR_UINT64) {
                    overflow = 1;
                } else {
                    uint64_t uval = (uint64_t)val;
                    // Strict identity check: guard fractional truncation and narrow max limits
                    if ((double)uval != val || uval > uint_ranges[dst_idx].max) {
                        overflow = 1;
                    }
                }
            } else {
                int dst_idx = dst_ti - TAG_INT8; // Correct int_ranges mapping
                // Guard physical 2^63 bounds to prevent hardware conversion UB
                if (val >= FLOAT64_UPPER_BOUND_FOR_INT64 || val > int_ranges[dst_idx].max) {
                    overflow = 1;
                } else {
                    int64_t sval = (int64_t)val;
                    // Enforce strict fractional truncation validation for signed types
                    if ((double)sval != val || sval < int_ranges[dst_idx].min ||
                        sval > int_ranges[dst_idx].max) {
                        overflow = 1;
                    }
                }
            }
        }

        if (overflow) {
            int src_idx = src_tag - TAG_FLOAT16;
            int print_idx = map_tag_to_int_names_idx(dst_ti); // Use map to safely get name index
            fprintf(stderr,
                    "panic: runtime error: %s to %s cast overflow (value %.17g cannot be "
                    "represented as %s)\n",
                    float_type_names[src_idx], int_type_names[print_idx], val,
                    int_type_names[print_idx]);
            abort();
        }

        regs[rd].ival = is_unsigned ? (int64_t)(uint64_t)val : (int64_t)val;
        regs[rd].tag = dst_ti;
        return;
    }

    if (mode == 1) { // wrap / saturate
        int64_t ival;
        if (is_unsigned) {
            if (val != val || val < 0.0) {
                ival = 0;
            } else if (val >= FLOAT64_UPPER_BOUND_FOR_UINT64) {
                ival = (int64_t)UINT64_MAX;
            } else {
                ival = (int64_t)(uint64_t)val;
            }
        } else {
            if (val != val) {
                ival = 0;
            } else if (val <= FLOAT64_LOWER_BOUND_FOR_INT64) {
                ival = INT64_MIN;
            } else if (val >= FLOAT64_UPPER_BOUND_FOR_INT64) {
                ival = INT64_MAX;
            } else {
                ival = (int64_t)val;
            }
        }
        do_int_wrap(dst_ti, &ival);
        regs[rd].ival = ival;
        regs[rd].tag = dst_ti;
    }
}
