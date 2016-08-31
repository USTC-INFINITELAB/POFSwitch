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
#include "../include/pof_byte_transfer.h"
#include "../include/pof_log_print.h"

static uint32_t instruction_NtoH(void *ptr);
static uint32_t instruction_HtoN(void *ptr);

static uint32_t match(void *ptr){
    pof_match *p = (pof_match *)ptr;

    POF_NTOHS_FUNC(p->field_id);
    POF_NTOHS_FUNC(p->offset);
    POF_NTOHS_FUNC(p->len);

    return POF_OK;
}

static uint32_t match_x(void *ptr){
    pof_match_x *p = (pof_match_x *)ptr;

    POF_NTOHS_FUNC(p->field_id);
    POF_NTOHS_FUNC(p->offset);
    POF_NTOHS_FUNC(p->len);

    return POF_OK;
}

static uint32_t
value_byte(uint8_t type, void *ptr)
{
	union {
		uint8_t value;
		struct pof_match field;
	} *p = ptr;

	if(type != POFVT_IMMEDIATE_NUM){
		match(&p->field);
	}
}

static uint32_t
value_16(uint8_t type, void *ptr)
{
	union {
		uint16_t value;
		struct pof_match field;
	} *p = ptr;

	if(type == POFVT_IMMEDIATE_NUM){
		POF_NTOHS_FUNC(p->value);
	}else{
		match(&p->field);
	}
}

static uint32_t
value(uint8_t type, void *ptr)
{
	union {
		uint32_t value;
		struct pof_match field;
	} *p = ptr;

	if(type == POFVT_IMMEDIATE_NUM){
		POF_NTOHL_FUNC(p->value);
	}else{
		match(&p->field);
	}
}

uint32_t pof_NtoH_transfer_error(void *ptr){
    pof_error *p = (pof_error *)ptr;

#ifdef POF_MULTIPLE_SLOTS
    POF_NTOHS_FUNC(p->slotID);
#endif // POF_MULTIPLE_SLOTS
    POF_NTOHS_FUNC(p->type);
    POF_NTOHS_FUNC(p->code);
    POF_NTOHL_FUNC(p->device_id);

    return POF_OK;
}

uint32_t pof_NtoH_transfer_packet_in(void *ptr){
    pof_packet_in *p = (pof_packet_in *)ptr;

    POF_NTOHL_FUNC(p->buffer_id);
    POF_NTOHS_FUNC(p->total_len);
    POF_NTOH64_FUNC(p->cookie);
    POF_NTOHL_FUNC(p->device_id);
//#ifdef POF_MULTIPLE_SLOTS
    POF_NTOHS_FUNC(p->slotID);
    POF_NTOHS_FUNC(p->port_id);
//#endif // POF_MULTIPLE_SLOTS

    return POF_OK;
}

uint32_t pof_NtoH_transfer_header(void *ptr){
    pof_header *head_p = (pof_header *)ptr;

    POF_NTOHS_FUNC(head_p->length);
    POF_NTOHL_FUNC(head_p->xid);

    return POF_OK;
}

uint32_t pof_HtoN_transfer_header(void *ptr){
    pof_header *head_p = (pof_header *)ptr;

    POF_HTONS_FUNC(head_p->length);
    POF_HTONL_FUNC(head_p->xid);

    return POF_OK;
}

uint32_t pof_NtoH_transfer_port(void *ptr){
    pof_port *p = (pof_port *)ptr;

#ifdef POF_MULTIPLE_SLOTS
    POF_NTOHS_FUNC(p->slotID);
    POF_NTOHS_FUNC(p->port_id);
#else // POF_MULTIPLE_SLOTS
    POF_NTOHL_FUNC(p->port_id);
#endif // POF_MULTIPLE_SLOTS
    POF_NTOHL_FUNC(p->device_id);
    POF_NTOHL_FUNC(p->config);
    POF_NTOHL_FUNC(p->state);
    POF_NTOHL_FUNC(p->curr);
    POF_NTOHL_FUNC(p->advertised);
    POF_NTOHL_FUNC(p->supported);
    POF_NTOHL_FUNC(p->peer);
    POF_NTOHL_FUNC(p->curr_speed);
    POF_NTOHL_FUNC(p->max_speed);

    return POF_OK;
}

