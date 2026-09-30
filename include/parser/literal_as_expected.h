
static int expect_lit_as_int8(Literal *lit, Loc loc, ParserState *ps)
{
    if (lit->sign) {
        switch (lit->len) {
            case 1:
                return 1;

            case 2: {
                int16_t val = (int16_t)lit->ival;
                if (val >= INT8_MIN && val <= INT8_MAX) return 1;
                kl_error(loc, "Literal value out of range for int8");
                return 0;
            }

            case 4: {
                int32_t val = (int32_t)lit->ival;
                if (val >= INT8_MIN && val <= INT8_MAX) return 1;
                kl_error(loc, "Literal value out of range for int8");
                return 0;
            }

            case 8: {
                int64_t val = (int64_t)lit->ival;
                if (val >= INT8_MIN && val <= INT8_MAX) return 1;
                kl_error(loc, "Literal value out of range for int8");
                return 0;
            }

            default:
                return 0;
        }
    }

    switch (lit->len) {
        case 1: {
            uint8_t val = (uint8_t)lit->ival;
            if (val <= INT8_MAX) return 1;
            kl_error(loc, "Literal value out of range for int8");
            return 0;
        }

        case 2: {
            uint16_t val = (uint16_t)lit->ival;
            if (val <= INT8_MAX) return 1;
            kl_error(loc, "Literal value out of range for int8");
            return 0;
        }

        case 4: {
            uint32_t val = (uint32_t)lit->ival;
            if (val <= INT8_MAX) return 1;
            kl_error(loc, "Literal value out of range for int8");
            return 0;
        }

        case 8: {
            uint64_t val = (uint64_t)lit->ival;
            if (val <= INT8_MAX) return 1;
            kl_error(loc, "Literal value out of range for int8");
            return 0;
        }

        default:
            return 0;
    }
}

static int expect_lit_as_int16(Literal *lit, Loc loc, ParserState *ps)
{
    if (lit->sign) {
        switch (lit->len) {
            case 1:
            case 2:
                return 1;

            case 4: {
                int32_t val = (int32_t)lit->ival;
                if (val >= INT16_MIN && val <= INT16_MAX) return 1;
                kl_error(loc, "Literal value out of range for int16");
                return 0;
            }

            case 8: {
                int64_t val = (int64_t)lit->ival;
                if (val >= INT16_MIN && val <= INT16_MAX) return 1;
                kl_error(loc, "Literal value out of range for int16");
                return 0;
            }

            default:
                return 0;
        }
    }

    switch (lit->len) {
        case 1:
            return 1;

        case 2: {
            uint16_t val = (uint16_t)lit->ival;
            if (val <= INT16_MAX) return 1;
            kl_error(loc, "Literal value out of range for int16");
            return 0;
        }

        case 4: {
            uint32_t val = (uint32_t)lit->ival;
            if (val <= INT16_MAX) return 1;
            kl_error(loc, "Literal value out of range for int16");
            return 0;
        }

        case 8: {
            uint64_t val = (uint64_t)lit->ival;
            if (val <= INT16_MAX) return 1;
            kl_error(loc, "Literal value out of range for int16");
            return 0;
        }

        default:
            return 0;
    }
}

static int expect_lit_as_int32(Literal *lit, Loc loc, ParserState *ps)
{
    if (lit->sign) {
        switch (lit->len) {
            case 1:
            case 2:
            case 4:
                return 1;

            case 8: {
                int64_t val = (int64_t)lit->ival;
                if (val >= INT32_MIN && val <= INT32_MAX) return 1;
                kl_error(loc, "Literal value out of range for int32");
                return 0;
            }

            default:
                return 0;
        }
    }

    switch (lit->len) {
        case 1:
        case 2:
            return 1;

        case 4: {
            uint32_t val = (uint32_t)lit->ival;
            if (val <= INT32_MAX) return 1;
            kl_error(loc, "Literal value out of range for int32");
            return 0;
        }

        case 8: {
            uint64_t val = (uint64_t)lit->ival;
            if (val <= INT32_MAX) return 1;
            kl_error(loc, "Literal value out of range for int32");
            return 0;
        }

        default:
            return 0;
    }
}

static int expect_lit_as_int64(Literal *lit, Loc loc, ParserState *ps)
{
    if (lit->sign) {
        switch (lit->len) {
            case 1:
            case 2:
            case 4:
            case 8:
                return 1;

            default:
                return 0;
        }
    }

    switch (lit->len) {
        case 1:
        case 2:
        case 4:
            return 1;

        case 8: {
            uint64_t val = (uint64_t)lit->ival;
            if (val <= INT64_MAX) return 1;
            kl_error(loc, "Literal value out of range for int64");
            return 0;
        }

        default:
            return 0;
    }
}

