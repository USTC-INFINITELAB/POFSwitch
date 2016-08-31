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
#include "../include/pof_conn.h"
#include "../include/pof_datapath.h"
#include "../include/pof_byte_transfer.h"
#include <string.h>

/* Move the packet data pointer buf_offset. 
 * buf_offset = buf_offset + offset 
 * The offset can be negative. */
static uint32_t
movePacketBufOffset(int16_t offset, struct pofdp_packet *dpp)
{
    /* Check offset. */
	if((offset > dpp->left_len) || \
            (offset < (0 - (int32_t)(POFDP_PACKET_PREBUF_LEN + dpp->offset)))){
		POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
	}
    dpp->offset     += offset;
    dpp->left_len   -= offset;
    dpp->buf_offset += offset;
    return POF_OK;
}

/* Move the instruction forward.
 * Caller needs to ensure that the forward offset is valid, less than todo number. */
static void
insJumpForward(uint32_t insNum, struct pofdp_packet *dpp)
{
    uint32_t i;
    /* Forward one by one. */
    for(i=0; i<insNum; i++){
#ifdef POF_SHT_VXLAN
        dpp->ins = (struct pof_instruction *)( \
                (uint8_t *)dpp->ins + dpp->ins->len );
#else // POF_SHT_VXLAN
        dpp->ins ++;
#endif // POF_SHT_VXLAN
    }
    dpp->ins_todo_num -= insNum;
    dpp->ins_done_num += insNum;
}

/* Move the instruction forward or backword. */
static uint32_t
insJump(uint8_t direction, uint32_t insNum, struct pofdp_packet *dpp)
{
	if(direction == POFD_FORWARD){
		if(insNum > dpp->ins_todo_num){
			POF_DEBUG_CPRINT_FL(1,RED,"The number of instruction to jump forward is more than the instructions left. " \
					"insNum = %u, insLeft=%u", insNum, dpp->ins_todo_num);
			POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_INSTRUCTION,POFBIC_JUM_TO_INVALID_INST,g_upward_xid++);
		}
        insJumpForward(insNum, dpp);
	}else{
		if(insNum > dpp->ins_done_num){
			POF_DEBUG_CPRINT_FL(1,RED,"The number of instruction to jump backward is more than the instructions reserved. " \
					"insNum = %u, insDone = %u", insNum, dpp->ins_done_num);
			POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_INSTRUCTION,POFBIC_JUM_TO_INVALID_INST,g_upward_xid++);
		}
#ifdef POF_SHT_VXLAN
        insNum = dpp->ins_done_num - insNum;
        dpp->ins_done_num = 0;
        dpp->ins_todo_num = dpp->insBlock->insNum;
        dpp->ins = (struct pof_instruction *)dpp->insBlock->insData;
        insJumpForward(insNum, dpp);
#else // POF_SHT_VXLAN
        dpp->ins            -= insNum;
        dpp->ins_done_num   -= insNum;
        dpp->ins_todo_num   += insNum;
#endif // POF_SHT_VXLAN
	}
    return POF_OK;
}

uint8_t *
pofdp_get_field_buf(const struct pof_match *pm, const struct pofdp_packet *dpp)
{
    uint8_t *buf = NULL;

	if(pm->field_id == POFDP_METADATA_FIELD_ID){
        /* From metadata. */
		if(pm->len + pm->offset > dpp->metadata_len * POF_BITNUM_IN_BYTE){
			POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_METADATA_LEN_ERROR, g_upward_xid);
            return NULL;
		}
		buf = (uint8_t *)dpp->metadata;
#ifdef POF_SHT_VXLAN
    }else if((pm->field_id & 0xF000) == POFDP_PARA_FIELD_ID){
        /* From parameter data. */
		if(pm->len + pm->offset > dpp->paraLen){
			POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PARA_LEN_ERROR, g_upward_xid);
            return NULL;
		}
		buf = (uint8_t *)dpp->para;
#endif // POF_SHT_VXLAN
	}else{
        /* From packet data. */
		if(pm->len + pm->offset > dpp->left_len * POF_BITNUM_IN_BYTE){
			POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid);
            return NULL;
		}
		buf = dpp->buf_offset;
    }

    return buf;
}