uint32_t pof_NtoH_transfer_flow_table(void *ptr){
    pof_flow_table *p = (pof_flow_table *)ptr;
    uint32_t i;

    POF_NTOHL_FUNC(p->size);
    POF_NTOHS_FUNC(p->key_len);
    POF_NTOHS_FUNC(p->slotID);
    for(i=0; i<POF_MAX_MATCH_FIELD_NUM; i++)
        match((pof_match *)p->match + i);

    return POF_OK;
}

uint32_t pof_NtoH_transfer_meter(void *ptr){
    pof_meter *p = (pof_meter *)ptr;

#ifdef POF_MULTIPLE_SLOTS
    POF_NTOHS_FUNC(p->slotID);
    POF_NTOHL_FUNC(p->rate);
    POF_NTOHL_FUNC(p->meter_id);
#else // POF_MULTIPLE_SLOTS
    POF_NTOHS_FUNC(p->rate);
    POF_NTOHL_FUNC(p->meter_id);
#endif 

    return POF_OK;
}

uint32_t pof_NtoH_transfer_counter(void *ptr){
    pof_counter *p = (pof_counter *)ptr;

    POF_NTOHL_FUNC(p->counter_id);
#ifdef POF_MULTIPLE_SLOTS
    POF_NTOHS_FUNC(p->slotID);
#endif // POF_MULTIPLE_SLOTS
    POF_NTOH64_FUNC(p->value);
#ifdef POF_SD2N
    POF_NTOH64_FUNC(p->byte_value);
#endif // POF_SD2N

    return POF_OK;
}

uint32_t pof_HtoN_transfer_switch_features(void *ptr){
    pof_switch_features *p = (pof_switch_features *)ptr;

    POF_HTONL_FUNC(p->dev_id);
#ifdef POF_MULTIPLE_SLOTS
    POF_HTONS_FUNC(p->slotID);
#endif // POF_MULTIPLE_SLOTS
    POF_HTONS_FUNC(p->port_num);
    POF_HTONS_FUNC(p->table_num);
    POF_HTONL_FUNC(p->capabilities);

    return POF_OK;
}

uint32_t pof_HtoN_transfer_table_resource_desc(void *ptr){
    pof_table_resource_desc *p = (pof_table_resource_desc *)ptr;

    POF_HTONL_FUNC(p->device_id);
    POF_HTONS_FUNC(p->key_len);
    POF_HTONL_FUNC(p->total_size);

    return POF_OK;
}

uint32_t pof_HtoN_transfer_flow_table_resource(void *ptr){
    pof_flow_table_resource *p = (pof_flow_table_resource *)ptr;
    int i;

#ifdef POF_MULTIPLE_SLOTS
    POF_HTONS_FUNC(p->slotID);
#endif // POF_MULTIPLE_SLOTS
    POF_HTONL_FUNC(p->counter_num);
    POF_HTONL_FUNC(p->meter_num);
    POF_HTONL_FUNC(p->group_num);

    for(i=0;i<POF_MAX_TABLE_TYPE;i++)
        pof_HtoN_transfer_table_resource_desc((pof_table_resource_desc *)p->tbl_rsc_desc+i);

    return POF_OK;
}

#ifdef POF_SHT_VXLAN
uint32_t pof_HtoN_transfer_slot_status(void *ptr){
    struct pof_slot_status *p = (struct pof_slot_status *)ptr;

    POF_HTONS_FUNC(p->slotID);

    return POF_OK;
}

uint32_t pof_HtoN_transfer_slot_config(void *ptr){
    struct pof_slot_config *p = (struct pof_slot_config *)ptr;

    POF_HTONS_FUNC(p->slotID);

    return POF_OK;
}

