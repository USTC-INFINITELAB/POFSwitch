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
#include "../include/pof_log_print.h"
#include "../include/pof_hmap.h"
#include "../include/pof_global.h"
#include "../include/pof_memory.h"

#define HASH_S (0x9E3779B9)

#define BUCKETS_TRAVERSE(map,bkt,start)                         \
            for( bkt = map->buckets + start;                    \
                 (bkt - map->buckets) < HMAP_BUCKETS_COUNT(map); \
                 bkt++ )

#define NODES_TRAVERSE_IN_BUCKET(node,bkt)                    \
            for( node = *bkt; node; node = node->next )

/* bktSize should be 2^x. */
static uint32_t 
hmapInit(struct hmap *map, hash_t bktSize)
{
    POF_MALLOC_SAFE_RETURN(map->buckets, bktSize, POF_ERROR);
    map->mask = bktSize - 1;
    map->n = 0;
    return POF_OK;
}

static hash_t
bucketsNum(hash_t size)
{
    size = size  >> 1;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
//    size |= size >> 32;
    return (size + 1);
}

struct hmap * 
hmap_create(hash_t size)
{
    struct hmap *map;

    POF_MALLOC_SAFE_RETURN(map, 1, NULL);
    if(hmapInit(map, bucketsNum(size)) != POF_OK){
        return NULL;
    }
    return map;
}

struct hmap * 
hmap_destroy(struct hmap *map)
{
    FREE(map->buckets);
    FREE(map);
    return NULL;
}

void 
hmap_clear(struct hmap *map)
{
    struct hnode **bkt;
    BUCKETS_TRAVERSE(map, bkt, 0){
        *bkt = NULL;
    }
    map->n = 0;
}

bool
hmap_empty(const struct hmap *map)
{
    return (map->n == 0);
}

hash_t 
hmap_nodesCountMax(const struct hmap *map)
{
    return 2 * map->mask + 1;
}

static struct hnode **
bucketWithHash(const struct hmap *map, hash_t hash)
{
    return map->buckets + (map->mask & hash);
}

struct hnode * 
hmap_nodeGetWithHash(const struct hmap *map, hash_t hash)
{
    struct hnode *node;
    NODES_TRAVERSE_IN_BUCKET(node,bucketWithHash(map,hash)){
        if(node->hash == hash){
            return node;
        }
    }
    return node;
}

struct hnode * 
hmap_nodeGetWithHashNext(const struct hnode *node, hash_t hash)
{
    for(; node, node->hash!=hash; node=node->next){
        continue;
    }
    return (struct hnode *)node;
}

hash_t 
hmap_nodePosBktId(const struct hmap *map, const struct hnode *node)
{
    return (map->mask & node->hash);
}

hash_t 
hmap_nodePosDeep(const struct hmap *map, const struct hnode *node)
{
    struct hnode *tmp;
    hash_t deep = 0;
    NODES_TRAVERSE_IN_BUCKET(tmp, bucketWithHash(map,node->hash)){
        if(tmp == node){
            return deep;
        }
        deep ++;
    }
    return POF_ERROR;
}

struct hnode * 
hmap_nodeNext(const struct hmap *map, const struct hnode *node)
{
    struct hnode **bkt;
    if(node->next){
        return node->next;
    }
    BUCKETS_TRAVERSE(map,bkt,((map->mask & node->hash) + 1)){
        if(*bkt){
            return *bkt;
        }
    }
    return NULL;
}

void 
hmap_nodeInsert(struct hmap *map, struct hnode *node)
{
    struct hnode **bkt = bucketWithHash(map, node->hash);
    node->next = *bkt;
    *bkt = node;
    map->n ++;
}

void 
hmap_nodeDelete(struct hmap *map, struct hnode *node)
{
    struct hnode **pnode;
    for(pnode=bucketWithHash(map,node->hash); *pnode; pnode=&(*pnode)->next){
        if(*pnode == node){
            *pnode = node->next;
            map->n --;
            return;
        }
    }
}

bool 
hmap_nodeContain(const struct hmap *map, const struct hnode *node)
{
    struct hnode *tmp;
    NODES_TRAVERSE_IN_BUCKET(tmp,bucketWithHash(map,node->hash)){
        if(tmp == node){
            return TRUE;
        }
    }
    return FALSE;
}

uint32_t 
hmap_nodeTrav(const struct hmap *map, uint32_t func(void *), void *arg)
{
    struct hnode **bkt, *node;
    BUCKETS_TRAVERSE(map,bkt,0){
        NODES_TRAVERSE_IN_BUCKET(node,bkt){
            if(func(arg) != POF_OK){
                return POF_ERROR;
            }
        }
    }
    return POF_OK;
}

struct hnode * 
hmap_nodeFirst(const struct hmap *map)
{
    struct hnode **bkt;
    BUCKETS_TRAVERSE(map,bkt,0){
        if(*bkt){
            return *bkt;
        }
    }
    return NULL;
}

hash_t 
hmap_hashForLinear(uint32_t value)
{
    return value;
}

hash_t hmap_hashForUint32(uint32_t value)
{
    value = value + 3 * (value >> 8) + 7 * (value >> 16) + 13 * (value >> 24);
    return (uint32_t)(value * HASH_S);
}

hash_t hmap_hashForBytes(const void *value, size_t n)
{
    uint32_t *p, left = 0;
    hash_t hash = 0;
    for(p = (uint32_t *)value; n >= 4; n -= 4, p ++){
        hash ^= hmap_hashForUint32(*p);
    }
    if(n != 0){
        memcpy(&left, p, n);
        hash ^= hmap_hashForUint32(left);
    }
    return hash;
}

hash_t 
hmap_hashForString(const char *str)
{
    return hmap_hashForBytes(str, strlen(str));
}
