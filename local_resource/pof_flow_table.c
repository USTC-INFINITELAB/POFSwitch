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
#include "../include/pof_log_print.h"
#include "../include/pof_memory.h"
#include "string.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "string.h"
#include "net/if.h"
#include "sys/ioctl.h"
#include "arpa/inet.h"

#define LPM_TREE (1)

/* Key length of each type flow table. */
uint16_t poflr_key_len_each_type[POF_MAX_TABLE_TYPE];

/* Key length. */
uint32_t poflr_key_len = POFLR_KEY_LEN;

#define TABLE_TYPES         \
        TABLE_TYPE(MM)      \
        TABLE_TYPE(LPM)     \
        TABLE_TYPE(EM)

static hash_t
map_tableHashByID(uint8_t id)
{
    return hmap_hashForLinear(id);
}

static hash_t
map_tableHashByType(uint8_t type)
{
    return hmap_hashForLinear(type);
}

static void
map_tableInsert(struct tableInfo *table, struct pof_local_resource *lr)
{

    hmap_nodeInsert(lr->tableIdMap, &table->idNode);
//    hmap_nodeInsert(lr->tableTypeMap, &table->typeNode);
    lr->tableNum ++;
}

/* Malloc memory for table information. Should be FREE by map_tableDelete(). */
static struct tableInfo *
map_tableCreate()
{
    struct tableInfo *table;
    POF_MALLOC_SAFE_RETURN(table, 1, NULL);
    return table;
}

static void
map_tableDelete(struct tableInfo *table, struct pof_local_resource *lr)
{
    hmap_nodeDelete(lr->tableIdMap, &table->idNode);
//    hmap_nodeDelete(lr->tableTypeMap, &table->typeNode);
    lr->tableNum --;
    FREE(table);
}

struct tableInfo *
poflr_get_table_with_ID(uint8_t id, const struct pof_local_resource *lr)
{
    struct tableInfo *table, *ptr;
    return HMAP_STRUCT_GET(table, idNode, \
            map_tableHashByID(id),  lr->tableIdMap, ptr);
}

static hash_t
entryHashByID(uint32_t id)
{
    return hmap_hashForLinear(id);
}

static hash_t
entryHashByValue(const uint8_t *value, uint16_t len_b)
{
    return hmap_hashForBytes(value, POF_BITNUM_TO_BYTENUM_CEIL(len_b));
}

static uint32_t
valueMaskAssemble(const struct pof_match_x *match, uint8_t match_field_num, \
                  uint8_t *value, uint8_t *mask, uint16_t keyLen)
{
    uint16_t i, offset_b = 0;
    struct pof_match_x *matchTmp;

    /* Assemble matches to bytes. */
    for(i=0; i<match_field_num; i++){
        matchTmp = (struct pof_match_x *)(match + i);
        pofbf_cover_bit(value, matchTmp->value, offset_b, matchTmp->len);
        if(mask){
            pofbf_cover_bit(mask, matchTmp->mask, offset_b, matchTmp->len);
        }
        offset_b += matchTmp->len;
    }

    /* Check the match length in bit. */
    if(offset_b != keyLen){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_KEY_LEN, g_upward_xid++);
    }
    return POF_OK;
}

static uint32_t 
get1sCountInByte(uint8_t value)
{
    uint32_t count = 0;
    while(value > 0){
        count ++;
        value &= value - 1;
    }
    return count;
}

static uint32_t
get1sCountInBytes(uint8_t *value, uint32_t n)
{
    uint32_t count = 0, i;
    for(i=0; i<n; i++){
        count += get1sCountInByte(*(value + i));
    }
    return count;
}

static uint32_t
lpmInsert(const struct entryInfo *entry, struct tableInfo *table)
{
    uint32_t ret, bitNum;
    uint8_t value[POF_MAX_FIELD_LENGTH_IN_BYTE * POF_MAX_MATCH_FIELD_NUM];

    bitNum = get1sCountInBytes((uint8_t *)entry->mask, POF_BITNUM_TO_BYTENUM_CEIL(table->keyLen));
    memcpy(value, entry->value, POF_BITNUM_TO_BYTENUM_CEIL(table->keyLen));
    ret = tree_nodeInsert(table->tree, entry, value, bitNum);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    return POF_OK;
}