uint32_t pof_NtoH_transfer_insBlock(void *ptr){
    struct pof_instruction_block *p = (struct pof_instruction_block *)ptr;
    struct pof_instruction *ins = p->instruction;
    uint8_t insNum = p->instruction_num, i;

    POF_HTONS_FUNC(p->slotID);
    POF_HTONS_FUNC(p->instruction_block_id);

    for(i=0; i<insNum; i++){
        instruction_NtoH(ins);
        ins = (struct pof_instruction *)( \
                (uint8_t *)ins + ins->len );
    }

    return POF_OK;
}

static uint32_t
action_CALCULATE_FIELD(void *ptr)
{
	struct pof_action_calc_field *p = \
			(struct pof_action_calc_field *)ptr;

	POF_NTOHS_FUNC(p->calc_type);
	match(&p->dst_field);

	value(p->src_type, &p->src_operand);
	
	return POF_OK;
}

static uint32_t
instruction_SET_PACKET_OFFSET(void *ptr)
{
    struct pof_instruction_set_packet_offset *p = \
            (struct pof_instruction_set_packet_offset *)ptr;
    value(p->offset_type, &p->offset);
    return POF_OK;
}

static uint32_t
instruction_COMPARE(void *ptr)
{
    struct pof_instruction_compare *p = \
            (struct pof_instruction_compare *)ptr;
    match(&p->operand1);
    value(p->operand2_type, &p->operand2);
    return POF_OK;
}

static uint32_t
instruction_BRANCH(void *ptr)
{
    struct pof_instruction_branch *p = \
            (struct pof_instruction_branch *)ptr;

	POF_NTOHL_FUNC(p->skip_instruction_num);
    match(&p->operand1);
    value(p->operand2_type, &p->operand2);
    return POF_OK;
}

static uint32_t
instruction_JMP(void *ptr)
{
    struct pof_instruction_jmp *p = \
            (struct pof_instruction_jmp *)ptr;
    POF_NTOHL_FUNC(p->instruction_num);
    return POF_OK;
}

static uint32_t
action_ENCAP_VXLAN_HEADER(void *ptr)
{
    struct pof_action_encap_vxlan_header *p = \
        (struct pof_action_encap_vxlan_header *)ptr;
    value_byte(p->vni_type, &p->vni);
    return POF_OK;
}

static uint32_t
action_ENCAP_UDP_HEADER(void *ptr)
{
    struct pof_action_encap_udp_header *p = \
        (struct pof_action_encap_udp_header *)ptr;
    value_16(p->sport_type, &p->sport);
    value_16(p->dport_type, &p->dport);
    return POF_OK;
}

static uint32_t
action_ENCAP_IP_HEADER(void *ptr)
{
    struct pof_action_encap_ip_header *p = \
        (struct pof_action_encap_ip_header *)ptr;
    value_byte(p->sip_type, &p->sip);
    value_byte(p->dip_type, &p->dip);
    value_byte(p->tos_type, &p->tos);
    return POF_OK;
}

static uint32_t
action_ENCAP_MAC_HEADER(void *ptr)
{
    struct pof_action_encap_mac_header *p = \
        (struct pof_action_encap_mac_header *)ptr;
    value_byte(p->smac_type, &p->smac);
    value_byte(p->dmac_type, &p->dmac);
    POF_NTOHS_FUNC(p->ethType);
    return POF_OK;
}
#endif // POF_SHT_VXLAN

static uint32_t port(void *ptr){
    pof_port *p = (pof_port *)ptr;

#ifdef POF_MULTIPLE_SLOTS
    POF_HTONS_FUNC(p->slotID);
    POF_HTONS_FUNC(p->port_id);
#else // POF_MULTIPLE_SLOTS
    POF_HTONL_FUNC(p->port_id);
#endif // POF_MULTIPLE_SLOTS
    POF_HTONL_FUNC(p->device_id);
    POF_HTONL_FUNC(p->config);
    POF_HTONL_FUNC(p->state);
    POF_HTONL_FUNC(p->curr);
    POF_HTONL_FUNC(p->advertised);
    POF_HTONL_FUNC(p->supported);
    POF_HTONL_FUNC(p->peer);
    POF_HTONL_FUNC(p->curr_speed);
    POF_HTONL_FUNC(p->max_speed);

    return POF_OK;
}

