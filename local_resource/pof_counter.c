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

/* Malloc memory for counter information. Should be free by map_counterDelete(). */
static struct counterInfo *
map_counterCreate()
{
    struct counterInfo *counter;
    POF_MALLOC_SAFE_RETURN(counter, 1, NULL);
    return counter;
}

static void
map_counterDelete(struct counterInfo *counter, struct pof_local_resource *lr)
{
    hmap_nodeDelete(lr->counterMap, &counter->idNode);
    lr->counterNum --;
    FREE(counter);
}

static void
map_counterInsert(struct counterInfo *counter, struct pof_local_resource *lr)
{
    hmap_nodeInsert(lr->counterMap, &counter->idNode);
    lr->counterNum ++;
}

static hash_t
map_counterHashByID(uint32_t id)
{
    return hmap_hashForLinear(id);
}

/***********************************************************************
 * Initialize the counter corresponding the counter_id.
 * Form:     uint32_t poflr_counter_init(uint32_t counter_id, \
 *                                       struct pof_local_resource *lr)
 * Input:    counter id
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function will initialize the counter corresponding the
 *           counter_id. The initial counter value is zero.
 ***********************************************************************/
uint32_t 
poflr_counter_init(uint32_t counter_id, struct pof_local_resource *lr)
{
    struct counterInfo *counter;

	if(!counter_id){
		POF_DEBUG_CPRINT_FL(1,GREEN,"The counter_id 0 means that counter is no need.");
		return POF_OK;
	}

    /* Check counter id. */
    if(counter_id >= lr->counterNumMax){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_BAD_COUNTER_ID, g_recv_xid);
    }
    if(!poflr_get_counter_with_ID(counter_id, lr)){
        counter = map_counterCreate();
        POF_MALLOC_ERROR_HANDLE_RETURN_UPWARD(counter, g_upward_xid++);
        counter->id = counter_id;
        counter->idNode.hash = map_counterHashByID(counter_id);
        map_counterInsert(counter, lr);
    }

    POF_DEBUG_CPRINT_FL(1,GREEN,"The counter[%u] has been initialized!", counter_id);
    return POF_OK;
}

/***********************************************************************
 * Delete the counter corresponding the counter_id.
 * Form:     uint32_t poflr_counter_delete(uint32_t counter_id \)
 *                                       struct pof_local_resource *lr)
 * Input:    counter id
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function will delete the counter.
 ***********************************************************************/
uint32_t 
poflr_counter_delete(uint32_t counter_id, struct pof_local_resource *lr)
{
    struct counterInfo *counter;

    if(!counter_id){
        return POF_OK;
    }

    /* Check counter id. */
    if(counter_id >= lr->counterNumMax){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_BAD_COUNTER_ID, g_recv_xid);
    }
    if(!(counter = poflr_get_counter_with_ID(counter_id, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_COUNTER_UNEXIST, g_recv_xid);
    }

    map_counterDelete(counter, lr);

    POF_DEBUG_CPRINT_FL(1,GREEN,"The counter[%u] has been deleted!", counter_id);
    return POF_OK;
}

/***********************************************************************
 * Cleare counter value.
 * Form:     uint32_t poflr_counter_clear(uint32_t counter_id \)
 *                                       struct pof_local_resource *lr)
 * Input:    device id, counter id
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will make the counter value corresponding to
 *           counter_id to be zero.
 ***********************************************************************/
uint32_t 
poflr_counter_clear(uint32_t counter_id, struct pof_local_resource *lr)
{
    struct counterInfo *counter;
    if(!counter_id){
        return POF_OK;
    }

    /* Check counter_id. */
    if(counter_id >= lr->counterNumMax){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_BAD_COUNTER_ID, g_recv_xid);
    }
    if(!(counter = poflr_get_counter_with_ID(counter_id, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_COUNTER_UNEXIST, g_recv_xid);
    }

    /* Initialize the counter value. */
    counter->value = 0;
#ifdef POF_SD2N
    counter->byte_value = 0;
#endif // POF_SD2N

    POF_DEBUG_CPRINT_FL(1,GREEN,"Clear counter[%u] value SUC!", counter_id);
    return POF_OK;
}

/***********************************************************************
 * Get counter value.
 * Form:     uint32_t poflr_get_counter_value(uint32_t counter_id \)
 *                                       struct pof_local_resource *lr)
 * Input:    device id, counter id
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will get the counter value corresponding to
 *           tht counter_id and send it to the Controller as reply
 *           through the OpenFlow channel.
 ***********************************************************************/