static uint32_t
lpmDelete(struct entryInfo *entry, struct tableInfo *table)
{
    uint32_t ret, bitNum;
    uint8_t value[POF_MAX_FIELD_LENGTH_IN_BYTE * POF_MAX_MATCH_FIELD_NUM];

    bitNum = get1sCountInBytes(entry->mask, POF_BITNUM_TO_BYTENUM_CEIL(table->keyLen));
    memcpy(value, entry->value, POF_BITNUM_TO_BYTENUM_CEIL(table->keyLen));
    ret = tree_nodeDelete(table->tree, value, bitNum);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    return POF_OK;
}

static struct entryInfo *
lpmLookup(uint8_t *key, const struct tableInfo *table)
{
    return tree_nodeLookup(table->tree, key, table->keyLen);
}

/* Fill the struct entryInfo *entry. 
 * Calculate the hash value.
 * Assemble the value and mask.*/
static uint32_t
entryFill(const struct pof_flow_entry *pofEntry,    \
          struct entryInfo *entry,                  \
          const struct tableInfo *table)
{
    uint32_t ret = POF_OK;
    entry->index = pofEntry->index;
    entry->counter_id = pofEntry->counter_id;
#ifdef POF_SHT_VXLAN
    entry->insBlockID = pofEntry->instruction_block_id;
    entry->paraLen = pofEntry->parameter_length;
    memcpy(entry->para, pofEntry->parameters, \
            POF_BITNUM_TO_BYTENUM_CEIL(entry->paraLen));
#else // POF_SHT_VXLAN
    entry->instruction_num = pofEntry->instruction_num;
    memcpy(entry->instruction, pofEntry->instruction, \
            POF_MAX_INSTRUCTION_NUM * sizeof(struct pof_instruction));
#endif // POF_SHT_VXLAN
    entry->match_field_num = pofEntry->match_field_num;
    memcpy(entry->match, pofEntry->match, \
            POF_MAX_MATCH_FIELD_NUM * sizeof(struct pof_match_x));

    switch(table->type){
        case POF_LPM_TABLE:
        case POF_MM_TABLE:
            entry->priority = pofEntry->priority;
            entry->keyLen = table->keyLen;
            ret = valueMaskAssemble(pofEntry->match, pofEntry->match_field_num, \
                    entry->value, entry->mask, table->keyLen) != POF_OK;
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            entry->node.hash = entryHashByID(pofEntry->index);
            break;
        case POF_EM_TABLE:
            entry->priority = pofEntry->priority;
            entry->keyLen = table->keyLen;
            ret = valueMaskAssemble(pofEntry->match, pofEntry->match_field_num, \
                    entry->value, NULL, table->keyLen) != POF_OK;
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            entry->node.hash = entryHashByValue(entry->value, table->keyLen);
            break;
        case POF_LINEAR_TABLE:
            entry->node.hash = entryHashByID(pofEntry->index);
            break;
        default:
            break;
    }
    return ret;
}

/* Malloc memory for entryInfo. Free at entryDelete. */
/* Transfer the struct pof_flow_entry *pofEntry to the struct entryInfo *entry.
 * Insert the entry into the table.*/
static uint32_t
entryInsert(const struct pof_flow_entry *pofEntry, struct tableInfo *table)
{
    uint32_t ret;
    /* Create entry node. */
    struct entryInfo *entry;
#ifdef POF_SHT_VXLAN
    uint16_t size;
    size = POF_BITNUM_TO_BYTENUM_CEIL(pofEntry->parameter_length);
    size += sizeof(struct entryInfo);
    POF_MALLOC_SAFE_RETURN_SIZE(entry, 1, POF_ERROR, size);
#else // POF_SHT_VXLAN
    POF_MALLOC_SAFE_RETURN(entry, 1, POF_ERROR);
#endif // POF_SHT_VXLAN

    /* Fill the entry's information. Including the hash value.
     * Assemble the value and mask of the entry. */
    if(entryFill(pofEntry, entry, table) != POF_OK){
        POF_ERROR_HANDLE_NO_RETURN_NO_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_UNKNOWN);
        FREE(entry);
        return POF_ERROR;
    }

    hmap_nodeInsert(table->entryMap, &entry->node);
    table->entryNum ++;

    if(table->type == POF_LPM_TABLE){
        lpmInsert(entry, table);
    }

    return POF_OK;
}