uint32_t pof_HtoN_transfer_port_status(void *ptr){
    pof_port_status *p = (pof_port_status *)ptr;

    port((pof_port *)&(p->desc));

    return POF_OK;
}

uint32_t pof_HtoN_transfer_switch_config(void * ptr){
    pof_switch_config *p = (pof_switch_config *)ptr;

#ifdef POF_MULTIPLE_SLOTS
    POF_HTONL_FUNC(p->dev_id);
#endif // POF_MULTIPLE_SLOTS
    POF_HTONS_FUNC(p->flags);
    POF_HTONS_FUNC(p->miss_send_len);

    return POF_OK;
}

uint32_t pof_HtoN_transfer_queryall_request(void * ptr){
    struct pof_queryall_request *p = (struct pof_queryall_request *)ptr;
    POF_HTONS_FUNC(p->slotID);

    return POF_OK;
}

static uint32_t action_OUTPUT(void *ptr){
    pof_action_output *p = (pof_action_output *)ptr;

#ifdef POF_SD2N
	value(p->portId_type, &p->outputPortId);
#else // POF_SD2N
    POF_NTOHL_FUNC(p->outputPortId);
#endif // POF_SD2N
    POF_NTOHS_FUNC(p->metadata_offset);
    POF_NTOHS_FUNC(p->metadata_len);
    POF_NTOHS_FUNC(p->packet_offset);

    return POF_OK;
}

static uint32_t action_ADD_FIELD(void *ptr){
    pof_action_add_field *p = (pof_action_add_field *)ptr;

    POF_NTOHS_FUNC(p->tag_pos);
#ifdef POF_SHT_VXLAN
    POF_NTOHS_FUNC(p->tag_len);
    value_byte(p->tag_type, &p->tag);
#else // POF_SHT_VXLAN
    POF_NTOHS_FUNC(p->tag_id);
    POF_NTOHL_FUNC(p->tag_len);
#ifdef POF_SD2N_AFTER1015
#else // POF_SD2N_AFTER1015
    POF_NTOH64_FUNC(p->tag_value);
#endif // POF_SD2N_AFTER1015
#endif // POF_SHT_VXLAN

    return POF_OK;
}

static uint32_t action_DROP(void *ptr){
    pof_action_drop *p = (pof_action_drop *)ptr;

#ifdef POF_SHT_VXLAN
    value(p->code_type, &p->reason_code);
#else // POF_SHT_VXLAN
    POF_NTOHL_FUNC(p->reason_code);
#endif // POF_SHT_VXLAN

    return POF_OK;
}

static uint32_t action_PACKET_IN(void *ptr){
    pof_action_packet_in *p = (pof_action_packet_in *)ptr;

#ifdef POF_SHT_VXLAN
    value(p->code_type, &p->reason_code);
#else // POF_SHT_VXLAN
    POF_NTOHL_FUNC(p->reason_code);
#endif // POF_SHT_VXLAN

    return POF_OK;
}


static uint32_t action_DELETE_FIELD(void *ptr){
    pof_action_delete_field *p = (pof_action_delete_field *)ptr;

    POF_NTOHS_FUNC(p->tag_pos);
#ifdef POF_SD2N
    value(p->len_type, &p->tag_len);
#else // POF_SD2N
    POF_NTOHL_FUNC(p->tag_len);
#endif // POF_SD2N

    return POF_OK;
}

static uint32_t action_CALCULATE_CHECKSUM(void *ptr){
    pof_action_calculate_checksum *p = (pof_action_calculate_checksum *)ptr;

    POF_NTOHS_FUNC(p->checksum_pos);
    POF_NTOHS_FUNC(p->checksum_len);
    POF_NTOHS_FUNC(p->cal_startpos);
    POF_NTOHS_FUNC(p->cal_len);

    return POF_OK;
}

