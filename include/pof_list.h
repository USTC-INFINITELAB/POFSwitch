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

#ifndef _POF_LIST_H_
#define _POF_LIST_H_

#include "pof_type.h"
#include "pof_common.h"

struct listNode {
    struct listNode *prev;
    struct listNode *next;
};

struct list{
    struct listNode nil;
    uint32_t count;
};

#define LIST_NODES_IN_STRUCT_TRAVERSE(obj,next,node,list)                           \
            for( obj = POF_STRUCT_FROM_MEMBER(obj,node,(list)->nil.next);           \
                 ( &((obj)->node) != &(list)->nil ) &&                              \
                 (next = POF_STRUCT_FROM_MEMBER(next,node,(obj)->node.next), 1);    \
                 obj = next )
#define LIST_NODES_IN_STRUCT_TRAVERSE_REVERSE(obj,next,node,list)                   \
            for( obj = POF_STRUCT_FROM_MEMBER(obj,node,(list)->nil.prev);           \
                 ( &((obj)->node) != &(list)->nil ) &&                              \
                 (next = POF_STRUCT_FROM_MEMBER(next,node,(obj)->node.prev), 1);    \
                 obj = next )
#define LIST_CLEAR_ALL(obj,next,node,list)                          \
            LIST_NODES_IN_STRUCT_TRAVERSE(obj,next,node,(list)){    \
                FREE(obj);                                          \
            }                                                       \
            list_clear(list)

struct list *list_create();
void list_destroy(struct list *);
void list_clear(struct list *);

void list_nodeInsertTail(struct list *, struct listNode *);
void list_nodeInsertHead(struct list *, struct listNode *);
void list_nodeDelete(struct list *, struct listNode *node);


#endif // _LIST_H_