static void
entryDelete(struct entryInfo *entry, struct tableInfo *table)
{
    hmap_nodeDelete(table->entryMap, &entry->node);
    table->entryNum --;

    if(table->type == POF_LPM_TABLE){
        lpmDelete(entry, table);
    }

    FREE(entry);
}

static void
keyAssemble(uint8_t *key, const uint8_t *packet, const uint8_t *metadata, \
        uint8_t match_field_num, const struct pof_match *match)
{
    uint16_t i, offset_b = 0;
    struct pof_match *matchTmp;
    for(i=0; i<match_field_num; i++){
        uint8_t tmp[POF_MAX_FIELD_LENGTH_IN_BYTE] = {0};
        matchTmp = (struct pof_match *)(match + i);
        if(matchTmp->field_id == 0xFFFF){
            pofbf_copy_bit(metadata, tmp, matchTmp->offset, matchTmp->len);
        }else{
            pofbf_copy_bit(packet, tmp, matchTmp->offset, matchTmp->len);
        }
        pofbf_cover_bit(key, tmp, offset_b, matchTmp->len);
        offset_b += matchTmp->len;
    }
}

/* Entry lookup for Linear. */
struct entryInfo *
poflr_entry_lookup_Linear(uint32_t index, const struct tableInfo *table)
{
    struct entryInfo *entry, *ptr;
    return HMAP_STRUCT_GET(entry, node, \
            entryHashByID(index), table->entryMap, ptr);
}

static bool
maskMatch(const uint8_t *mask, const uint8_t *value, const uint8_t *key, uint16_t len_b)
{
    uint32_t i;
    for(i=0; i<POF_BITNUM_TO_BYTENUM_CEIL(len_b); i++){
        if((*(mask + i) & *(key + i)) != \
           (*(mask + i) & *(value + i))){
            return FALSE;
        }
    }
    return TRUE;
}

/* Entry lookup for MM. */
static struct entryInfo *
entryLookup_MM(const void *key, const struct tableInfo *table)
{
    struct entryInfo *entry, *next, *ret = NULL;

    /* Traverse all entries to lookup. */
    HMAP_NODES_IN_STRUCT_TRAVERSE(entry, next, node, table->entryMap){
        /* Match or not. */
        if(maskMatch(entry->mask, entry->value, (uint8_t *)key, table->keyLen)){
            if(!ret){
                /* First match. */
                ret = entry;
            }else{
                /* Multiple matches. Competition. */
                if(ret->priority < entry->priority){
                    ret = entry;
                }
            }
        }
    }
    return ret;
}

/* Entry lookup for EM. */
static struct entryInfo *
entryLookup_EM(const void *key, const struct tableInfo *table)
{
    struct entryInfo *entry, *ptr;
    return HMAP_STRUCT_GET(entry, node,                                         \
            entryHashByValue(key, table->keyLen),   \
            table->entryMap, ptr);
}

/* Entry lookup for LPM. */
static struct entryInfo *
entryLookup_LPM(const void *key, const struct tableInfo *table)
{
#ifdef LPM_TREE
    /* Find the entry using LPM tree. */
    return lpmLookup((uint8_t *)key, table);
#else // LPM_TREE
    struct entryInfo *entry, *next, *ret = NULL;
    uint32_t bitNum = 0, tmp;

    /* Traverse all entries to lookup. */
    HMAP_NODES_IN_STRUCT_TRAVERSE(entry, next, node, table->entryMap){
        /* Match or not. */
        if(maskMatch(entry->mask, entry->value, (uint8_t *)key, table->keyLen)){
            tmp = get1sCountInBytes(entry->mask, POF_BITNUM_TO_BYTENUM_CEIL(table->keyLen));
            if(tmp > bitNum){
                bitNum = tmp;
                ret = entry;
            }
        }
    }
    return ret;
#endif // LPM_TREE
}

