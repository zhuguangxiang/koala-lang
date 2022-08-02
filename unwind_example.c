#define UNW_LOCAL_ONLY
#include <inttypes.h>
#include <libunwind.h>
#include <stdio.h>
#include <stdlib.h>

// Call this function to get a backtrace.
void backtrace()
{
    unw_cursor_t cursor;
    unw_context_t context;

    // Initialize cursor to current frame for local unwinding.
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    // Unwind frames one by one, going up the frame stack.
    while (unw_step(&cursor) > 0) {
        unw_word_t offset, pc;
        unw_get_reg(&cursor, UNW_REG_IP, &pc);
        if (pc == 0) {
            break;
        }
        printf("0x%lx:", pc);

        char sym[256];
        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
            printf(" (%s+0x%lx)\n", sym, offset);
        } else {
            printf(" -- error: unable to obtain symbol name for this frame\n");
        }
    }
}

int fac(int n)
{
    if (n == 0) {
        backtrace();
        return 1;
    } else {
        return n * fac(n - 1);
    }
}

int main()
{
    fac(10);
    return 0;
}
