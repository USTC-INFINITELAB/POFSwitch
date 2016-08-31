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

#include "../include/pof_tree.h"
#include "../include/pof_log_print.h"
#include "../include/pof_global.h"
#include "../include/pof_memory.h"

struct tree * 
tree_create()
{
    struct tree *tree;
    POF_MALLOC_SAFE_RETURN(tree, 1, NULL);
    tree->root = tree_nodeCreate();
    if(!tree->root){                                                \
        POF_ERROR_HANDLE_NO_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, \
                POF_ALLOCATE_RESOURCE_FAILURE);                 \
        return NULL;                                                \
    }                                                               \
    return tree;
}

uint32_t 
tree_destroy(struct tree *tree)
{
    tree_nodeDestroy(&tree->root);
    FREE(tree);
    return POF_OK;
}

uint32_t 
tree_clear(struct tree *tree)
{
    if(tree){
        tree_nodeDestroy(&tree->root);
    }
    tree->count = 0;
    return POF_OK;
}

struct treeNode * 
tree_nodeCreate()
{
    struct treeNode *node = NULL;
    POF_MALLOC_SAFE_RETURN(node, 1, NULL);
    return node;
}

void
tree_nodeDestroy(struct treeNode **node)
{
    if((*node)->leftSon){
        tree_nodeDestroy(&(*node)->leftSon);
    }
    if((*node)->rightSon){
        tree_nodeDestroy(&(*node)->rightSon);
    }
    FREE(*node);
    *node = NULL;
    return;
}

static void
moveLeft(uint8_t *value, uint32_t bitNum)
{
    uint32_t i;
    if(!bitNum){
        return;
    }
    for(i=0; i<(POF_BITNUM_TO_BYTENUM_CEIL(bitNum)-1); i++){
        *(value + i) = (*(value + i) << 1) | (*(value + i + 1) >> 7);
    }
    *(value + i) = *(value + i) << 1;
}

static bool
toLeft(const uint8_t *value)
{
    return ((*value & 0x80) != 0x80);
}

static bool
isLeaf(const struct treeNode *node)
{
    return ( !node->leftSon && !node->rightSon );
}

static bool
isRoot(const struct treeNode *node, const struct tree *tree)
{
    return ( node == tree->root );
}

uint32_t 
tree_nodeInsert(struct tree *tree, const void *ptr, uint8_t *value, uint32_t bitNum)
{
    struct treeNode ** node = &tree->root;
    while(bitNum > 0){
        node = (toLeft(value)) ? &(*node)->leftSon : &(*node)->rightSon;
        moveLeft(value, bitNum);
        bitNum --;

        if(!(*node)){
            *node = tree_nodeCreate();
            POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(*node);
        }
    }
    
    (*node)->ptr = (void *)ptr;
    tree->count ++;
    return POF_OK;
}

uint32_t 
tree_nodeDelete(struct tree *tree, uint8_t *value, uint32_t bitNum)
{
    struct treeNode ** node = &tree->root;
    while(bitNum > 0){
        node = (toLeft(value)) ? &(*node)->leftSon : &(*node)->rightSon;
        moveLeft(value, bitNum);
        bitNum --;

        if(!(*node)){
            return POF_ERROR;
        }
    }

    (*node)->ptr = NULL;
    if(isLeaf(*node) && !isRoot(*node, tree)){
        FREE(*node);
        *node = NULL;
    }

    tree->count --;
    return POF_OK;
}

void * 
tree_nodeLookup(const struct tree *tree, uint8_t *value, uint32_t bitNum)
{
    struct treeNode *next = tree->root, *node;
    void *ptr = NULL;
    while(next){
        node = next;
        if(node->ptr){
            ptr = node->ptr;
        }
        next = (toLeft(value)) ? node->leftSon : node->rightSon;
        moveLeft(value,bitNum);
        bitNum --;
    }
    return ptr;
}

static uint32_t
nodeTrav(const struct treeNode *node, uint32_t func(void *), void *arg)
{
    if(!node){
        return POF_OK;
    }
    if((nodeTrav(node->leftSon, func, arg) != POF_OK) || \
            nodeTrav(node->rightSon, func, arg) != POF_OK){
        return POF_ERROR;
    }

    if(node->ptr){
        return func((struct treeNode *)node);
    }
    return POF_OK;
}

uint32_t
tree_nodeTrav(const struct tree *tree, uint32_t func(void *), void *arg)
{
    return nodeTrav(tree->root, func, arg);
}
