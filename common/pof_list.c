/**
 * Copyright (c) 2012, 2013, Huawei Technologies Co., Ltd.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include "../include/pof_type.h"
#include "../include/pof_list.h"
#include "../include/pof_common.h"
#include "../include/pof_log_print.h"
#include "../include/pof_global.h"
#include "../include/pof_memory.h"

struct list *
list_create()
{
    struct list *list;
    POF_MALLOC_SAFE_RETURN(list, 1, NULL);
    list->nil.prev = &list->nil;
    list->nil.next = &list->nil;
    return list;
}

void
list_destroy(struct list *list)
{
    FREE(list);
}

void 
list_clear(struct list *list)
{
    list->count = 0;
    list->nil.prev = &list->nil;
    list->nil.next = &list->nil;
}

void 
list_nodeInsertTail(struct list *list, struct listNode *node)
{
    list->nil.prev->next = node;
    node->prev = list->nil.prev;
    node->next = &list->nil;
    list->nil.prev = node;

    list->count ++;
}

void 
list_nodeInsertHead(struct list *list, struct listNode *node)
{
    list->nil.next->prev = node;
    node->prev = &list->nil;
    node->next = list->nil.next;
    list->nil.next = node;

    list->count ++;
}

void
list_nodeDelete(struct list *list, struct listNode *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    list->count --;
}