static uint32_t action_SET_FIELD(void *ptr){
    pof_action_set_field *p = (pof_action_set_field *)ptr;

#ifdef POF_SHT_VXLAN
    match(&p->dst_field);
    value(p->src_type, &p->src);
#else // POF_SHT_VXLAN
    match_x(&p->field_setting);
#endif // POF_SHT_VXLAN

    return POF_OK;
}

static uint32_t action_SET_FIELD_FROM_METADATA(void *ptr){
    pof_action_set_field_from_metadata *p = (pof_action_set_field_from_metadata *)ptr;

    match(&p->field_setting);
    POF_NTOHS_FUNC(p->metadata_offset);

    return POF_OK;
}

static uint32_t action_MODIFY_FIELD(void *ptr){
    pof_action_modify_field *p = (pof_action_modify_field *)ptr;

    match(&p->field);
    POF_NTOHL_FUNC(p->increment);

    return POF_OK;
}

static uint32_t action_COUNTER(void *ptr){
    pof_action_counter *p = (pof_action_counter *)ptr;

#ifdef POF_SHT_VXLAN
    value(p->id_type, &p->counter_id);
#else // POF_SHT_VXLAN
    POF_NTOHL_FUNC(p->counter_id);
#endif // POF_SHT_VXLAN

    return POF_OK;
}

static uint32_t action_GROUP(void *ptr){
    pof_action_group *p = (pof_action_group *)ptr;

    POF_NTOHL_FUNC(p->group_id);

    return POF_OK;
}

static uint32_t 
action_EXPERIMENTER(void *ptr)
{
	POF_ERROR_CPRINT_FL_NO_LOCK("Unsupport action type.");
	return POF_ERROR;
}

static uint32_t action(void *ptr){
    pof_action *p = (pof_action *)ptr;

    if(p->len > 0x00FF){
        POF_NTOHS_FUNC(p->type);
        POF_NTOHS_FUNC(p->len);
        switch(p->type){
#define ACTION(NAME,VALUE) case POFAT_##NAME: action_##NAME(p->action_data); break;
		    ACTIONS
#undef ACTION
            default:
                POF_ERROR_CPRINT_FL_NO_LOCK("Unknow action type: %u", p->type);
                break;
        }
    }else{
        switch(p->type){
#define ACTION(NAME,VALUE) case POFAT_##NAME: action_##NAME(p->action_data); break;
		    ACTIONS
#undef ACTION
            default:
                POF_ERROR_CPRINT_FL_NO_LOCK("Unknow action type: %u", p->type);
                break;
        }
        POF_NTOHS_FUNC(p->type);
        POF_NTOHS_FUNC(p->len);
    }

    return POF_OK;
}

/* Intructions. */
static uint32_t
instruction_WRITE_ACTIONS(void *ptr)
{
	POF_ERROR_CPRINT_FL_NO_LOCK("Unsupport instruction type.");
	return POF_ERROR;
}

static uint32_t
instruction_CLEAR_ACTIONS(void *ptr)
{
	POF_ERROR_CPRINT_FL_NO_LOCK("Unsupport instruction type.");
	return POF_ERROR;
}

static uint32_t
instruction_EXPERIMENTER(void *ptr)
{
	POF_ERROR_CPRINT_FL_NO_LOCK("Unsupport instruction type.");
	return POF_ERROR;
}

static uint32_t instruction_GOTO_DIRECT_TABLE(void *ptr){
    pof_instruction_goto_direct_table *p = (pof_instruction_goto_direct_table *)ptr;

#ifdef POF_SD2N
	value(p->index_type, &p->table_entry_index);
#else // POF_SD2N
    POF_NTOHL_FUNC(p->table_entry_index);
#endif // POF_SD2N
    POF_NTOHS_FUNC(p->packet_offset);

    return POF_OK;
}

static uint32_t instruction_METER(void *ptr){
    pof_instruction_meter *p = (pof_instruction_meter *)ptr;

#ifdef POF_SHT_VXLAN
    value(p->id_type, &p->meter_id);
#else // POF_SHT_VXLAN
    POF_NTOHL_FUNC(p->meter_id);
#endif // POF_SHT_VXLAN

    return POF_OK;
}

