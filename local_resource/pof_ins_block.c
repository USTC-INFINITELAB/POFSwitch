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

#include "../include/pof_common.h"
#include "../include/pof_type.h"
#include "../include/pof_global.h"
#include "../include/pof_local_resource.h"
#include "../include/pof_conn.h"
#include "../include/pof_byte_transfer.h"
#include "../include/pof_log_print.h"
#include "../include/pof_memory.h"

#ifdef POF_SHT_VXLAN


/* Malloc memory for instruction block information. Should be free by map_insBlockDelete(). */
static struct insBlockInfo *
map_insBlockCreate(uint16_t insSize)
{
    struct insBlockInfo *insBlock;
    uint16_t size = insSize + sizeof(struct insBlockInfo);
    POF_MALLOC_SAFE_RETURN_SIZE(insBlock, 1, NULL, size);
    return insBlock;
}

static void
map_insBlockDelete(struct insBlockInfo *insBlock, struct pof_local_resource *lr)
{
    hmap_nodeDelete(lr->insBlockMap, &insBlock->idNode);
    lr->insBlockNum --;
    FREE(insBlock);
}

static void
map_insBlockInsert(struct insBlockInfo *insBlock, struct pof_local_resource *lr)
{
    hmap_nodeInsert(lr->insBlockMap, &insBlock->idNode);
    lr->insBlockNum ++;
}

static hash_t
map_insBlockHashByID(uint32_t id)
{
    return hmap_hashForLinear(id);
}

/* Initialize instruction block resource. */
uint32_t 
poflr_init_insBlock(struct pof_local_resource *lr)
{
    lr->insBlockNumMax = POFLR_INS_BLOCK_NUM;
    /* Initialize meter table. */
    lr->meterMap = hmap_create(lr->meterNumMax);
    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(lr->meterMap);

	return POF_OK;
}

uint32_t 
poflr_add_insBlock(struct pof_instruction_block *pof_insBlock, struct pof_local_resource *lr)
{
    uint16_t insSize = 0;
    uint8_t insNum = pof_insBlock->instruction_num, i;
    struct pof_instruction *pof_ins = NULL;
    struct insBlockInfo *insBlock = NULL;

    if(poflr_get_insBlock_with_ID(pof_insBlock->instruction_block_id, lr)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_INSBLOCK_MOD_FAILED, POFIMFC_INSBLOCK_EXIST, g_recv_xid);
    }
    
    for(i=0; i<insNum; i++){
        pof_ins = (struct pof_instruction *)((uint8_t *)pof_insBlock->instruction + insSize);
        insSize += pof_ins->len;
    }

    insBlock = map_insBlockCreate(insSize);
    POF_MALLOC_ERROR_HANDLE_RETURN_UPWARD(insBlock, g_upward_xid++);
    insBlock->blockID = pof_insBlock->instruction_block_id;
    insBlock->idNode.hash = map_insBlockHashByID(insBlock->blockID);
    map_insBlockInsert(insBlock, lr);
    insBlock->insNum = insNum;
    insBlock->tableID = pof_insBlock->related_table_id;
    insBlock->lr = lr;
    memcpy(insBlock->insData, pof_insBlock->instruction, insSize);

    POF_DEBUG_CPRINT_FL(1,GREEN,"Add instruction block SUC!");
    return POF_OK;
}

uint32_t
poflr_delete_insBlock(uint16_t blockID, struct pof_local_resource *lr)
{
    struct insBlockInfo *insBlock;

    /* Get the insBlock. */
    if(!(insBlock = poflr_get_insBlock_with_ID(blockID, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_INSBLOCK_MOD_FAILED, POFIMFC_INSBLOCK_UNEXIST, g_recv_xid);
    }

    /* Delete the insBlock from local resource, and free the memory. */
    map_insBlockDelete(insBlock, lr);

    POF_DEBUG_CPRINT_FL(1,GREEN,"DELETE instruction block SUC!");
    return POF_OK;
}

uint32_t
poflr_modify_insBlock(struct pof_instruction_block *pof_insBlock, struct pof_local_resource *lr)
{
    uint32_t ret = POF_OK;

    /* Delete first. */
    ret = poflr_delete_insBlock(pof_insBlock->instruction_block_id, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    /* Add a new. */
    ret = poflr_add_insBlock(pof_insBlock, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    return ret;
}

uint32_t
poflr_empty_insBlock(struct pof_local_resource *lr)
{
    struct insBlockInfo *insBlock, *next;
    if(!lr || !lr->insBlockMap){
        return POF_OK;
    }
    /* Delete all insBlock. */
    HMAP_NODES_IN_STRUCT_TRAVERSE(insBlock, next, idNode, lr->insBlockMap){
        map_insBlockDelete(insBlock, lr);
    }
    return POF_OK;
}

struct insBlockInfo *
poflr_get_insBlock_with_ID(uint32_t id, const struct pof_local_resource *lr)
{
    struct insBlockInfo *insBlock, *ptr;
    return HMAP_STRUCT_GET(insBlock, idNode, \
            map_insBlockHashByID(id), lr->insBlockMap, ptr);
}


#endif // POF_SHT_VXLAN