/* This function shouldn't be executed. 
 * Use poflr_entry_lookup_Linear for Linear table lookup. */
/*
static struct entryInfo *
entryLookup_Linear(const void *key, const struct tableInfo *table)
{
    POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_BAD_MATCH, POFBMC_BAD_TYPE, g_upward_xid++);
    return NULL;
}
*/

/* Entry lookup for EM, MM, LPM type. */
struct entryInfo *
poflr_entry_lookup(const uint8_t *packet, const uint8_t *metadata, const struct tableInfo *table)
{
    struct entryInfo *entry;
    uint8_t *key;
    POF_MALLOC_SAFE_RETURN(key, POF_BITNUM_TO_BYTENUM_CEIL(table->keyLen), NULL);

    /* Assemble the find key. */
    keyAssemble(key, packet, metadata, table->match_field_num, table->match);

    /* Find the matched entry using different ways according to the table type. */
#define TABLE_TYPE(TYPE) \
            if(table->type == POF_##TYPE##_TABLE) {entry = entryLookup_##TYPE(key, table);}
    TABLE_TYPES
#undef TABLE_TYPE

    FREE(key);
    return entry;
}

/* Traverse to find the entry with the index. */
struct entryInfo *
poflr_entry_get_with_index(uint32_t index, const struct tableInfo *table)
{
    struct entryInfo *entry, *next;
    HMAP_NODES_IN_STRUCT_TRAVERSE(entry, next, node, table->entryMap){
        if(entry->index == index){
            return entry;
        }
    }
    return NULL;
}

uint32_t 
poflr_create_flow_table(uint8_t id,              \
                        uint8_t type,            \
                        uint16_t key_len,        \
                        uint32_t size,           \
				        char *name,              \
				        uint8_t match_field_num, \
				        pof_match *match,        \
                        struct pof_local_resource *lr)
{
    struct tableInfo *table;
    uint8_t ID;

    /* Check type. */
    if(type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_TYPE, g_recv_xid);
    }

    /* Check table_id. */
    if(id >= lr->tableNumMaxEachType[type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_ID, g_recv_xid);
    }

    poflr_table_id_to_ID(type, id, &ID, lr);

    /* Check whether the table have already existed. */
    if(poflr_get_table_with_ID(ID, lr)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_TABLE_EXIST, g_recv_xid);
    }

    /* Check table_size. */
    if(size > lr->tableSizeMax){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_SIZE, g_recv_xid);
    }

    /* Check table_key_length. */
    if(key_len > poflr_key_len_each_type[type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_KEY_LEN, g_recv_xid);
    }

    /* Malloc memory for table. */
    table = map_tableCreate();
    POF_MALLOC_ERROR_HANDLE_RETURN_UPWARD(table, g_upward_xid++);
    /* Fill the table information. */
    table->id = ID;
    table->idNode.hash = map_tableHashByID(ID);
    table->type = type;
//    table->typeNode.hash = map_tableHashByType(type);
    strncpy(table->name, name, TABLE_NAME_LEN);
    table->entryMap = hmap_create(size);
    table->entryNum = 0;
    table->size = size;
    table->keyLen = key_len;
    table->match_field_num = match_field_num;
    memcpy(table->match, match, match_field_num * sizeof(pof_match));
    if(table->type == POF_LPM_TABLE){
        table->tree = tree_create();
    }
    
    /* Insert the table to the local resource. */
    map_tableInsert(table, lr);

    POF_DEBUG_CPRINT_FL(1,GREEN,"Create flow table SUC!");
    return POF_OK;
}