static uint32_t instruction_WRITE_METADATA(void *ptr){
    pof_instruction_write_metadata *p = (pof_instruction_write_metadata *)ptr;

    POF_NTOHS_FUNC(p->metadata_offset);
    POF_NTOHS_FUNC(p->len);
#ifdef POF_SD2N
#else // POF_SD2N
    POF_NTOHL_FUNC(p->value);
#endif // POF_SD2N

    return POF_OK;
}

static uint32_t instruction_WRITE_METADATA_FROM_PACKET(void *ptr){
    pof_instruction_write_metadata_from_packet *p = \
                    (pof_instruction_write_metadata_from_packet *)ptr;

    POF_NTOHS_FUNC(p->metadata_offset);
    POF_NTOHS_FUNC(p->packet_offset);
    POF_NTOHS_FUNC(p->len);

    return POF_OK;
}

static uint32_t instruction_GOTO_TABLE(void *ptr){
    pof_instruction_goto_table *p = (pof_instruction_goto_table *)ptr;
    int i;

    POF_NTOHS_FUNC(p->packet_offset);
    for(i=0; i<POF_MAX_MATCH_FIELD_NUM; i++)
        match((pof_match *)p->match + i);

    return POF_OK;
}

static uint32_t instruction_APPLY_ACTIONS(void *ptr){
    pof_instruction_apply_actions *p = (pof_instruction_apply_actions *)ptr;
    struct pof_action *act = p->action;
    uint8_t i;

    for(i=0; i<p->action_num; i++){
        action(act);
#ifdef POF_SHT_VXLAN
        act = (struct pof_action *)( \
                (uint8_t *)act + act->len );
#else // POF_SHT_VXLAN
        act ++;
#endif // POF_SHT_VXLAN
    }

    return POF_OK;
}

/*add by wenjian 2015/12/01*/
uint32_t pof_NtoH_transfer_packet_out(void *ptr){
	pof_packet_out *p=(pof_packet_out*)ptr;
	POF_NTOHL_FUNC(p->bufferId);
	POF_NTOHL_FUNC(p->inPort);
	POF_NTOHL_FUNC(p->packetLen);
	struct pof_action *act = p->actionList;
	uint8_t i;

	 for(i=0; i<p->actionNum; i++){
	        action(act);
	  #ifdef POF_SHT_VXLAN
	        act = (struct pof_action *)( \
	                (uint8_t *)act + act->len );
	  #else // POF_SHT_VXLAN
	        act ++;
	  #endif // POF_SHT_VXLAN
	 }
	//the packet data don't need to transfer
	return POF_OK;
}


#ifdef POF_SD2N
static uint32_t 
instruction_CONDITIONAL_JMP(void *ptr)
{
	struct pof_instruction_conditional_jump *p = \
			(struct pof_instruction_conditional_jump *)ptr;

	match(&p->compare_field1);

	value(p->field2_type, &p->compare_field2);
	value(p->offset1_type, &p->offset1);
	value(p->offset2_type, &p->offset2);
	value(p->offset3_type, &p->offset3);

	return POF_OK;
}

static uint32_t
instruction_CALCULATE_FIELD(void *ptr)
{
	struct pof_instruction_calc_field *p = \
			(struct pof_instruction_calc_field *)ptr;

	POF_NTOHS_FUNC(p->calc_type);
	match(&p->dst_field);

	value(p->src_type, &p->src_operand);
	
	return POF_OK;
}

static uint32_t
instruction_MOVE_PACKET_OFFSET(void *ptr)
{
	struct pof_instruction_mov_packet_offset *p = \
			(struct pof_instruction_mov_packet_offset *)ptr;

	value(p->value_type, &p->movement);

	return POF_OK;
}
#endif // POF_SD2N

