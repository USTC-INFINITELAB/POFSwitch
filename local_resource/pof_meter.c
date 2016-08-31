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

/* Malloc memory for meter information. Should be free by map_meterDelete(). */
static struct meterInfo *
map_meterCreate()
{
    struct meterInfo *meter;
    POF_MALLOC_SAFE_RETURN(meter, 1, NULL);
    return meter;
}

static void
map_meterDelete(struct meterInfo *meter, struct pof_local_resource *lr)
{
    hmap_nodeDelete(lr->meterMap, &meter->idNode);
    lr->meterNum --;
    FREE(meter);
}

static void
map_meterInsert(struct meterInfo *meter, struct pof_local_resource *lr)
{
    hmap_nodeInsert(lr->meterMap, &meter->idNode);
    lr->meterNum ++;
}

static hash_t
map_meterHashByID(uint32_t id)
{
    return hmap_hashForLinear(id);
}

/***********************************************************************
 * Add the meter.
 * Form:     uint32_t poflr_mod_meter_entry(uint32_t meter_id, uint32_t rate)
 * Input:    device id, meter id, rate
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will add the meter in the meter table.
 ***********************************************************************/
uint32_t 
poflr_add_meter_entry(uint32_t meter_id, uint32_t rate, struct pof_local_resource *lr)
{
    struct meterInfo *meter;
    
    /* Check meter_id. */
    if(meter_id >= lr->meterNumMax){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_INVALID_METER, g_recv_xid);
    }
    if(poflr_get_meter_with_ID(meter_id, lr)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_METER_EXISTS, g_recv_xid);
    }

    /* Create meter and insert to the local resource. */
    meter = map_meterCreate();
    POF_MALLOC_ERROR_HANDLE_RETURN_UPWARD(meter, g_upward_xid++);
    meter->id = meter_id;
    meter->rate = rate;
    meter->idNode.hash = map_meterHashByID(meter_id);
    map_meterInsert(meter, lr);

    POF_DEBUG_CPRINT_FL(1,GREEN,"Add meter SUC!");
    return POF_OK;
}

/***********************************************************************
 * Modify the meter.
 * Form:     uint32_t poflr_mod_meter_entry(uint32_t meter_id, uint32_t rate)
 * Input:    device id, meter id, rate
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will modify the meter in the meter table.
 ***********************************************************************/
uint32_t 
poflr_modify_meter_entry(uint32_t meter_id, uint32_t rate, struct pof_local_resource *lr)
{
    struct meterInfo *meter;

    /* Check meter_id. */
    if(meter_id >= lr->meterNumMax){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_INVALID_METER, g_recv_xid);
    }
    /* Get the meter. */
    if(!(meter = poflr_get_meter_with_ID(meter_id, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_UNKNOWN_METER, g_recv_xid);
    }
    /* Modify the rate. */
    meter->rate = rate;

    POF_DEBUG_CPRINT_FL(1,GREEN,"Modify meter SUC!");
    return POF_OK;
}

/***********************************************************************
 * Delete the meter.
 * Form:     uint32_t poflr_mod_meter_entry(uint32_t meter_id, uint32_t rate)
 * Input:    device id, meter id, rate
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will add the meter in the meter table.
 ***********************************************************************/
uint32_t 
poflr_delete_meter_entry(uint32_t meter_id, uint32_t rate, struct pof_local_resource *lr)
{
    struct meterInfo *meter;

    /* Check meter_id. */
    if(meter_id >= lr->meterNumMax){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_INVALID_METER, g_recv_xid);
    }
    /* Get the meter. */
    if(!(meter = poflr_get_meter_with_ID(meter_id, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_UNKNOWN_METER, g_recv_xid);
    }

    /* Delete the meter from local resource, and free the memory. */
    map_meterDelete(meter, lr);

    POF_DEBUG_CPRINT_FL(1,GREEN,"Delete meter SUC!");
    return POF_OK;
}

/* Initialize meter resource. */
uint32_t poflr_init_meter(struct pof_local_resource *lr){

    /* Initialize meter table. */
    lr->meterMap = hmap_create(lr->meterNumMax);
    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(lr->meterMap);

	return POF_OK;
}

/* Empty meter. */
uint32_t 
poflr_empty_meter(struct pof_local_resource *lr)
{
    struct meterInfo *meter, *next;
    if(!lr || !lr->meterMap){
        return POF_OK;
    }
    /* Delete all meters. */
    HMAP_NODES_IN_STRUCT_TRAVERSE(meter, next, idNode, lr->meterMap){
        map_meterDelete(meter, lr);
    }
	return POF_OK;
}

/* Get meter table. */
struct meterInfo *
poflr_get_meter_with_ID(uint32_t id, const struct pof_local_resource *lr)
{
    struct meterInfo *meter, *ptr;
    return HMAP_STRUCT_GET(meter, idNode, \
            map_meterHashByID(id), lr->meterMap, ptr);
}

static uint32_t
reply(const struct meterInfo * meter, const struct pof_local_resource *lr)
{
    struct pof_meter pofMeter = {0};

    pofMeter.command = POFMC_QUERY_RESULT;
#ifdef POF_MULTIPLE_SLOTS
    pofMeter.slotID = lr->slotID;
#endif // POF_MULTIPLE_SLOTS
    pofMeter.meter_id = meter->id;
    pofMeter.rate = meter->rate;
    pof_NtoH_transfer_meter(&pofMeter);

    if(POF_OK != pofec_reply_msg(POFT_METER_MOD, g_recv_xid, sizeof(pof_meter), (uint8_t *)&pofMeter)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
    }
    return POF_OK;
}

uint32_t
poflr_reply_meter(const struct pof_local_resource *lr, uint32_t meterID)
{
    struct meterInfo *meter;
    uint32_t ret;
    if((meter = poflr_get_meter_with_ID(meterID, lr)) == NULL){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_UNKNOWN_METER, g_recv_xid);
    }
    ret = reply(meter, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    return POF_OK;
}

uint32_t
poflr_reply_meter_all(const struct pof_local_resource *lr)
{
    uint32_t ret;
    struct meterInfo *meter, *next;
    HMAP_NODES_IN_STRUCT_TRAVERSE(meter, next, idNode, lr->meterMap){
        ret = reply(meter, lr);
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    }
    return POF_OK;
}