uint32_t
poflr_delete_flow_table(uint8_t id, uint8_t type, struct pof_local_resource *lr)
{
    struct tableInfo *table;
    uint8_t ID;

    /* Check type. */
    if(type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_TYPE, g_recv_xid);
    }

    /* Check table_id. */
    if(id >= lr->tableNumMaxEachType[type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_ID, g_recv_xid);
    }

    poflr_table_id_to_ID(type, id, &ID, lr);

    /* Get the table. */
    if(!(table = poflr_get_table_with_ID(ID, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_TABLE_UNEXIST, g_recv_xid);
    }

    /* Check whether the table is empty. */
    if(table->entryNum){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_TABLE_UNEMPTY, g_recv_xid);
    }

    /* FREE the hash map of the entry in the table. */
    hmap_destroy(table->entryMap);
    if(table->type == POF_LPM_TABLE){
        /* FREE the tree of LPM entry. */
        tree_destroy(table->tree);
    }

    /* Delete the table from local resource and FREE the memory of table. */
    map_tableDelete(table, lr);

    POF_DEBUG_CPRINT_FL(1,GREEN,"Delete flow table SUC!");
    return POF_OK;
}

/***********************************************************************
 * Add a flow entry.
 * Form:     uint32_t poflr_add_flow_entry(pof_flow_entry *flow_ptr)
 * Input:    flow entry
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will add a new flow entry in the table. If a
 *           same flow entry is already exist in this table, ERROR.
 ***********************************************************************/
uint32_t poflr_add_flow_entry(pof_flow_entry *flow_ptr, struct pof_local_resource *lr){
    struct tableInfo *table;
    uint32_t index = flow_ptr->index, ret;
    uint8_t  table_id = flow_ptr->table_id;
    uint8_t  ID;
    uint8_t  table_type = flow_ptr->table_type;

	POF_LOG_LOCK_ON;
    poflp_flow_entry(flow_ptr);
    POF_DEBUG_CPRINT(1,WHITE,"\n");
	POF_LOG_LOCK_OFF;

    /* Check type. */
    if(table_type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_TYPE, g_recv_xid);
    }

    /* Check table_id. */
    if(table_id >= lr->tableNumMaxEachType[table_type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_ID, g_recv_xid);
    }

    poflr_table_id_to_ID(table_type, table_id, &ID, lr);
    /* Get the table. */
    if(!(table = poflr_get_table_with_ID(ID, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_ID, g_recv_xid);
    }

    /* Check the index. */
    if(index >= table->size){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_ENTRY_ID, g_recv_xid);
    }

    /* Create the entry, and insert to the table. */
    if(entryInsert(flow_ptr, table) != POF_OK){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_UNKNOWN, g_recv_xid);
    }

    /* Initialize the counter_id. */
	ret = poflr_counter_init(flow_ptr->counter_id, lr);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,GREEN,"Add flow entry SUC! Totally %d entries in this table.",
            table->entryNum);
    return POF_OK;
}

/***********************************************************************
 * Modify a flow entry.
 * Form:     uint32_t poflr_modify_flow_entry(pof_flow_entry *flow_ptr)
 * Input:    flow entry
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will modify a flow entry.
 ***********************************************************************/
uint32_t poflr_modify_flow_entry(pof_flow_entry *flow_ptr, struct pof_local_resource *lr){
    struct tableInfo *table;
    struct entryInfo *entry;
    uint32_t index = flow_ptr->index, ret;
    uint8_t  table_id = flow_ptr->table_id;
    uint8_t  table_type = flow_ptr->table_type;
    uint8_t ID;

	POF_LOG_LOCK_ON;
    poflp_flow_entry(flow_ptr);
    POF_DEBUG_CPRINT(1,WHITE,"\n");
	POF_LOG_LOCK_OFF;

    /* Check type. */
    if(table_type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_TYPE, g_recv_xid);
    }

    /* Check table_id. */
    if(table_id >= lr->tableNumMaxEachType[table_type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_ID, g_recv_xid);
    }

    poflr_table_id_to_ID(table_type, table_id, &ID, lr);
    /* Get the table.. */
    if(!(table = poflr_get_table_with_ID(ID, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_ID, g_recv_xid);
    }

    /* Check index. */
    if( index >= table->size ){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_ENTRY_ID, g_recv_xid);
    }
    
    /* Check whether the index have already existed. */
    if(!(entry = poflr_entry_get_with_index(index, table))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_ENTRY_UNEXIST, g_recv_xid);
    }

    /* Check the counter id in the flow entry. */
    if(entry->counter_id != flow_ptr->counter_id){
        /* Initialize the counter_id. */
        ret = poflr_counter_init(flow_ptr->counter_id, lr);
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    }

    /* Delete the original entry, then insert a new one. */
    entryDelete(entry, table);
    if(entryInsert(flow_ptr, table) != POF_OK){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_UNKNOWN, g_recv_xid);
    }

    POF_DEBUG_CPRINT_FL(1,GREEN,"Modify flow entry SUC!");
    return POF_OK;
}

