
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

#ifndef _POF_HMAP_H_
#define _POF_HMAP_H_

#include <stddef.h>
#include "pof_type.h"
#include "pof_global.h"

struct hnode {
    hash_t hash;
    struct hnode *next;
};

struct hmap {
    struct hnode **buckets;
    hash_t mask;
    hash_t n;
};

#define HMAP_STRUCT_GET(obj, node, hash, map, ptr)              \
            ( (ptr = (void *)hmap_nodeGetWithHash(map, hash)) ? \
              POF_STRUCT_FROM_MEMBER(obj, node, ptr) : NULL )

#define HMAP_BUCKETS_COUNT(map) (map->mask + 1)
#define HMAP_NODES_COUNT(map) (map->n)

#define HMAP_NODES_IN_STRUCT_TRAVERSE(obj, next, node, map)                     \
            for( obj = POF_STRUCT_FROM_MEMBER(obj, node, hmap_nodeFirst(map));  \
                 &((obj)->node) &&                                              \
                 (next = POF_STRUCT_FROM_MEMBER(next, node,                     \
                     hmap_nodeNext(map, &((obj)->node)) ), 1);                  \
                 obj = next)

struct hmap * hmap_create(hash_t size);
struct hmap * hmap_destroy(struct hmap *hmap);
void hmap_clear(struct hmap *);
bool hmap_empty(const struct hmap *);
hash_t hmap_nodesCount(const struct hmap *);
hash_t hmap_nodesCountMax(const struct hmap *);
struct hnode * hmap_nodeGetWithHash(const struct hmap *, hash_t);
struct hnode * hmap_nodeGetWithHashNext(const struct hnode *, hash_t);
hash_t hmap_nodePosBktId(const struct hmap *, const struct hnode *);
hash_t hmap_nodePosDeep(const struct hmap *, const struct hnode *);
struct hnode * hmap_nodeNext(const struct hmap *, const struct hnode *);
struct hnode * hmap_nodeFirst(const struct hmap *);
void hmap_nodeInsert(struct hmap *, struct hnode *);
void hmap_nodeDelete(struct hmap *, struct hnode *);
bool hmap_nodeContain(const struct hmap *, const struct hnode *);
uint32_t hmap_nodeTrav(const struct hmap *, uint32_t func(void *), void *);

hash_t hmap_hashForLinear(uint32_t);
hash_t hmap_hashForUint32(uint32_t);
hash_t hmap_hashForBytes(const void *, size_t n);
hash_t hmap_hashForString(const char *);

#endif // _POF_HMAP_H_
