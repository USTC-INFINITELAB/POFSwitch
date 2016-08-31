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
#include <pthread.h>
#include "../include/pof_type.h"
#include "../include/pof_memory.h"
#include "../include/pof_log_print.h"
#include "../include/pof_global.h"
#include "../include/pof_list.h"

#define MEMORY_DIS_STR_LEN (100)

struct memoryNode {
    struct listNode node;
    void *ptr;
    size_t size;
    char dis[MEMORY_DIS_STR_LEN];
};

struct memoryList {
    struct list list;
    size_t totalSize;
};

struct memoryList memoryList = {
    { {&memoryList.list.nil, &memoryList.list.nil}, 0 }, 0
};

pthread_mutex_t memoryMutex = PTHREAD_MUTEX_INITIALIZER;

#define POF_MEMORY_LOCK_ON      pthread_mutex_lock(&memoryMutex);
#define POF_MEMORY_LOCK_OFF     pthread_mutex_unlock(&memoryMutex);

void * 
memory_malloc(size_t size, const char *file, int line)
{
    struct memoryNode *memory = (struct memoryNode *)malloc(sizeof(*memory));
    if(!memory){
        return NULL;
    }
    memory->ptr = malloc(size);
    if(!memory->ptr){
        return NULL;
    }
    memset(memory->ptr, 0, size);

    memory->size = size;
    snprintf(memory->dis, MEMORY_DIS_STR_LEN - 1, "%s:%d", file, line);

    POF_MEMORY_LOCK_ON;
    list_nodeInsertHead(&memoryList.list, &memory->node);
    memoryList.totalSize += size;
    POF_MEMORY_LOCK_OFF;

    return memory->ptr;
}

void 
memory_free(void *ptr)
{
    struct memoryNode *memory, *next;

    LIST_NODES_IN_STRUCT_TRAVERSE(memory, next, node, &memoryList.list){
        if(memory->ptr == ptr){
            POF_MEMORY_LOCK_ON;
            list_nodeDelete(&memoryList.list, &memory->node);
            if(memoryList.totalSize < memory->size){
                POF_PRINT_FL(1,YELLOW,"totalSize ERROR!");
            }
            memoryList.totalSize -= memory->size;
            POF_MEMORY_LOCK_OFF;
            
            free(memory->ptr);
            free(memory);
            return;
        }
    }
    POF_ERROR_CPRINT_FL("Memory free failed!");
}

static void
printOne(const struct memoryNode *memory)
{
    POF_PRINT(1,CYAN,"dis = %s, ptr = %p, size = %zu, node = %p, node->next = %p\n", \
            memory->dis, memory->ptr, memory->size, &memory->node, memory->node.next);
    POF_PRINT(1,YELLOW,"memoryList.list.nil = %p, next = %p, prev = %p\n", \
            &memoryList.list.nil, memoryList.list.nil.next, memoryList.list.nil.prev);
    return;
}

void 
memory_printAll()
{
    struct memoryNode *memory, *next;
    LIST_NODES_IN_STRUCT_TRAVERSE(memory, next, node, &memoryList.list){
        printOne(memory);
    }
    return;
}

size_t 
memory_totalSize()
{
    return memoryList.totalSize;
}

uint32_t 
memory_count()
{
    return memoryList.list.count;
}