/***********************************************************************
 * Delete a flow entry.
 * Form:     uint32_t poflr_delete_flow_entry(pof_flow_entry *flow_ptr)
 * Input:    flow entry
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will delete a flow entry in the flow table.
 ***********************************************************************/
uint32_t poflr_delete_flow_entry(pof_flow_entry *flow_ptr, struct pof_local_resource *lr){
    struct entryInfo *entry, *next;
    struct tableInfo *table;
    uint32_t index = flow_ptr->index, ret;
    uint8_t  table_id = flow_ptr->table_id;
    uint8_t  table_type = flow_ptr->table_type;
    uint8_t ID;

	POF_LOG_LOCK_ON;
    poflp_flow_entry(flow_ptr);
    POF_DEBUG_CPRINT(1,WHITE,"\n");
	POF_LOG_LOCK_OFF;

    /* Check type. */
    if(table_type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_TYPE, g_recv_xid);
    }

    /* Check table_id. */
    if(table_id >= lr->tableNumMaxEachType[table_type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_ID, g_recv_xid);
    }

    poflr_table_id_to_ID(table_type, table_id, &ID, lr);
    /* Get the table. */
    if(!(table = poflr_get_table_with_ID(ID, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_ID, g_recv_xid);
    }

    /* Check index. */
    if( index >= table->size ){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_ENTRY_ID, g_recv_xid);
    }

    /* Check whether the index have already existed. */
    if(!(entry = poflr_entry_get_with_index(index, table))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_ENTRY_UNEXIST, g_recv_xid);
    }

    /* Delete the counter. */
    ret = poflr_counter_delete(entry->counter_id, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Dlete the entry from the table, and FREE the memory. */
    entryDelete(entry, table);

    POF_DEBUG_CPRINT_FL(1,GREEN,"Delete flow entry SUC!");
    return POF_OK;
}

/* Initialize flow table resource. */
uint32_t poflr_init_flow_table(struct pof_local_resource *lr){
    uint32_t i;

    /* Initialize the key length of each table type. */
    for(i=0; i<POF_MAX_TABLE_TYPE; i++){
        poflr_key_len_each_type[i] = poflr_key_len;
    }

    lr->tableNumMax = 0;
    /* Calculate the flow table max number, and the table id base value. */
    for(i = 0; i < POF_MAX_TABLE_TYPE; i++){
        lr->tableNumMax += lr->tableNumMaxEachType[i];
    }

    /* Table map initialization. */
    lr->tableIdMap = hmap_create(lr->tableNumMax);
//    lr->tableTypeMap = hmap_create(lr->tableNumMax);
    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(lr->tableIdMap);
//    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(lr->tableTypeMap);

	return POF_OK;
}

/* Empty flow table. */
uint32_t poflr_empty_flow_table(struct pof_local_resource *lr){
    struct tableInfo *table, *nextTable;
    struct entryInfo *entry, *nextEntry;
    HMAP_NODES_IN_STRUCT_TRAVERSE(table, nextTable, idNode, lr->tableIdMap){
        /* Delete all entries. */
        HMAP_NODES_IN_STRUCT_TRAVERSE(entry, nextEntry, node, table->entryMap){
            entryDelete(entry, table);
        }
        /* FREE the hash map of entry in table. */
        hmap_destroy(table->entryMap);
        if(table->type == POF_LPM_TABLE){
            /* FREE the tree of LPM entry in table. */
            tree_destroy(table->tree);
        }
        /* Delete the table from local resource, and FREE the memory. */
        map_tableDelete(table, lr);
    }
	return POF_OK;
}

