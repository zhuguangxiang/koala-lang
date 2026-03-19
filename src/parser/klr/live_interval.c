/*
 * This file is part of the koala project with MIT License.
 * Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
 */

#include "live_interval.h"

#ifdef __cplusplus
extern "C" {
#endif

void klr_interval_add_range(KlrInterval *interval, int start, int end)
{
    ASSERT(start <= end);

    if (list_empty(&interval->ranges)) {
        KlrRange *r = mm_alloc_obj(r);
        r->start = start;
        r->end = end;
        init_list(&r->link);
        list_push_back(&interval->ranges, &r->link);
        interval->start = start;
        interval->end = end;
        return;
    }

    KlrLiveRange *first = list_first_entry(&li->ranges, KlrLiveRange, link);

    /* 情况 1: 新区间与现有最前面的区间有重叠或相邻，直接合并 */
    if (start <= first->end && end >= first->start) {
        if (start < first->start) first->start = start;
        if (end > first->end) first->end = end;
    }
    /* 情况 2: 新区间在现有区间之前且不重叠 */
    else {
        KlrLiveRange *r = malloc(sizeof(KlrLiveRange));
        r->start = start;
        r->end = end;
        list_add(&r->link, &li->ranges); // 逆向扫描，新的总是插在前面
    }
}

int klr_interval_is_live_at(KlrInterval *interval, int pos)
{
    KlrRange *range;
    list_foreach(range, link, &interval->ranges) {
        if (pos >= range->start && pos <= range->end) {
            return 1;
        }
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