static void pofdp_find_key(uint8_t *packet, uint8_t *metadata, uint8_t **key_ptr, uint8_t match_field_num, const pof_match *match);

/* Update instruction pointer and number in dpp when one instruction
 * has been done. */
static void 
instruction_update(struct pofdp_packet *dpp)
{
	if(dpp->ins_todo_num == 1){
		dpp->packet_done = TRUE;
		return;
	}
    insJumpForward(1, dpp);
	return;
}

/***********************************************************************
 * Find the packet key before forwarding to the table.
 * Form:     uint32_t pofdp_find_key(uint8_t *packet, \
 *                                   uint8_t *metadata, \
 *                                   uint8_t **key_ptr, \
 *                                   uint8_t match_field_num, \
 *                                   const pof_match *match)
 * Input:    packet, metadata, match field number, match data
 * Output:   key
 * Return:   POF_OK or Error code
 * Discribe: This function get the key of packet, according to the match
 *           data. This function will be called when the POFIT_GOTO_TABLE
 *           instruction is processing. We lookup the packet into flow
 *           table using the packet key builded in this function.
 ***********************************************************************/
static void 
pofdp_find_key(uint8_t *packet, uint8_t *metadata, uint8_t **key_ptr, \
               uint8_t match_field_num, const pof_match *match)
{
    pof_match tmp_match;
    uint32_t  ret;
    uint8_t   i;

    for(i=0; i<match_field_num; i++){
        tmp_match = match[i];
        if(tmp_match.field_id != POFDP_METADATA_FIELD_ID){
            pofbf_copy_bit(packet, key_ptr[i], tmp_match.offset, tmp_match.len);
        }
        if(tmp_match.field_id == POFDP_METADATA_FIELD_ID){
            pofbf_copy_bit(metadata, key_ptr[i], tmp_match.offset, tmp_match.len);
        }
    }

    return;
}

static uint32_t pofdp_entry_nomatch(const struct pofdp_packet *dpp, const struct pof_local_resource *lr){
    uint32_t ret;
    uint8_t  table_ID;

#if (POF_NOMATCH == POF_NOMATCH_PACKET_IN)
    POF_DEBUG_CPRINT_FL(1,BLUE,"Send the packet which does NOT match " \
            "any entry in the table[%d][%d] to the controller", dpp->table_type, dpp->table_id);

    /* Current table ID. */
    ret = poflr_table_id_to_ID(dpp->table_type, dpp->table_id, &table_ID, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Send the no match packet upward to the Controller. */
    //POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->packetBuf, dpp->offset + dpp->left_len, "The no match packet is ");

    //ret = pofdp_send_packet_in_to_controller(dpp->offset + dpp->left_len, \
			POFR_NO_MATCH, table_ID, POF_FE_ID, dpp->ori_port_id, lr->slotID, dpp->buf);

    ret = pofdp_send_packet_in_to_controller(dpp->offset + dpp->left_len, \
			POFR_NO_MATCH, table_ID, POF_FE_ID, dpp->ori_port_id, lr->slotID, dpp->packetBuf);

    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

#elif (POF_NOMATCH == POF_NOMATCH_DROP)
    POF_DEBUG_CPRINT_FL(1,BLUE,"Drop the packet which does NOT match " \
            "any entry in the table[%d][%d].", dpp->table_type, dpp->table_id);
#endif

    return POF_OK;
}