/***********************************************************************
 * Mapping the table id each type to global table ID.
 * Form:     uint32_t poflr_table_id_to_ID (uint8_t type, uint8_t id, uint8_t *ID_ptr)
 * Input:    table type, table id
 * Output:   global table ID.
 * Return:   POF_OK or Error code
 * Discribe: This function calculate the global table ID according to the
 *           table id each table type.
 ***********************************************************************/
uint32_t 
poflr_table_id_to_ID(uint8_t type,                      \
                     uint8_t id,                        \
                     uint8_t *ID,                       \
                     const struct pof_local_resource *lr)
{
    uint32_t i;

    for(i=0, *ID=0; i<type; i++){
        *ID += lr->tableNumMaxEachType[i];
    }
    *ID += id;

    return POF_OK;
}

/***********************************************************************
 * Mapping the global table ID to the table id each type.
 * Form:     uint32_t poflr_table_ID_to_id (uint8_t table_ID, uint8_t *type_ptr, uint8_t *table_id_ptr){
 * Input:    global table ID
 * Output:   table type, table id
 * Return:   POF_OK or Error code
 * Discribe: This function calculate the table type and the table id
 *           according to the global table ID.
 ***********************************************************************/
uint32_t 
poflr_table_ID_to_id(uint8_t ID,                    \
                     uint8_t *type,                 \
                     uint8_t *id,                   \
                     const struct pof_local_resource *lr)
{
    uint32_t i;
    for(i=0; i<POF_MAX_TABLE_TYPE; i++){
        if(ID >=lr->tableNumMaxEachType[i]){
            ID -= lr->tableNumMaxEachType[i];
            continue;
        }else{
            *id = ID;
            *type = i;
            return POF_OK;
        }
    }

    POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_BAD_TABLE_ID, g_upward_xid++);
}

/* Get key len. */
uint32_t poflr_get_key_len(uint16_t **key_len_ptrptr){
	*key_len_ptrptr = poflr_key_len_each_type;
	return POF_OK;
}

/* Set the key length. */
uint32_t poflr_set_key_len(uint32_t key_len){
	poflr_key_len = key_len;
	return POF_OK;
}

static uint32_t
reply_table(const struct tableInfo *table, const struct pof_local_resource *lr)
{
    struct pof_flow_table pofTable = {0};
    
    pofTable.command = POFTC_QUERY_RESULT;
    poflr_table_ID_to_id(table->id, &pofTable.type, &pofTable.tid, lr);
    pofTable.type = table->type;
    pofTable.match_field_num = table->match_field_num;
    pofTable.size = table->size;
    pofTable.key_len = table->keyLen;
    pofTable.slotID = lr->slotID;
    strncpy(pofTable.table_name, table->name, TABLE_NAME_LEN);
    memcpy(pofTable.match, table->match, POF_MAX_MATCH_FIELD_NUM * sizeof(struct pof_match));
    pof_NtoH_transfer_flow_table(&pofTable);

    if(POF_OK != pofec_reply_msg(POFT_TABLE_MOD, g_recv_xid, sizeof(pof_flow_table), (uint8_t *)&pofTable)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
    }
    return POF_OK;
}