static int expect_lit_as_uint8(Literal *lit, Loc loc, ParserState *ps)
{
    if (lit->sign) {
        switch (lit->len) {
            case 1: {
                int8_t val = (int8_t)lit->ival;
                if (val >= 0) return 1;
                kl_error(loc, "Literal value out of range for uint8");
                return 0;
            }

            case 2: {
                int16_t val = (int16_t)lit->ival;
                if (val >= 0 && val <= UINT8_MAX) return 1;
                kl_error(loc, "Literal value out of range for uint8");
                return 0;
            }

            case 4: {
                int32_t val = (int32_t)lit->ival;
                if (val >= 0 && val <= UINT8_MAX) return 1;
                kl_error(loc, "Literal value out of range for uint8");
                return 0;
            }

            case 8: {
                int64_t val = (int64_t)lit->ival;
                if (val >= 0 && val <= UINT8_MAX) return 1;
                kl_error(loc, "Literal value out of range for uint8");
                return 0;
            }

            default:
                return 0;
        }
    }

    switch (lit->len) {
        case 1:
            return 1;

        case 2: {
            uint16_t val = (uint16_t)lit->ival;
            if (val <= UINT8_MAX) return 1;
            kl_error(loc, "Literal value out of range for uint8");
            return 0;
        }

        case 4: {
            uint32_t val = (uint32_t)lit->ival;
            if (val <= UINT8_MAX) return 1;
            kl_error(loc, "Literal value out of range for uint8");
            return 0;
        }

        case 8: {
            uint64_t val = (uint64_t)lit->ival;
            if (val <= UINT8_MAX) return 1;
            kl_error(loc, "Literal value out of range for uint8");
            return 0;
        }

        default:
            return 0;
    }
}

static int expect_lit_as_uint16(Literal *lit, Loc loc, ParserState *ps)
{
    if (lit->sign) {
        switch (lit->len) {
            case 1: {
                int8_t val = (int8_t)lit->ival;
                if (val >= 0) return 1;
                kl_error(loc, "Literal value out of range for uint16");
                return 0;
            }

            case 2: {
                int16_t val = (int16_t)lit->ival;
                if (val >= 0) return 1;
                kl_error(loc, "Literal value out of range for uint16");
                return 0;
            }

            case 4: {
                int32_t val = (int32_t)lit->ival;
                if (val >= 0 && val <= UINT16_MAX) return 1;
                kl_error(loc, "Literal value out of range for uint16");
                return 0;
            }

            case 8: {
                int64_t val = (int64_t)lit->ival;
                if (val >= 0 && val <= UINT16_MAX) return 1;
                kl_error(loc, "Literal value out of range for uint16");
                return 0;
            }

            default:
                return 0;
        }
    }

    switch (lit->len) {
        case 1:
        case 2:
            return 1;

        case 4: {
            uint32_t val = (uint32_t)lit->ival;
            if (val <= UINT16_MAX) return 1;
            kl_error(loc, "Literal value out of range for uint16");
            return 0;
        }

        case 8: {
            uint64_t val = (uint64_t)lit->ival;
            if (val <= UINT16_MAX) return 1;
            kl_error(loc, "Literal value out of range for uint16");
            return 0;
        }

        default:
            return 0;
    }
}

static int expect_lit_as_uint32(Literal *lit, Loc loc, ParserState *ps)
{
    if (lit->sign) {
        switch (lit->len) {
            case 1: {
                int8_t val = (int8_t)lit->ival;
                if (val >= 0) return 1;
                kl_error(loc, "Literal value out of range for uint32");
                return 0;
            }

            case 2: {
                int16_t val = (int16_t)lit->ival;
                if (val >= 0) return 1;
                kl_error(loc, "Literal value out of range for uint32");
                return 0;
            }

            case 4: {
                int32_t val = (int32_t)lit->ival;
                if (val >= 0) return 1;
                kl_error(loc, "Literal value out of range for uint32");
                return 0;
            }

            case 8: {
                int64_t val = (int64_t)lit->ival;
                if (val >= 0 && val <= UINT32_MAX) return 1;
                kl_error(loc, "Literal value out of range for uint32");
                return 0;
            }

            default:
                return 0;
        }
    }

    switch (lit->len) {
        case 1:
        case 2:
        case 4:
            return 1;

        case 8: {
            uint64_t val = (uint64_t)lit->ival;
            if (val <= UINT32_MAX) return 1;
            kl_error(loc, "Literal value out of range for uint32");
            return 0;
        }

        default:
            return 0;
    }
}

static int expect_lit_as_uint64(Literal *lit, Loc loc, ParserState *ps)
{
    if (lit->sign) {
        switch (lit->len) {
            case 1: {
                int8_t val = (int8_t)lit->ival;
                if (val >= 0) return 1;
                kl_error(loc, "Literal value out of range for uint64");
                return 0;
            }

            case 2: {
                int16_t val = (int16_t)lit->ival;
                if (val >= 0) return 1;
                kl_error(loc, "Literal value out of range for uint64");
                return 0;
            }

            case 4: {
                int32_t val = (int32_t)lit->ival;
                if (val >= 0) return 1;
                kl_error(loc, "Literal value out of range for uint64");
                return 0;
            }

            case 8: {
                int64_t val = (int64_t)lit->ival;
                if (val >= 0) return 1;
                kl_error(loc, "Literal value out of range for uint64");
                return 0;
            }

            default:
                return 0;
        }
    }

    switch (lit->len) {
        case 1:
        case 2:
        case 4:
        case 8:
            return 1;

        default:
            return 0;
    }
}
