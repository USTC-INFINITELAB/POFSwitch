
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

#ifndef _POF_TREE_H_
#define _POF_TREE_H_

#include "pof_type.h"

struct treeNode {
    void *ptr;
    struct treeNode *leftSon;
    struct treeNode *rightSon;
};

struct tree {
    struct treeNode *root;
    uint32_t count;
};

struct tree * tree_create();
uint32_t tree_destroy(struct tree *);
uint32_t tree_clear(struct tree *tree);
struct treeNode * tree_nodeCreate();
void tree_nodeDestroy(struct treeNode **);
uint32_t tree_nodeInsert(struct tree *, const void *ptr, uint8_t *value, uint32_t bitNum);
uint32_t tree_nodeDelete(struct tree *, uint8_t *value, uint32_t bitNum);
void * tree_nodeLookup(const struct tree *, uint8_t *value, uint32_t bitNum);
uint32_t tree_nodeTrav(const struct tree *, uint32_t func(void *), void *);

#endif // _POF_TREE_H_