static uint32_t instruction_HtoN(void *ptr){
    pof_instruction *p = (pof_instruction *)ptr;

    switch(p->type){
#define INSTRUCTION(NAME,VALUE) case POFIT_##NAME: instruction_##NAME(p->instruction_data); break;
		INSTRUCTIONS
#undef INSTRUCTION
        default:
            POF_ERROR_CPRINT_FL_NO_LOCK("Unknow Instruction type: %u", p->type);
            return POF_ERROR;
    }

    POF_NTOHS_FUNC(p->type);
    POF_NTOHS_FUNC(p->len);
    return POF_OK;
}

static uint32_t instruction_NtoH(void *ptr){
    pof_instruction *p = (pof_instruction *)ptr;

    POF_NTOHS_FUNC(p->type);
    POF_NTOHS_FUNC(p->len);

    switch(p->type){
#define INSTRUCTION(NAME,VALUE) case POFIT_##NAME: instruction_##NAME(p->instruction_data); break;
		INSTRUCTIONS
#undef INSTRUCTION
        default:
            POF_ERROR_CPRINT_FL_NO_LOCK("Unknow Instruction type: %u", p->type);
            return POF_ERROR;
    }
    return POF_OK;
}

uint32_t pof_HtoN_transfer_flow_entry(void *ptr){
    pof_flow_entry *p = (pof_flow_entry *)ptr;
    int i;

    POF_NTOHL_FUNC(p->counter_id);
    POF_NTOH64_FUNC(p->cookie);
    POF_NTOH64_FUNC(p->cookie_mask);
    POF_NTOHS_FUNC(p->idle_timeout);
    POF_NTOHS_FUNC(p->hard_timeout);
    POF_NTOHS_FUNC(p->priority);
    POF_NTOHL_FUNC(p->index);
#ifdef POF_MULTIPLE_SLOTS
    POF_NTOHS_FUNC(p->slotID);
#endif // POF_MULTIPLE_SLOTS

    for(i=0;i<p->match_field_num;i++)
        match_x((pof_match_x *)p->match+i);

#ifdef POF_SHT_VXLAN
    POF_NTOHS_FUNC(p->instruction_block_id);
    POF_NTOHS_FUNC(p->parameter_length);
#else // POF_SHT_VXLAN
    for(i=0;i<p->instruction_num;i++){
        instruction_HtoN((pof_instruction *)p->instruction+i);
    }
#endif // POF_SHT_VXLAN

    return POF_OK;
}

uint32_t pof_NtoH_transfer_flow_entry(void *ptr){
    pof_flow_entry *p = (pof_flow_entry *)ptr;
    int i;

    POF_NTOHL_FUNC(p->counter_id);
    POF_NTOH64_FUNC(p->cookie);
    POF_NTOH64_FUNC(p->cookie_mask);
    POF_NTOHS_FUNC(p->idle_timeout);
    POF_NTOHS_FUNC(p->hard_timeout);
    POF_NTOHS_FUNC(p->priority);
    POF_NTOHL_FUNC(p->index);
#ifdef POF_MULTIPLE_SLOTS
    POF_NTOHS_FUNC(p->slotID);
#endif // POF_MULTIPLE_SLOTS

    for(i=0;i<p->match_field_num;i++)
        match_x((pof_match_x *)p->match+i);

#ifdef POF_SHT_VXLAN
    POF_NTOHS_FUNC(p->instruction_block_id);
    POF_NTOHS_FUNC(p->parameter_length);
#else // POF_SHT_VXLAN
    for(i=0;i<p->instruction_num;i++){
        instruction_NtoH((pof_instruction *)p->instruction+i);
    }
#endif // POF_SHT_VXLAN

    return POF_OK;
}

uint32_t pof_NtoH_transfer_group(void *ptr){
    pof_group *p = (pof_group *)ptr;
    int i;

    POF_NTOHL_FUNC(p->group_id);
    POF_NTOHL_FUNC(p->counter_id);
    POF_NTOHS_FUNC(p->slotID);
    for(i=0;i<POF_MAX_ACTION_NUMBER_PER_GROUP;i++)
        action((pof_action *)p->action+i);

    return POF_OK;
}
