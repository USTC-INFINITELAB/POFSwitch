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
#include "string.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "string.h"
#include "net/if.h"
#include "sys/ioctl.h"
#include "arpa/inet.h"

/* Malloc memory for group information. Should be free by map_groupDelete(). */
static struct groupInfo *
map_groupCreate()
{
    struct groupInfo *group;
    POF_MALLOC_SAFE_RETURN(group, 1, NULL);
    return group;
}

static void
map_groupDelete(struct groupInfo *group, struct pof_local_resource *lr)
{
    hmap_nodeDelete(lr->groupMap, &group->idNode);
    lr->groupNum --;
    FREE(group);
}

static void
map_groupInsert(struct groupInfo *group, struct pof_local_resource *lr)
{
    hmap_nodeInsert(lr->groupMap, &group->idNode);
    lr->groupNum ++;
}

static hash_t
map_groupHashByID(uint32_t id)
{
    return hmap_hashForLinear(id);
}

/* Fill the group information, including the hash value. */
static void
groupFill(const struct pof_group *pofGroup, struct groupInfo *group)
{
    group->type = pofGroup->type;
    group->action_number = pofGroup->action_number;
    group->id = pofGroup->group_id;
    group->counter_id = pofGroup->counter_id;
    memcpy(group->action, pofGroup->action, \
            group->action_number * sizeof(struct pof_action));
    group->idNode.hash = map_groupHashByID(group->id);
}

/***********************************************************************
 * Add the group entry.
 * Form:     uint32_t poflr_modify_group_entry(pof_group *group_ptr)
 * Input:    device id, group id, counter id, group type, action number,
 *           action data
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will add the group entry in the group table.
 ***********************************************************************/
uint32_t 
poflr_add_group_entry(pof_group *group_ptr, struct pof_local_resource *lr)
{
    struct groupInfo *group;
    uint32_t ret;

    /* Check group_id. */
    if(group_ptr->group_id >= lr->groupNumMax){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_INVALID_GROUP, g_recv_xid);
    }
    if(poflr_get_group_with_ID(group_ptr->group_id, lr)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_GROUP_EXISTS, g_recv_xid);
    }

    /* Initialize the counter_id. */
    ret = poflr_counter_init(group_ptr->counter_id, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Create group and insert to the local resource. */
    group = map_groupCreate();
    POF_MALLOC_ERROR_HANDLE_RETURN_UPWARD(group, g_upward_xid++);
    groupFill(group_ptr, group);
    map_groupInsert(group, lr);

    POF_DEBUG_CPRINT_FL(1,GREEN,"Add group entry SUC!");
    return POF_OK;
}

/***********************************************************************
 * Modify the group entry.
 * Form:     uint32_t poflr_modify_group_entry(pof_group *group_ptr)
 * Input:    device id, group id, counter id, group type, action number,
 *           action data
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will modify the group entry in the group table.
 ***********************************************************************/
uint32_t 
poflr_modify_group_entry(pof_group *group_ptr, struct pof_local_resource *lr)
{
    struct groupInfo *group;

    /* Check group_id. */
    if(group_ptr->group_id >= lr->groupNumMax){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_INVALID_GROUP, g_recv_xid);
    }
    /* Get the group. */
    if(!(group = poflr_get_group_with_ID(group_ptr->group_id, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_UNKNOWN_GROUP, g_recv_xid);
    }

    /* Check the counter_id. */
    if(group_ptr->counter_id != group->counter_id){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_BAD_COUNTER_ID, g_recv_xid);
    }

    /* Modify the information of group. */
    groupFill(group_ptr, group);

    POF_DEBUG_CPRINT_FL(1,GREEN,"Modify group entry SUC!");
    return POF_OK;
}

/***********************************************************************
 * Delete the group entry.
 * Form:     uint32_t poflr_modify_group_entry(pof_group *group_ptr)
 * Input:    device id, group id, counter id, group type, action number,
 *           action data
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will delete the group entry in the group table.
 ***********************************************************************/
uint32_t 
poflr_delete_group_entry(pof_group *group_ptr, struct pof_local_resource *lr)
{
    struct groupInfo *group;
    uint32_t ret;

    /* Check group_id. */
    if(group_ptr->group_id >= lr->groupNumMax){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_INVALID_GROUP, g_recv_xid);
    }
    /* Get the group. */
    if(!(group = poflr_get_group_with_ID(group_ptr->group_id, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_UNKNOWN_GROUP, g_recv_xid);
    }

    /* Delete the counter_id. */
    ret = poflr_counter_delete(group_ptr->counter_id, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Delete the group from local resource, and free the memory. */
    map_groupDelete(group, lr);

    POF_DEBUG_CPRINT_FL(1,GREEN,"Delete group entry SUC!");
    return POF_OK;
}

/* Initialize group resource. */
uint32_t poflr_init_group(struct pof_local_resource *lr){

    /* Initialize group map. */
    lr->groupMap = hmap_create(lr->groupNumMax);
    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(lr->groupMap);

	return POF_OK;
}

/* Empty group. */
uint32_t 
poflr_empty_group(struct pof_local_resource *lr)
{
    struct groupInfo *group, *next;
    if(!lr || !lr->groupMap){
        return POF_OK;
    }
    /* Delete all groups. */
    HMAP_NODES_IN_STRUCT_TRAVERSE(group, next, idNode, lr->groupMap){
        map_groupDelete(group, lr);
    }
	return POF_OK;
}

/* Get group table. */
struct groupInfo *
poflr_get_group_with_ID(uint32_t id, const struct pof_local_resource *lr)
{
    struct groupInfo *group, *ptr;
    return HMAP_STRUCT_GET(group, idNode, \
            map_groupHashByID(id), lr->groupMap, ptr);
}

static uint32_t
reply(const struct groupInfo * group, const struct pof_local_resource *lr)
{
    struct pof_group pofGroup = {0};

    pofGroup.command = POFGC_QUERY_RESULT;
    pofGroup.slotID = lr->slotID;
    pofGroup.group_id = group->id;
    pofGroup.type = group->type;
    pofGroup.action_number = group->action_number;
    pofGroup.counter_id = group->counter_id;
    memcpy(pofGroup.action, group->action, POF_MAX_ACTION_NUMBER_PER_GROUP * sizeof(struct pof_action));
    pof_NtoH_transfer_group(&pofGroup);

    if(POF_OK != pofec_reply_msg(POFT_GROUP_MOD, g_recv_xid, sizeof(pof_group), (uint8_t *)&pofGroup)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
    }
    return POF_OK;
}

uint32_t
poflr_reply_group(const struct pof_local_resource *lr, uint32_t groupID)
{
    struct groupInfo *group;
    uint32_t ret;
    if((group = poflr_get_group_with_ID(groupID, lr)) == NULL){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFGMFC_UNKNOWN_GROUP, g_recv_xid);
    }
    ret = reply(group, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    return POF_OK;
}

uint32_t
poflr_reply_group_all(const struct pof_local_resource *lr)
{
    uint32_t ret;
    struct groupInfo *group, *next;
    HMAP_NODES_IN_STRUCT_TRAVERSE(group, next, idNode, lr->groupMap){
        ret = reply(group, lr);
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    }
    return POF_OK;
}