static uint32_t execute_METER(POFDP_ARG)
{
    pof_instruction_meter *p = (pof_instruction_meter *)dpp->ins->instruction_data;
    struct meterInfo *meter;
    uint32_t meterID = 0, ret;

#ifdef POF_SHT_VXLAN
	ret = pofdp_get_32value(&meterID, p->id_type, &p->meter_id, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
#else // POF_SHT_VXLAN
    meterID = p->meter_id;
#endif // POF_SHT_VXLAN

    /* Check the meter id. */
    if(!(meter = poflr_get_meter_with_ID(meterID, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_UNKNOWN_METER, g_upward_xid++);
    }

	dpp->rate = meter->rate;
    POF_DEBUG_CPRINT_FL(1,GREEN,"instruction_meter has been DONE! meter_id = %u, rate = %u", \
			meter->id, meter->rate);

	instruction_update(dpp);
    return POF_OK;
}

#ifdef POF_SHT_VXLAN
static uint32_t execute_SET_PACKET_OFFSET(POFDP_ARG)
{
    struct pof_instruction_set_packet_offset *p = \
            (struct pof_instruction_set_packet_offset *)dpp->ins->instruction_data;
    int32_t offset = 0, offsetMove = 0;
    uint32_t ret;

    /* Get offset. */
	ret = pofdp_get_32value((uint32_t *)&offset, p->offset_type, &p->offset, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Convert to the move offset. */
    offsetMove = offset - dpp->offset;

    /* Move offset. */
    movePacketBufOffset(offsetMove, dpp);

    POF_DEBUG_CPRINT_FL(1,GREEN,"instruction_set_packet_offset has been DONE! offset = %d", \
			offset);
	instruction_update(dpp);
    return POF_OK;
}

static uint32_t execute_COMPARE(POFDP_ARG)
{
    struct pof_instruction_compare *p = \
            (struct pof_instruction_compare *)dpp->ins->instruction_data;
    uint32_t value1 = 0, value2 = 0, ret;
    uint8_t resNum = 0, compRes = 0, *resAddr = NULL, mask = 0, mov_b = 0;

    resNum = p->comp_result_field_num;
    resAddr = &(dpp->metadata->compRes);

	/* Get value1 and value2. */
	ret = pofdp_get_32value(&value1, POFVT_FIELD, &p->operand1, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	ret = pofdp_get_32value(&value2, p->operand2_type, &p->operand2, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Compare. */
    if(value1 == value2){
        compRes = POFCR_EQUAL;
    }else if(value1 > value2){
        compRes = POFCR_BIGER;
    }else{
        compRes = POFCR_SMALLER;
    }

    /* Write the compare result.
     * There are four compRes fields with 2bit in the metadata.
     * Identified by resNum. 
     * metadata.compRes(8bit):  00 00 00 00 
     * resNum:                   0  1  2  3 */
    mov_b =  (POF_BITNUM_IN_BYTE - (resNum + 1) * POF_COMP_RES_FIELD_BITNUM);
    mask = 3 << mov_b;
    compRes = compRes << mov_b;
    *resAddr = ( *resAddr & ( ~ mask ) ) |    \
               ( compRes  & (   mask ) );

    POF_DEBUG_CPRINT_FL(1,GREEN,\
            "instruction_compare has been DONE! value1 = %u, value2 = %u, res = %u, resNum = %u", \
			value1, value2, compRes, resNum);
	instruction_update(dpp);
    return POF_OK;
}

static uint32_t execute_BRANCH(POFDP_ARG)
{
    struct pof_instruction_branch *p = \
            (struct pof_instruction_branch *)dpp->ins->instruction_data;
    uint32_t ret = POF_OK, value1 = 0, value2 = 0;

	/* Get value1 and value2. */
	ret = pofdp_get_32value(&value1, POFVT_FIELD, &p->operand1, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	ret = pofdp_get_32value(&value2, p->operand2_type, &p->operand2, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    if(value1 != value2){
        ret = insJump(POFD_FORWARD, p->skip_instruction_num, dpp);
	    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

        POF_DEBUG_CPRINT_FL(1,GREEN,\
                "instruction_branch has been DONE! value1 = %u, value2 = %u, skipInsNum = %u", \
                value1, value2, p->skip_instruction_num);
    }else{
        POF_DEBUG_CPRINT_FL(1,GREEN,\
                "instruction_branch has been DONE! value1 = %u, value2 = %u, NO Branch.", \
                value1, value2, p->skip_instruction_num);
	    instruction_update(dpp);
    }

    return POF_OK;
}

static uint32_t execute_JMP(POFDP_ARG)
{
    struct pof_instruction_jmp *p = \
            (struct pof_instruction_jmp *)dpp->ins->instruction_data;
    uint32_t ret;

    ret = insJump(p->direction, p->instruction_num, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,GREEN,\
            "instruction_JMP has been DONE! direction = %u, jumpInsNum = %u", \
            p->direction, p->instruction_num);
	instruction_update(dpp);
    return POF_OK;
}
#endif // POF_SHT_VXLAN

static uint32_t execute_WRITE_METADATA(POFDP_ARG)
{
    pof_instruction_write_metadata *p = \
			(pof_instruction_write_metadata *)dpp->ins->instruction_data;
#ifdef POF_SD2N_AFTER1015
#else // POF_SD2N_AFTER1015
    uint32_t value = p->value;
#endif // POF_SD2N_AFTER1015
    uint32_t ret;
	pof_match pm = {POFDP_METADATA_FIELD_ID, p->metadata_offset, p->len};

#ifdef POF_SD2N_AFTER1015
    pofbf_cover_bit((uint8_t *)dpp->metadata, (uint8_t *)p->value, p->metadata_offset, p->len);
#else // POF_SD2N_AFTER1015
	ret = pofdp_write_32value_to_field(value, &pm, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
#endif // POF_SD2N_AFTER1015

    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->metadata, POF_BITNUM_TO_BYTENUM_CEIL(p->metadata_offset+p->len),"instruction_write_metadata has been DONE! The metadata = ");

	instruction_update(dpp);
    return POF_OK;
}

static uint32_t execute_WRITE_METADATA_FROM_PACKET(POFDP_ARG)
{
    pof_instruction_write_metadata_from_packet *p = \
                (pof_instruction_write_metadata_from_packet *)dpp->ins->instruction_data;
	struct pofdp_metadata *metadata = dpp->metadata;
    uint8_t  value[POFDP_METADATA_MAX_LEN];

    memset(value, 0, sizeof value);

    if(dpp->left_len * POF_BITNUM_IN_BYTE < p->len + p->packet_offset){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    pofbf_copy_bit(dpp->buf_offset, value, p->packet_offset, p->len);
    pofbf_cover_bit((uint8_t *)metadata, value, p->metadata_offset, p->len);

    POF_DEBUG_CPRINT_FL_0X(1,GREEN,metadata, POF_BITNUM_TO_BYTENUM_CEIL(p->metadata_offset+p->len),"instruction_write_metadata_from_packet has been DONE! The metadata = ");

	instruction_update(dpp);
    return POF_OK;
}

static uint32_t execute_GOTO_DIRECT_TABLE(POFDP_ARG)
{
    pof_instruction_goto_direct_table *p = \
			(pof_instruction_goto_direct_table *)dpp->ins->instruction_data;
    struct entryInfo *entry;
    struct tableInfo *table;
    uint8_t *table_type = &dpp->table_type;
    uint8_t *table_id = &dpp->table_id;
    uint32_t i, ret, entry_index;

    p = (pof_instruction_goto_direct_table *)dpp->ins->instruction_data;

    /* The packet forward to the next table. */
	ret = movePacketBufOffset((int16_t)p->packet_offset, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Find the table type and id of the next direct table. */
    ret = poflr_table_ID_to_id(p->next_table_id, table_type, table_id, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

#ifdef POF_SD2N
	ret = pofdp_get_32value(&entry_index, p->index_type, &p->table_entry_index, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
#else // POF_SD2N
	entry_index = p->table_entry_index;
#endif // POF_SD2N

    table = poflr_get_table_with_ID(p->next_table_id, lr);
    if(!(table)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBIC_BAD_TABLE_ID, g_upward_xid++);
    }
    POF_DEBUG_CPRINT_FL(1,BLUE,"Go to DT table[%d][%d][%d]!", *table_type, *table_id, entry_index);

    /* Check the table type. */
    if(*table_type != POF_LINEAR_TABLE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBIC_BAD_TABLE_TYPE, g_upward_xid++);
    }

    /* Check whether the index have already existed. */
    if(!(dpp->flow_entry = poflr_entry_lookup_Linear(entry_index, table))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBIC_ENTRY_UNEXIST, g_upward_xid++);
    }

    /* Increase the counter value. */
#ifdef POF_SD2N
    ret = poflr_counter_increace(dpp->flow_entry->counter_id, POF_PACKET_REL_LEN_GET(dpp), lr);
#else // POF_SD2N
    ret = poflr_counter_increace(dpp->flow_entry->counter_id, lr);
#endif // POF_SD2N
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Update the instruction number and the instruction data corresponding to the
     * matched flow entry in the current flow table. */
#ifdef POF_SHT_VXLAN
    uint16_t blockID = dpp->flow_entry->insBlockID;
    struct insBlockInfo *insBlock = NULL;

    /* Get the insBlock. */
    if(!(insBlock = poflr_get_insBlock_with_ID(blockID, lr))){
        POF_ERROR_HANDLE_NO_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_BAD_INS_BLOCK_ID);
        dpp->packet_done = TRUE;
        return ret;
    }
    POF_DEBUG_CPRINT_FL(1,BLUE,"Execute the insBlock [%u]", blockID);

    dpp->insBlock = insBlock;
    dpp->ins = (struct pof_instruction *)insBlock->insData;
    dpp->ins_todo_num = insBlock->insNum;
    dpp->paraLen = dpp->flow_entry->paraLen;
    dpp->para = dpp->flow_entry->para;
#else // POF_SHT_VXLAN
    dpp->ins = dpp->flow_entry->instruction;
    dpp->ins_todo_num = dpp->flow_entry->instruction_num;
#endif // POF_SHT_VXLAN
	dpp->ins_done_num = 0;

    POF_DEBUG_CPRINT_FL(1,GREEN,"Match entry! ");

    return POF_OK;
}

static uint32_t execute_GOTO_TABLE(POFDP_ARG)
{
    struct pof_instruction_goto_table *p = \
				(pof_instruction_goto_table *)dpp->ins->instruction_data;
    struct tableInfo *table;
    uint32_t i, j, ret = POF_OK;
    uint8_t *table_type = &dpp->table_type;
    uint8_t *table_id = &dpp->table_id;

    p = (pof_instruction_goto_table *)dpp->ins->instruction_data;

    /* The packet forward to the next table. */
	ret = movePacketBufOffset((int16_t)p->packet_offset, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* The table type and id. */
    ret = poflr_table_ID_to_id(p->next_table_id, table_type, table_id, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,BLUE,"Go to table[%d][%d]!", *table_type, *table_id);

    if(!(table = poflr_get_table_with_ID(p->next_table_id, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBIC_BAD_TABLE_ID, g_upward_xid++);
    }

    if(!(dpp->flow_entry = poflr_entry_lookup(dpp->buf_offset, (uint8_t *)dpp->metadata, table))){
        /* No match. */
        POF_DEBUG_CPRINT_FL(1,RED,"Cannot find the right entry in table[%d][%d]!",*table_type,*table_id);

        ret = pofdp_entry_nomatch(dpp, lr);
        POF_CHECK_RETVALUE_NO_RETURN_NO_UPWARD(ret);

        dpp->packet_done = TRUE;
    }else{
        POF_DEBUG_CPRINT_FL(1,GREEN,"Match entry[%u]", dpp->flow_entry->index);
        /* Match. Increace the counter value. */
#ifdef POF_SD2N
        ret = poflr_counter_increace(dpp->flow_entry->counter_id, POF_PACKET_REL_LEN_GET(dpp), lr);
#else // POF_SD2N
        ret = poflr_counter_increace(dpp->flow_entry->counter_id, lr);
#endif // POF_SD2N
        POF_CHECK_RETVALUE_NO_RETURN_NO_UPWARD(ret);

        /* Update the instruction number and the instruction data corresponding to the
         * matched flow entry in the current flow table. */
#ifdef POF_SHT_VXLAN
        uint16_t blockID = dpp->flow_entry->insBlockID;
        struct insBlockInfo *insBlock = NULL;

        /* Get the insBlock. */
        if(!(insBlock = poflr_get_insBlock_with_ID(blockID, lr))){
            POF_ERROR_HANDLE_NO_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_BAD_INS_BLOCK_ID);
            dpp->packet_done = TRUE;
            return ret;
        }
        POF_DEBUG_CPRINT_FL(1,BLUE,"Execute the insBlock [%u]", blockID);
        dpp->insBlock = insBlock;
        dpp->ins = (struct pof_instruction *)insBlock->insData;
        dpp->ins_todo_num = insBlock->insNum;
        dpp->paraLen = dpp->flow_entry->paraLen;
        dpp->para = dpp->flow_entry->para;
#else // POF_SHT_VXLAN
        dpp->ins = dpp->flow_entry->instruction;
        dpp->ins_todo_num = dpp->flow_entry->instruction_num;
#endif // POF_SHT_VXLAN
		dpp->ins_done_num = 0;
    }

    return ret;
}

static uint32_t execute_APPLY_ACTIONS(POFDP_ARG)
{
    pof_instruction_apply_actions *p = \
			(pof_instruction_apply_actions *)dpp->ins->instruction_data;
    uint32_t    ret;

    dpp->act = (pof_action *)p->action;
    dpp->act_num = p->action_num;

    ret = pofdp_action_execute(dpp, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	instruction_update(dpp);
    return POF_OK;
}

/* Caller should make sure that len is less than 32, and the sum of offset and
 * len is less than the buf size. */
static void
write_32value_to_buf(uint8_t *buf, uint32_t value, \
						   uint16_t offset, uint16_t len)
{
	/* Transform the value format. */
	value = POF_MOVE_BIT_LEFT(value, 32 - len);
	POF_NTOHL_FUNC(value);

    pofbf_cover_bit(buf, (uint8_t *)&value, offset, len);
	return;
}

/* write value to packet/metadta field. */
uint32_t
pofdp_write_32value_to_field(uint32_t value, const struct pof_match *pm, \
							 struct pofdp_packet *dpp)
{
	uint8_t *dst;

	if(pm->len > 32){
		POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_MATCH, POFBMC_BAD_LEN, g_upward_xid++);
	}

	/* Copy data from packet/metadata to value. */
	if(pm->field_id != POFDP_METADATA_FIELD_ID){
		if(pm->len + pm->offset > dpp->left_len * POF_BITNUM_IN_BYTE){
			POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
		}
		dst = dpp->buf_offset;
	}else{
		if(pm->len + pm->offset > dpp->metadata_len * POF_BITNUM_IN_BYTE){
			POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_METADATA_LEN_ERROR, g_upward_xid++);
		}
		dst = (uint8_t *)dpp->metadata;
	}

	write_32value_to_buf(dst, value, pm->offset, pm->len);

	return POF_OK;
}

/* Caller should make sure that len is less than 32, and the sum of offset and
 * len is less than the buf size. */
static void
pofdp_get_32value_from_buf(uint8_t *buf, uint32_t *value, \
						   uint16_t offset, uint16_t len)
{
	pofbf_copy_bit(buf, (uint8_t *)value, offset, len);

	/* Transform the value format. */
	POF_NTOHL_FUNC(*value);
	*value = POF_MOVE_BIT_RIGHT(*value, 32 - len);

	return;
}

static void
pofdp_get_16value_from_buf(uint8_t *buf, uint16_t *value, \
						   uint16_t offset, uint16_t len)
{
	pofbf_copy_bit(buf, (uint8_t *)value, offset, len);

	/* Transform the value format. */
	POF_NTOHS_FUNC(*value);
	*value = POF_MOVE_BIT_RIGHT(*value, 16 - len);

	return;
}

/* get value from packet/metadta field. */
uint32_t
get_32value_from_field(uint32_t *value, const struct pof_match *pm, \
							 const struct pofdp_packet *dpp)
{
	uint8_t *src;
	if(pm->len > 32){
		POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_MATCH, POFBMC_BAD_LEN, g_upward_xid);
	}

    /* Get the buf according to the match field id. */
    if(!(src = pofdp_get_field_buf(pm, dpp))){
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(POF_ERROR);
    }
	/* Copy data to value. */
	pofdp_get_32value_from_buf(src, value, pm->offset, pm->len);

	return POF_OK;
}

uint32_t
get_16value_from_field(uint16_t *value, const struct pof_match *pm, \
							 const struct pofdp_packet *dpp)
{
	uint8_t *src;
	if(pm->len > 16){
		POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_MATCH, POFBMC_BAD_LEN, g_upward_xid);
	}

    /* Get the buf according to the match field id. */
    if(!(src = pofdp_get_field_buf(pm, dpp))){
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(POF_ERROR);
    }
	/* Copy data to value. */
	pofdp_get_16value_from_buf(src, value, pm->offset, pm->len);

	return POF_OK;
}

#ifdef POF_SHT_VXLAN
/* Get byte value from match struct. */
/* If len_b is not zero, field.len should equals to len_b. */
uint32_t
pofdp_get_value_byte_match(uint8_t *value,                  \
                            uint16_t len_b,                 \
                            struct pof_match field,         \
                            const struct pofdp_packet *dpp)
{
    uint8_t *src = NULL;
    
    /* Check the length. */
    if((len_b != 0) && (len_b != field.len)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_FIELD_LEN_ERROR, g_upward_xid ++);
    }

    /* Get the buf according to the match field id. */
    if(!(src = pofdp_get_field_buf(&field, dpp))){
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(POF_ERROR);
    }
	pofbf_copy_bit(src, value, field.offset, field.len);

    return POF_OK;
}

uint32_t
pofdp_get_value_byte(uint8_t *value, uint16_t len_b, uint8_t type, void *u_, const struct pofdp_packet *dpp)
{
	union {
		uint8_t value[1];
		struct pof_match field;
	} *u = u_;
	uint32_t ret;

	if(type == POFVT_IMMEDIATE_NUM){
        memcpy(value, u->value, POF_BITNUM_TO_BYTENUM_CEIL(len_b));
		return POF_OK;
	}

    /* Get byte value from field. */
    ret = pofdp_get_value_byte_match(value, len_b, u->field, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	return POF_OK;
}
#endif // POF_SHT_VXLAN

uint32_t
pofdp_get_32value(uint32_t *value, uint8_t type, void *u_, const struct pofdp_packet *dpp)
{
	union {
		uint32_t value;
		struct pof_match field;
	} *u = u_;
	uint32_t ret;

	if(type == POFVT_IMMEDIATE_NUM){
		*value = u->value;
		return POF_OK;
	}

	ret = get_32value_from_field(value, &u->field, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	return POF_OK;
}

uint32_t
pofdp_get_16value(uint16_t *value, uint8_t type, void *u_, const struct pofdp_packet *dpp)
{
	union {
		uint16_t value;
		struct pof_match field;
	} *u = u_;
	uint32_t ret;

	if(type == POFVT_IMMEDIATE_NUM){
		*value = u->value;
		return POF_OK;
	}

	ret = get_16value_from_field(value, &u->field, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	return POF_OK;
}

#ifdef POF_SD2N
static uint32_t
execute_CONDITIONAL_JMP(POFDP_ARG)
{
	struct pof_instruction_conditional_jump *p = \
			(struct pof_instruction_conditional_jump *)dpp->ins->instruction_data;
	uint32_t value1, value2, offset, ret;
	uint8_t direction;

	/* Get value1 and value2. */
	ret = pofdp_get_32value(&value1, POFVT_FIELD, &p->compare_field1, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	ret = pofdp_get_32value(&value2, p->field2_type, &p->compare_field2, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	/* Compare value1 and value2. */
	if(value1 < value2){
		ret = pofdp_get_32value(&offset, p->offset1_type, &p->offset1, dpp);
		direction = p->offset1_direction;
	}else if(value1 == value2){
		ret = pofdp_get_32value(&offset, p->offset2_type, &p->offset2, dpp);
		direction = p->offset2_direction;
	}else if(value1 > value2){
		ret = pofdp_get_32value(&offset, p->offset3_type, &p->offset3, dpp);
		direction = p->offset3_direction;
	}
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	/* Move the instruction pointer to the offset. */
    ret = insJump(direction, offset, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	POF_DEBUG_CPRINT_FL(1,GREEN,"instruction_conditional_jump has been DONE!");
	return POF_OK;
}

/* Execute different type of CALCULATE_FIELD instruction. */
static uint32_t
calculate_ADD(uint32_t value, uint32_t operand)
{
	return value + operand;
}

static uint32_t
calculate_SUBTRACT(uint32_t value, uint32_t operand)
{
	return value - operand;
}

static uint32_t
calculate_LEFT_SHIFT(uint32_t value, uint32_t operand)
{
	return POF_MOVE_BIT_LEFT(value, operand);
}

static uint32_t
calculate_RIGHT_SHIFT(uint32_t value, uint32_t operand)
{
	return POF_MOVE_BIT_RIGHT(value, operand);
}

static uint32_t
calculate_BITWISE_ADD(uint32_t value, uint32_t operand)
{
	return value & operand;
}

static uint32_t
calculate_BITWISE_OR(uint32_t value, uint32_t operand)
{
	return value | operand;
}

static uint32_t
calculate_BITWISE_XOR(uint32_t value, uint32_t operand)
{
	return value ^ operand;
}

static uint32_t
calculate_BITWISE_NOR(uint32_t value, uint32_t operand)
{
	return ~ calculate_BITWISE_OR(value, operand);
}

static uint32_t
execute_CALCULATE_FIELD(POFDP_ARG)
{
	struct pof_instruction_calc_field *p = \
			(struct pof_instruction_calc_field *)dpp->ins->instruction_data;
	uint32_t value1, operand, result, ret;

	ret = pofdp_get_32value(&value1, POFVT_FIELD, &p->dst_field, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	ret = pofdp_get_32value(&operand, p->src_type, &p->src_operand, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	switch(p->calc_type){
#define CALCULATION(NAME,VALUE) \
		case POFCT_##NAME:											\
			result=calculate_##NAME(value1,operand);				\
			POF_DEBUG_CPRINT_FL(1,GREEN,"calculate_type = "#NAME);  \
			break;

		CALCULATIONS
#undef CALCULATION
		default:
			POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_INSTRUCTION, POFBIC_UNKNOWN_INST, g_upward_xid++);
			break;
	}

	ret = pofdp_write_32value_to_field(result, &p->dst_field, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	POF_DEBUG_CPRINT_FL(1,GREEN,"instruction_calculate_field has been done! " \
			"cal_type = %u, value = %u (0x%x), operand = %u (0x%x), result = %u (0x%x)", \
			p->calc_type, value1, value1, operand, operand, result, result);
	instruction_update(dpp);
	return POF_OK;
}

static uint32_t
execute_MOVE_PACKET_OFFSET(POFDP_ARG)
{
	struct pof_instruction_mov_packet_offset *p = \
			(struct pof_instruction_mov_packet_offset *)dpp->ins->instruction_data;
	uint32_t value, ret;

	ret = pofdp_get_32value(&value, p->value_type, &p->movement, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	if(p->direction == POFD_FORWARD){
		ret = movePacketBufOffset((int16_t)value, dpp);
	}else{
		ret = movePacketBufOffset((0 - (int16_t)value), dpp);
	}
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	POF_DEBUG_CPRINT_FL(1,GREEN,"instruction_move_packet_offset has been done! " \
			"direction = %u, offset = %u", p->direction, value);
    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf_offset,dpp->left_len,"The new packet = ");

	instruction_update(dpp);
	return POF_OK;
}
#endif // POF_SD2N

static uint32_t 
execute_WRITE_ACTIONS(POFDP_ARG)
{
	// TODO
    POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_INSTRUCTION, POFBIC_UNSUP_INST, g_upward_xid++);
}

static uint32_t 
execute_CLEAR_ACTIONS(POFDP_ARG)
{
	// TODO
    POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_INSTRUCTION, POFBIC_UNSUP_INST, g_upward_xid++);
}

static uint32_t 
execute_EXPERIMENTER(POFDP_ARG)
{
	// TODO
    POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_INSTRUCTION, POFBIC_UNSUP_INST, g_upward_xid++);
}

/***********************************************************************
 * Execute instructions
 * Form:     uint32_t pofdp_instruction_execute(POFDP_ARG)
 * Input:    dpp, dp
 * Return:   POF_OK or Error code
 * Discribe: This function executes the actions. 
 ***********************************************************************/
uint32_t pofdp_instruction_execute(POFDP_ARG)
{
	uint32_t ret = POF_OK;
    /* Forward the packet via executing the instructions until packet_over is TRUE or all
     * instructions have been done. */
    while(dpp->packet_done == FALSE){
        /* Execute the instructions. */
        switch(dpp->ins->type){
#define INSTRUCTION(NAME,VALUE) case POFIT_##NAME: ret = execute_##NAME(dpp,lr); break;
			INSTRUCTIONS
#undef INSTRUCTION
            default:
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_INSTRUCTION, POFBIC_UNKNOWN_INST, g_upward_xid++);
				break;
        }
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    }
	return POF_OK;
}
