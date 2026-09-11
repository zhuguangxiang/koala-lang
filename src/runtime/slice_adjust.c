/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Sentinel value indicating that a slice bound was not specified. */
#define SLICE_SENTINEL INT64_MIN

/*
 * Normalize slice indices for a sequence of length `len`.
 *
 * Returns:
 *   1  - slice is non-empty; *_start and *_end are normalized and ready to use
 *   0  - slice is empty; caller should return an empty result without iterating
 *  -1  - invalid arguments (step == 0); caller must raise an exception
 *
 * The caller MUST branch on the return value before using *_start or *_end.
 *
 * Step is passed through unchanged; its sign determines the traversal direction.
 *
 * Output range when the slice is non-empty:
 *
 *   step > 0:
 *       start ∈ [0, len]
 *       end   ∈ [0, len]
 *       start < end
 *
 *   step < 0:
 *       start ∈ [-1, len - 1]
 *       end   ∈ [-1, len - 1]
 *       start > end
 *
 * Empty slice:
 *
 *   step > 0 && start >= end -> empty
 *   step < 0 && start <= end -> empty
 *
 * For backward slices, -1 is the exclusive boundary before index 0.
 *
 * A zero step is invalid according to Python slice semantics.
 *
 * The caller must ensure:
 *
 *   0 <= len <= INT64_MAX
 */
int slice_adjust(int64_t *_start, int64_t *_end, int64_t step, int64_t len)
{
    /* A zero step is invalid. */
    if (step == 0) return -1;

    int64_t start = *_start;
    int64_t end = *_end;

    bool forward = step > 0;

    /*
     * Resolve start
     *
     * Forward default:  start = 0
     * Backward default: start = len - 1
     */
    if (start == SLICE_SENTINEL) {
        start = (forward ? 0 : len - 1);
    } else if (start < 0) {
        start += len;
    } else {
        // nothing to do
    }

    /*
     * Resolve end
     *
     * Forward default:  end = len
     * Backward default: end = -1
     */
    if (end == SLICE_SENTINEL) {
        end = (forward ? len : -1);
    } else if (end < 0) {
        end += len;
    } else {
        // nothing to do
    }

    // Clamp bounds according to traversal direction
    if (forward) {
        // Clamp start and end to valid forward range [0, len]
        if (start < 0)
            start = 0;
        else if (start > len)
            start = len;

        if (end < 0)
            end = 0;
        else if (end > len)
            end = len;

        // Empty slice: no elements to traverse in forward direction
        if (start >= end) {
            *_start = 0;
            *_end = 0;
            return 0;
        }
    } else {
        // Clamp start and end to valid backward range [-1, len - 1]
        // -1 is a legitimate backward boundary meaning "before index 0"
        if (start >= len)
            start = len - 1;
        else if (start < -1)
            start = -1;

        if (end >= len)
            end = len - 1;
        else if (end < -1)
            end = -1;

        // Empty slice: no elements to traverse in backward direction
        if (start <= end) {
            *_start = 0;
            *_end = 0;
            return 0;
        }
    }

    // Write back normalized indices
    *_start = start;
    *_end = end;
    return 1;
}

#ifdef __cplusplus
}
#endif