uint32_t 
poflr_get_counter_value(uint32_t counter_id, struct pof_local_resource *lr)
{
    struct counterInfo *counter;
    pof_counter pofCounter = {0};

    /* Check counter_id. */
    if(!counter_id || counter_id >= lr->counterNumMax){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_BAD_COUNTER_ID, g_recv_xid);
    }
    if(!(counter = poflr_get_counter_with_ID(counter_id, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_COUNTER_UNEXIST, g_recv_xid);
    }

#ifdef POF_MULTIPLE_SLOTS
    pofCounter.command = POFCC_QUERY_RESULT;
    pofCounter.slotID = lr->slotID;
#else // POF_MULTIPLE_SLOTS
    pofCounter.command = POFCC_QUERY;
#endif // POF_MULTIPLE_SLOTS
    pofCounter.counter_id = counter_id;
    pofCounter.value = counter->value;
#ifdef POF_SD2N
    pofCounter.byte_value = counter->byte_value;
#endif // POF_SD2N
    pof_NtoH_transfer_counter(&pofCounter);

	/* Delay 0.1s. */
	pofbf_task_delay(100);

    if(POF_OK != pofec_reply_msg(POFT_COUNTER_REPLY, g_recv_xid, sizeof(pof_counter), (uint8_t *)&pofCounter)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
    }

#ifdef POF_SD2N
    POF_DEBUG_CPRINT_FL(1,GREEN,"Get counter value SUC! counter id = %u, counter value = %" \
            POF_PRINT_FORMAT_U64", byte_value = %"POF_PRINT_FORMAT_U64, \
                        counter_id, counter->value, counter->byte_value);
#else // POF_SD2N
    POF_DEBUG_CPRINT_FL(1,GREEN,"Get counter value SUC! counter id = %u, counter value = %"POF_PRINT_FORMAT_U64, \
                        counter_id, counter->value);
#endif // POF_SD2N
    return POF_OK;
}

uint32_t
poflr_reply_counter_all(const struct pof_local_resource *lr)
{
    struct pof_counter pofCounter = {0};
    struct counterInfo *counter, *next;

    HMAP_NODES_IN_STRUCT_TRAVERSE(counter, next, idNode, lr->counterMap){
        pofCounter.command = POFCC_QUERY_RESULT;
#ifdef POF_MULTIPLE_SLOTS
        pofCounter.slotID = lr->slotID;
#endif // POF_MULTIPLE_SLOTS
        pofCounter.counter_id = counter->id;
        pofCounter.value = counter->value;
#ifdef POF_SD2N
        pofCounter.byte_value = counter->byte_value;
#endif // POF_SD2N
        pof_NtoH_transfer_counter(&pofCounter);

        if(POF_OK != pofec_reply_msg(POFT_COUNTER_REPLY, g_recv_xid, sizeof(pof_counter), (uint8_t *)&pofCounter)){
            POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
        }
    }
}

/***********************************************************************
 * Increace the counter
 * Form:     uint32_t poflr_counter_increace(uint32_t counter_id, \)
 *                      uint16_t byte_len, struct pof_local_resource *lr)
 * Input:    counter_id
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function increase the counter value corresponding to
 *           the counter_id by one.
 ***********************************************************************/
#ifdef POF_SD2N
uint32_t 
poflr_counter_increace(uint32_t counter_id, uint16_t byte_len, struct pof_local_resource *lr)
{
#else // POF_SD2N
uint32_t 
poflr_counter_increace(uint32_t counter_id, struct pof_local_resource *lr)
{
#endif // POF_SD2N
    struct counterInfo *counter;

    if(!counter_id){
        return POF_OK;
    }

    /* Check the counter id. */
    if(counter_id >= lr->counterNumMax){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_BAD_COUNTER_ID, g_upward_xid++);
    }
    if(!(counter = poflr_get_counter_with_ID(counter_id, lr))){
		poflr_counter_init(counter_id, lr);
    }

    counter->value ++;
#ifdef POF_SD2N
    counter->byte_value += byte_len;
#endif // POF_SD2N

#ifdef POF_SD2N
    POF_DEBUG_CPRINT_FL(1,GREEN,"The counter %d has increased, value = %" \
            POF_PRINT_FORMAT_U64", byte_value = %"POF_PRINT_FORMAT_U64, \
            counter_id, counter->value, counter->byte_value);
#else // POF_SD2N
    POF_DEBUG_CPRINT_FL(1,GREEN,"The counter %d has increased, value = %"POF_PRINT_FORMAT_U64, \
            counter_id, counter->value);
#endif // POF_SD2N
    return POF_OK;
}

/* Initialize counter resource. */
uint32_t poflr_init_counter(struct pof_local_resource *lr){

    /* Initialize counter table. */
    lr->counterMap = hmap_create(lr->counterNumMax);
    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(lr->counterMap);

	return POF_OK;
}

/* Empty counter. */
uint32_t poflr_empty_counter(struct pof_local_resource *lr){
    struct counterInfo *counter, *next;
    if(!lr || !lr->counterMap){
        return POF_OK;
    }
    HMAP_NODES_IN_STRUCT_TRAVERSE(counter, next, idNode, lr->counterMap){
        map_counterDelete(counter, lr);
    }
	return POF_OK;
}

/* Get counter table. */
struct counterInfo *
poflr_get_counter_with_ID(uint32_t id, const struct pof_local_resource *lr)
{
    struct counterInfo *counter, *ptr;
    return HMAP_STRUCT_GET(counter, idNode, \
            map_counterHashByID(id), lr->counterMap, ptr);
}