static uint32_t
reply_entry(const struct entryInfo *entry, const struct tableInfo *table, \
        const struct pof_local_resource *lr)
{
    struct pof_flow_entry *pofEntry = NULL;
    uint32_t size, ret;

#ifdef POF_SHT_VXLAN
    size = sizeof(struct pof_flow_entry) + POF_BITNUM_TO_BYTENUM_CEIL(entry->paraLen);
#else // POF_SHT_VXLAN
    size = sizeof(struct pof_flow_entry);
#endif // POF_SHT_VXLAN
    pofEntry = (struct pof_flow_entry *)MALLOC(size);
    
    pofEntry->command = POFFC_QUERY_RESULT;
    pofEntry->match_field_num = entry->match_field_num;
    pofEntry->counter_id = entry->counter_id;
    poflr_table_ID_to_id(table->id, &pofEntry->table_type, &pofEntry->table_id, lr);
    pofEntry->table_type = table->type;
    pofEntry->priority = entry->priority;
    pofEntry->index = entry->index;
    pofEntry->slotID = lr->slotID;
    memcpy(pofEntry->match, entry->match, \
            POF_MAX_MATCH_FIELD_NUM * sizeof(struct pof_match_x));
#ifdef POF_SHT_VXLAN
    pofEntry->instruction_block_id = entry->insBlockID;
    pofEntry->parameter_length = entry->paraLen;
#else // POF_SHT_VXLAN
    pofEntry->instruction_num = entry->instruction_num;
    memcpy(pofEntry->instruction, entry->instruction, \
            POF_MAX_INSTRUCTION_NUM * sizeof(struct pof_instruction));
#endif // POF_SHT_VXLAN
    if(POF_OK != (ret = pof_HtoN_transfer_flow_entry(pofEntry))){
        FREE(pofEntry);
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    }

    if(POF_OK != pofec_reply_msg(POFT_FLOW_MOD, g_recv_xid, size, (uint8_t *)pofEntry)){
        FREE(pofEntry);
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
    }
    FREE(pofEntry);
    return POF_OK;
}

uint32_t
poflr_reply_table(const struct pof_local_resource *lr, uint8_t id, uint8_t type)
{
    struct tableInfo *table;
    uint32_t ret;
    uint8_t ID;
    
    /* Check type. */
    if(type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_TYPE, g_recv_xid);
    }
    /* Check table_id. */
    if(id >= lr->tableNumMaxEachType[type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_ID, g_recv_xid);
    }
    poflr_table_id_to_ID(type, id, &ID, lr);

    /* Check whether the table have already existed. */
    if((table = poflr_get_table_with_ID(ID, lr)) == NULL){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_TABLE_UNEXIST, g_recv_xid);
    }
    ret = reply_table(table, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    return POF_OK;
}

uint32_t
poflr_reply_table_all(const struct pof_local_resource *lr)
{
    uint32_t ret;
    struct tableInfo *table, *next;
    HMAP_NODES_IN_STRUCT_TRAVERSE(table, next, idNode, lr->tableIdMap){
        ret = reply_table(table, lr);
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    }
    return POF_OK;
}

uint32_t
poflr_reply_entry(const struct pof_local_resource *lr, uint8_t table_id, \
                  uint8_t table_type, uint32_t index)
{
    struct tableInfo *table;
    struct entryInfo *entry;
    uint32_t ret;
    uint8_t ID;
    
    /* Check type. */
    if(table_type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_TYPE, g_recv_xid);
    }
    /* Check table_id. */
    if(table_id >= lr->tableNumMaxEachType[table_type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_ID, g_recv_xid);
    }
    poflr_table_id_to_ID(table_type, table_id, &ID, lr);

    /* Check whether the table have already existed. */
    if((table = poflr_get_table_with_ID(ID, lr)) == NULL){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_TABLE_UNEXIST, g_recv_xid);
    }
    /* Check index. */
    if(index >= table->size){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_ENTRY_ID, g_recv_xid);
    }
    /* Check whether the index have already existed. */
    if((entry = poflr_entry_get_with_index(index, table)) == NULL){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_ENTRY_UNEXIST, g_recv_xid);
    }
    ret = reply_entry(entry, table, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    return POF_OK;
}

uint32_t
poflr_reply_entry_all(const struct pof_local_resource *lr)
{
    uint32_t ret;
    struct tableInfo *table, *tableNext;
    struct entryInfo *entry, *entryNext;
    HMAP_NODES_IN_STRUCT_TRAVERSE(table, tableNext, idNode, lr->tableIdMap){
        HMAP_NODES_IN_STRUCT_TRAVERSE(entry, entryNext, node, table->entryMap){
            ret = reply_entry(entry, table, lr);
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
        }
    }
    return POF_OK;
}

#undef TABLE_TYPES
