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

#ifndef _POF_MEMORY_H_
#define _POF_MEMORY_H_

#include <stdlib.h>
#include "pof_log_print.h"
#include "pof_list.h"
#include "pof_type.h"

/* Debug for malloc memroy. */
//#define MALLOC_DEBUG


#ifdef MALLOC_DEBUG
#define MALLOC(size)    memory_malloc(size, __FILE__, __LINE__)
#define FREE(ptr)       memory_free(ptr)
#else // MALLOC_DEBUG
#define MALLOC(size)  malloc(size)
#define FREE(ptr)     free(ptr)
#endif // MALLOC_DEBUG

/* Macro for malloc memory. */
#define POF_MALLOC_SAFE_RETURN(ptr, count, ret)                             \
            ptr = (typeof(ptr)) MALLOC(count * sizeof(*(ptr)));             \
            if(!(ptr)){                                                     \
                POF_ERROR_HANDLE_NO_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, \
                        POF_ALLOCATE_RESOURCE_FAILURE);                     \
                return ret;                                                 \
            }                                                               \
            memset(ptr, 0, count * sizeof(*(ptr)))
#define POF_MALLOC_SAFE_RETURN_SIZE(ptr, count, ret, size)                  \
            ptr = (typeof(ptr)) MALLOC(count * size);                       \
            if(!(ptr)){                                                     \
                POF_ERROR_HANDLE_NO_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, \
                        POF_ALLOCATE_RESOURCE_FAILURE);                     \
                return ret;                                                 \
            }                                                               \
            memset(ptr, 0, count * size)

#define MEMORY_PRINT()                                                          \
            POF_PRINT_FL(1,YELLOW,"Memory print! totalSize = %zu, count = %u\n",  \
                         memory_totalSize(), memory_count());
#define MEMORY_PRINT_ALL()      \
            MEMORY_PRINT()      \
            memory_printAll()

void * memory_malloc(size_t size, const char *file, int line);
void memory_free(void *ptr);
void memory_printAll();
size_t memory_totalSize();
uint32_t memory_count();


#endif // _MEMORY_H_
