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
#include "../include/pof_memory.h"
#include "../include/pof_protocol_header.h"
#include <string.h>

/* Update action pointer and number in dpp when one action
 * has been done. */
static void action_update(struct pofdp_packet *dpp){
    if(--dpp->act_num < 1){
        return;
    }
#ifdef POF_SHT_VXLAN
    dpp->act = (struct pof_action *)( 
            (uint8_t *)dpp->act + dpp->act->len );
#else // POF_SHT_VXLAN
    dpp->act++;
#endif // POF_SHT_VXLAN
    return;
}

/**********************************************************************
 * Make the piece of packet to be zero.
 * Form:     pofdp_bzero_bit(uint8_t *data, uint16_t pos_b, uint16_t len_b)
 * Input:    packet, start position(bit), length(bit)
 * Output:   packet
 * Return:   POF_OK or Error code
 * Discribe: This function makes the piece of packet with pos_b and len_b
 *           to be zero. The units of pos_b and len_b both are BIT.
 *********************************************************************/
static uint32_t bzero_bit(uint8_t *data, uint16_t pos_b, uint16_t len_b){
    uint32_t ret;
    uint16_t len_B;
    uint8_t  *zero;

    if((pos_b % POF_BITNUM_IN_BYTE == 0) && (len_b % POF_BITNUM_IN_BYTE == 0)){
        memset(data + pos_b / POF_BITNUM_IN_BYTE, 0, len_b / POF_BITNUM_IN_BYTE);
        return POF_OK;
    }

    len_B = POF_BITNUM_TO_BYTENUM_CEIL(len_b);

    POF_MALLOC_SAFE_RETURN(zero, len_B, POF_ERROR);
    pofbf_cover_bit(data, zero, pos_b, len_b);

    FREE(zero);
    return POF_OK;
}

/* Insert a tag into the packet data. */
static uint32_t
insertTagToPacket(uint32_t tag_pos_b, uint32_t tag_len_b, const uint8_t *value, POFDP_ARG)
{
    uint8_t  buf_behindtag[POFDP_PACKET_RAW_MAX_LEN];
    uint32_t len_b_behindtag = 0;

    /* The length behind the tag, unit is bit. */
    len_b_behindtag = dpp->left_len * 8 - tag_pos_b;

    /* Check the length. */
    if((dpp->offset + dpp->left_len + POF_BITNUM_TO_BYTENUM_CEIL(tag_len_b)) > POFDP_PACKET_RAW_MAX_LEN \
            || len_b_behindtag < 0){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    /* Copy the data left to a temp buffer. */
    pofbf_copy_bit(dpp->buf_offset, buf_behindtag, tag_pos_b, len_b_behindtag);
    /* Copy the tag value into the packet data. */
    pofbf_cover_bit(dpp->buf_offset, value, tag_pos_b, tag_len_b);
    /* Copy the temp buffer back to the packet data behind the tag. */
    pofbf_cover_bit(dpp->buf_offset, buf_behindtag, tag_pos_b+tag_len_b, len_b_behindtag);

    /* Updata the packet length. */
    dpp->left_len += POF_BITNUM_TO_BYTENUM_CEIL(tag_len_b);
    POF_PACKET_REL_LEN_INC(dpp, POF_BITNUM_TO_BYTENUM_CEIL(tag_len_b));

    return POF_OK;
}

/***********************************************************************
 * Calculate the checksum.
 * Form:     static uint32_t pofdp_checksum(uint8_t *data, \
 *                                          uint64_t *value, \
 *                                          uint16_t pos_b, \
 *                                          uint16_t len_b, \
 *                                          uint16_t cs_len_b)
 * Input:    data, start position(bit), length(bit), checksum len(bit)
 * Output:   calculate result
 * Return:   POF_OK or Error code
 * Discribe: This function calculates the checksum of the piece of data
 *           with pos_b and length. The length of checksum is cs_len_b.
 *           The result will be stored in value. The units of pos_b,
 *           len_b, cs_len_b all are BIT.
 * NOTE:     The max of length of checksum is 64bits.
 ***********************************************************************/
static uint32_t checksum(uint8_t *data, \
                         uint64_t *value, \
                         uint16_t pos_b, \
                         uint16_t len_b, \
                         uint16_t cs_len_b)
{
    uint64_t *value_temp, threshold = 1;
    uint32_t ret;
    uint16_t cal_num, i;
    uint8_t  data_temp[8] = {0};

    cal_num = len_b / cs_len_b;
    value_temp = (uint64_t *)data_temp;

    for(i=0; i<cs_len_b; i++){
        threshold *= 2;
    }

    for(i=0; i<cal_num; i++){
        memset(data_temp, 0, sizeof(data_temp));
        pofbf_copy_bit(data, data_temp, pos_b + i*cs_len_b, cs_len_b);

        POF_NTOH64_FUNC(*value_temp);
        *value_temp = ~*value_temp;
        *value_temp = POF_MOVE_BIT_RIGHT(*value_temp, 64 - cs_len_b);

        *value += *(value_temp);
        if(*value >= threshold){
            *value = *value - threshold + 1;
        }
    }

    *value = POF_MOVE_BIT_LEFT(*value, 64 - cs_len_b);
    POF_NTOH64_FUNC(*value);

    return POF_OK;
}

/* Calculate the checksum using the normal way.
 * NOT for TCP. */
static uint32_t
calChecksum(uint8_t *checksum_buf, uint8_t *cal_buf, \
        uint16_t cal_pos_b, uint16_t cal_len_b, uint16_t cs_pos_b, uint16_t cs_len_b)
{
    uint64_t checksum_value = 0;
    uint32_t ret;

    ret = bzero_bit(checksum_buf, cs_pos_b, cs_len_b);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    ret = checksum(cal_buf, &checksum_value, cal_pos_b, cal_len_b, cs_len_b);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    pofbf_cover_bit(checksum_buf, (uint8_t *)&checksum_value, cs_pos_b, cs_len_b);

    POF_DEBUG_CPRINT_FL_0X(1,GREEN,&checksum_value,8,"Calculate_checksum has been done! The checksum_value = ");

    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_COUNTER type.
 * Form:     execute_COUNTER(POFDP_ARG)
 * Input:    dpp->act, dpp->act_num, dp->resource
 * Return:   POF_OK or Error code
 * Discribe: This function handles the action with POFAT_COUNTER type. The
 *           counter value corresponding the counter_id given by
 *           action_data will increase by 1.
 ***********************************************************************/
static uint32_t execute_COUNTER(POFDP_ARG)
{
    pof_action_counter *p = (pof_action_counter *)dpp->act->action_data;
    uint32_t ret, counterID = 0;

    /* Get counterID. */
#ifdef POF_SHT_VXLAN
    ret = pofdp_get_32value(&counterID, p->id_type, &p->counter_id, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
#else // POF_SHT_VXLAN
    counterID = p->counter_id;
#endif // POF_SHT_VXLAN

    /* Increace the Counter. */
#ifdef POF_SD2N
    ret = poflr_counter_increace(counterID, 0, lr);
#else // POF_SD2N
    ret = poflr_counter_increace(counterID, lr);
#endif // POF_SD2N
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,GREEN,"action_counter has been done!");
    action_update(dpp);
    return POF_OK;
}

#ifdef POF_SHT_VXLAN
static void
headerFmtVxlan(struct vxlan_header *header, const uint8_t *vni, POFDP_ARG)
{
    memset(header, 0, sizeof(*header));
    header->flag = POF_VXLAN_FLAG;
    memcpy(header->vni, vni, POF_VXLAN_VNI_LEN);
}

static void
headerFmtUdp(struct udp_header *header, uint16_t sport, uint16_t dport, POFDP_ARG)
{
    memset(header, 0, sizeof(*header));
    header->sport = POF_NTOHS(sport);
    header->dport = POF_NTOHS(dport);
    header->len = POF_NTOHS(sizeof(struct udp_header) + dpp->left_len);
    header->check = 0;  /* Checksum in UDP is disabled. */
}

static void
headerFmtMac(struct eth_header *header, const uint8_t *dmac, \
        const uint8_t *smac, uint16_t ethType, POFDP_ARG)
{
    memset(header, 0, sizeof(*header));
    memcpy(header->dmac, dmac, POF_ETH_ALEN);
    memcpy(header->smac, smac, POF_ETH_ALEN);
    header->ethType = POF_NTOHS(ethType);
}

static void
headerFmtIpv4(struct ipv4_header *header, const uint8_t *sip, \
        const uint8_t *dip, uint8_t tos, uint8_t protocol, POFDP_ARG)
{
    memset(header, 0, sizeof(*header));
    header->version = POF_IPV4_VERSION;
    header->ihl = POF_IPV4_HEADER_LEN;
    header->tos = tos;
    header->totalLen = POF_NTOHS(sizeof(struct ipv4_header) + dpp->left_len);
    header->id = POF_IPV4_ID_DEFAULT;
    header->frag_off = POF_IPV4_FRAG_OFF_DEFAULT;
    header->ttl = POF_IPV4_TTL_DEFAULT;
    header->protocol = protocol;
    header->checkSum = 0;   /* Calculate the checksum later. */
    memcpy(&header->sip, sip, POF_IPV4_ALEN);
    memcpy(&header->dip, dip, POF_IPV4_ALEN);
}

static uint32_t
execute_ENCAP_VXLAN_HEADER(POFDP_ARG)
{
	struct pof_action_encap_vxlan_header *p = \
			(struct pof_action_encap_vxlan_header *)dpp->act->action_data;
    uint8_t vni[POF_VXLAN_VNI_LEN] = {0};
    uint32_t ret;

    /* Get byte VNI. */
    ret = pofdp_get_value_byte(vni, POF_VXLAN_VNI_LEN * POF_BITNUM_IN_BYTE, \
            p->vni_type, &p->vni, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Format the VXLAN header. */
    struct vxlan_header *header = (struct vxlan_header *)dpp->buf_offset;
    headerFmtVxlan(header, vni, dpp, lr);

    POF_PACKET_REL_LEN_INC(dpp, sizeof(*header));

    POF_DEBUG_CPRINT_FL_0X(1,GREEN, header, sizeof(*header), \
            "action_encap_vxlan_header has been DONE! The header is: ");
    action_update(dpp);
    return POF_OK;
}

static uint32_t
execute_ENCAP_UDP_HEADER(POFDP_ARG)
{
	struct pof_action_encap_udp_header *p = \
			(struct pof_action_encap_udp_header *)dpp->act->action_data;
    uint16_t sport = 0, dport = 0;
    uint32_t ret;

    /* Get port. */
    ret = pofdp_get_16value(&sport, p->sport_type, &p->sport, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    ret = pofdp_get_16value(&dport, p->dport_type, &p->dport, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Format the VXLAN header. */
    struct udp_header *header = (struct udp_header *)dpp->buf_offset;
    headerFmtUdp(header, sport, dport, dpp, lr);

    POF_PACKET_REL_LEN_INC(dpp, sizeof(*header));

    POF_DEBUG_CPRINT_FL_0X(1,GREEN, header, sizeof(*header), \
            "action_encap_udp_header has been DONE! The header is: ");
    action_update(dpp);
    return POF_OK;
}

static uint32_t
execute_ENCAP_IP_HEADER(POFDP_ARG)
{
	struct pof_action_encap_ip_header *p = \
			(struct pof_action_encap_ip_header *)dpp->act->action_data;
    uint16_t ethType = 0;
    uint8_t sip[POF_IPV4_ALEN] = {0}, dip[POF_IPV4_ALEN] = {0}, tos = 0, protocol = 0;
    uint32_t ret;

    /* Get IP address. */
    ret = pofdp_get_value_byte(sip, POF_IPV4_ALEN * POF_BITNUM_IN_BYTE, p->sip_type, &p->sip, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    ret = pofdp_get_value_byte(dip, POF_IPV4_ALEN * POF_BITNUM_IN_BYTE, p->dip_type, &p->dip, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    /* Get tos. */
    ret = pofdp_get_value_byte(&tos, POF_BITNUM_IN_BYTE, p->tos_type, &p->tos, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    /* Get protocol. */
    protocol = p->protocol;

    /* Format the VXLAN header. */
    /* Copy the header into the packet, at current packet pointer, cover mode. */
    struct ipv4_header *header = (struct ipv4_header *)dpp->buf_offset;
    headerFmtIpv4(header, sip, dip, tos, protocol, dpp, lr);

    /* Calculate the Checksum of the IPv4 header. */
    ret = calChecksum(dpp->buf_offset, dpp->buf_offset, 0, 160, 80, 16);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_PACKET_REL_LEN_INC(dpp, sizeof(*header));

    POF_DEBUG_CPRINT_FL_0X(1,GREEN, header, sizeof(*header), \
            "action_encap_ip_header has been DONE! The header is: ");
    action_update(dpp);
    return POF_OK;
}

static uint32_t
execute_ENCAP_MAC_HEADER(POFDP_ARG)
{
	struct pof_action_encap_mac_header *p = \
			(struct pof_action_encap_mac_header *)dpp->act->action_data;
    uint16_t ethType = 0;
    uint8_t dmac[POF_ETH_ALEN] = {0}, smac[POF_ETH_ALEN] = {0};
    uint32_t ret;

    /* Get mac. */
    ret = pofdp_get_value_byte(dmac, POF_ETH_ALEN * POF_BITNUM_IN_BYTE, p->dmac_type, &p->dmac, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    ret = pofdp_get_value_byte(smac, POF_ETH_ALEN * POF_BITNUM_IN_BYTE, p->smac_type, &p->smac, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    /* Get ethType. */
    ethType = p->ethType;

    /* Format the VXLAN header. */
    struct eth_header *header = (struct eth_header *)dpp->buf_offset;
    headerFmtMac(header, dmac, smac, ethType, dpp, lr);

    POF_PACKET_REL_LEN_INC(dpp, sizeof(*header));

    POF_DEBUG_CPRINT_FL_0X(1,GREEN, header, sizeof(*header), \
            "action_encap_mac_header has been DONE! The header is: ");
    action_update(dpp);
    return POF_OK;
}

/* Execute different type of CALCULATE_FIELD action. */
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
	struct pof_action_calc_field *p = \
			(struct pof_action_calc_field *)dpp->act->action_data;
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
			POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBAC_BAD_TYPE, g_upward_xid++);
			break;
	}

	ret = pofdp_write_32value_to_field(result, &p->dst_field, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	POF_DEBUG_CPRINT_FL(1,GREEN,"action_calculate_field has been done! " \
			"cal_type = %u, value = %u (0x%x), operand = %u (0x%x), result = %u (0x%x)", \
			p->calc_type, value1, value1, operand, operand, result, result);
    action_update(dpp);
    return POF_OK;
}

#endif // POF_SHT_VXLAN

/***********************************************************************
 * Handle the action with POFAT_GROUP type.
 * Form:     uint32_t execute_GROUP(POFDP_ARG)
 * Input:    dpp, dp->resource, 
 * Return:   POF_OK or Error code
 * Discribe: This function handles the action with POFAT_GROUP type. The
 *           packet will be forward to the group table. The group id is
 *           given in action_data. In this version, we consider all group 
 *           type is all.
 * Note:     If there is an ERROR, The packet_over identifier will be TRUE
 ***********************************************************************/
static uint32_t execute_GROUP(POFDP_ARG)
{
    pof_action_group *p = (pof_action_group *)dpp->act->action_data;
    struct groupInfo *group;
    uint32_t   group_id, ret;

    group_id = p->group_id;
    if(!(group = poflr_get_group_with_ID(group_id, lr))){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_UNKNOWN_GROUP, g_upward_xid++);
    }

    POF_DEBUG_CPRINT_FL(1,BLUE,"Go to Group[%u]", group_id);

#ifdef POF_SD2N
    ret = poflr_counter_increace(group->counter_id, POF_PACKET_REL_LEN_GET(dpp), lr);
#else // POF_SD2N
    ret = poflr_counter_increace(group->counter_id, lr);
#endif // POF_SD2N
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	dpp->act = group->action;
	dpp->act_num = group->action_number;

    ret = pofdp_action_execute(dpp, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,GREEN,"action_group has been DONE!");
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_TOCP type.
 * Form:     uint32_t execute_PACKET_IN(POFDP_ARG)
 * Input:    dpp->act, dpp->act_num, dpp->buf, dpp->offset, dpp->left_len,
 *           dpp->table_type, dpp->table_id, dp->resource
 * Return:   POF_OK or Error code
 * Discribe: This function send the packet upward to the Controller through
 *           the OpenFlow Channel.
 ***********************************************************************/
static uint32_t execute_PACKET_IN(POFDP_ARG)
{
    pof_action_packet_in *p = (pof_action_packet_in *)dpp->act->action_data;
    uint32_t reason, ret;
    uint8_t  table_ID;

#ifdef POF_SHT_VXLAN
    ret = pofdp_get_32value(&reason, p->code_type, &p->reason_code, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
#else // POF_SHT_VXLAN
    reason = p->reason_code;
#endif // POF_SHT_VXLAN

    ret = poflr_table_id_to_ID(dpp->table_type, dpp->table_id, &table_ID, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->packetBuf, dpp->offset + dpp->left_len, "The packet in data is ");
    ret = pofdp_send_packet_in_to_controller(dpp->offset + dpp->left_len, \
            reason, table_ID, POF_FE_ID, dpp->ori_port_id, lr->slotID, dpp->packetBuf);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,BLUE,"action_packet_in has been done! The packet in reason is %d.", reason);
	POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->packetBuf, dpp->offset + dpp->left_len, "The packet in data is ");

    action_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_DROP type.
 * Form:     uint32_t execute_DROP(POFDP_ARG)
 * Input:    dpp->act, dpp->act_num
 * Return:   POF_OK or Error code
 * Discribe: This function drops the packet out of the switch.
 ***********************************************************************/
static uint32_t execute_DROP(POFDP_ARG)
{
    pof_action_drop *p = (pof_action_drop *)dpp->act->action_data;
    uint32_t reason = 0, ret;

#ifdef POF_SHT_VXLAN
    ret = pofdp_get_32value(&reason, p->code_type, &p->reason_code, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
#else // POF_SHT_VXLAN
    reason = p->reason_code;
#endif // POF_SHT_VXLAN

    POF_DEBUG_CPRINT_FL(1,BLUE,"action_drop has been done! The drop reason is %d\n.", reason);

    dpp->packet_done = TRUE;

    action_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_SET_FIELD type.
 * Form:     uint32_t execute_SET_FIELD(POFDP_ARG)
 * Input:    dpp->act, dpp->act_num, dpp->buf_offset, dpp->left_len,
 * Return:   POF_OK or Error code
 * Discribe: This function will set the field of the packet by the value
 *           given by action_data
 ***********************************************************************/
static uint32_t execute_SET_FIELD(POFDP_ARG)
{
    pof_action_set_field *p = (pof_action_set_field *)dpp->act->action_data;
    uint32_t i, ret;
    uint16_t offset_b, len_b;
    uint8_t  value[POFDP_PACKET_RAW_MAX_LEN] = {0};
    uint8_t *dst = NULL;

#ifdef POF_SHT_VXLAN
    offset_b = p->dst_field.offset;
    len_b = p->dst_field.len;

    /* Check len_b. */
    if(len_b > POFDP_PACKET_RAW_MAX_LEN * POF_BITNUM_IN_BYTE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_FIELD_LEN_ERROR, g_upward_xid++);
    }

    /* Get the buf according to the match field id. */
    if(!(dst = pofdp_get_field_buf(&p->dst_field, dpp))){
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(POF_ERROR);
    }
    ret = pofdp_get_value_byte(value, len_b, p->src_type, &p->src, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
#else // POF_SHT_VXLAN
    offset_b = p->field_setting.offset;
    len_b = p->field_setting.len;

    /* Check packet length. */
    if(dpp->left_len * POF_BITNUM_IN_BYTE < offset_b + len_b){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    for(i=0; i<POF_MAX_FIELD_LENGTH_IN_BYTE; i++){
        value[i] = p->field_setting.value[i] & p->field_setting.mask[i];
    }

    dst = dpp->buf_offset;
#endif // POF_SHT_VXLAN
    pofbf_cover_bit(dst, value, offset_b, len_b);

    POF_DEBUG_CPRINT_FL(1,GREEN,"action_set_field has been DONE");
	POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf_offset,dpp->left_len,"The packet is ");

    action_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_SET_FIELD_FROM_METADATA type.
 * Form:     uint32_t execute_SET_FIELD_FROM_METADATA(POFDP_ARG)
 * Input:    dpp->act, dpp->act_num, dpp->buf_offset, dpp->left_len,
 *           dpp->metadata
 * Return:   POF_OK or Error code
 * Discribe: This function will set the field of the packet from the piece
 *           of metadata.
 ***********************************************************************/
static uint32_t execute_SET_FIELD_FROM_METADATA(POFDP_ARG)
{
    pof_action_set_field_from_metadata *p = \
            (pof_action_set_field_from_metadata *)dpp->act->action_data;
    uint32_t ret;
    uint16_t offset_b, len_b, metadata_offset_b;
    uint8_t  value[POF_MAX_FIELD_LENGTH_IN_BYTE];

    offset_b = p->field_setting.offset;
    len_b = p->field_setting.len;
    metadata_offset_b = p->metadata_offset;

    /* Check packet length. */
    if(dpp->left_len * POF_BITNUM_IN_BYTE < offset_b + len_b){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    pofbf_copy_bit((uint8_t *)dpp->metadata, value, metadata_offset_b, len_b);

    pofbf_cover_bit(dpp->buf_offset, value, offset_b, len_b);

	POF_DEBUG_CPRINT_FL_0X(1,GREEN,value,POF_BITNUM_TO_BYTENUM_CEIL(len_b), \
			"Set_field_from_metadata has been done! The metadata is :");
    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf_offset,dpp->left_len,"The packet is :");

    action_update(dpp);
    return POF_OK;
}

#ifndef POF_SHT_VXLAN
/***********************************************************************
 * Handle the action with POFAT_MODIFY_FIELD type.
 * Form:     uint32_t execute_MODIFY_FIELD(POFDP_ARG)
 * Input:    dpp
 * Return:   POF_OK or Error code
 * Discribe: This function will modify the field of the packet. The value
 *           will be increase by the number given by action_data.
 ***********************************************************************/
static uint32_t execute_MODIFY_FIELD(POFDP_ARG)
{
    pof_action_modify_field *p = (pof_action_modify_field *)dpp->act->action_data;
    uint32_t value, ret;

	ret = pofdp_get_32value(&value, POFVT_FIELD, &p->field, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    value += p->increment;

	ret = pofdp_write_32value_to_field(value, &p->field, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,GREEN,"action_modeify_field has been done! The increment is %d", p->increment);
    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf_offset,dpp->left_len,"The packet is ");

    action_update(dpp);
    return POF_OK;
}
#endif // POF_SHT_VXLAN


/***********************************************************************
 * Handle the action with POFAT_CALCULATE_CHECKSUM type.
 * Form:     uint32_t execute_CALCULATE_CHECKSUM(POFDP_ARG)
 * Input:    dpp->act, dpp->act_num, dpp->buf_left
 * Return:   POF_OK or Error code
 * Discribe: This function will calculate the checksum of the piece of
 *           packet specified in action_data. The calculate result will be
 *           stored in the specified piece of packet.
 ***********************************************************************/
static uint32_t execute_CALCULATE_CHECKSUM(POFDP_ARG)
{
    pof_action_calculate_checksum *p = \
            (pof_action_calculate_checksum *)dpp->act->action_data;
    uint32_t ret;
    uint16_t cal_pos_b, cal_len_b, cs_pos_b, cs_len_b = p->checksum_len;

    cal_pos_b = p->cal_startpos;
    cal_len_b = p->cal_len;
    cs_pos_b = p->checksum_pos;
    cs_len_b = p->checksum_len;

    if(cs_len_b > 64){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBAC_BAD_LEN, g_upward_xid++);
    }

    uint8_t *checksum_buf, *cal_buf;
#ifdef POF_SD2N_AFTER1015

    if (p->checksum_pos_type == 0){
        checksum_buf = dpp->buf_offset;
    }else{
        checksum_buf = (uint8_t *)dpp->metadata;
    }

    if (p->cal_startpos_type == 0){
        cal_buf = dpp->buf_offset;
    }else{
        cal_buf = (uint8_t *)dpp->metadata;
    }
#else // POF_SD2N_AFTER1015
    checksum_buf = dpp->buf_offset;
    cal_buf = dpp->buf_offset;
#endif // POF_SD2N_AFTER1015

    ret = calChecksum(checksum_buf, cal_buf, cal_pos_b, cal_len_b, cs_pos_b, cs_len_b);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,GREEN,"action_calculate_checksum has been done!");

    action_update(dpp);
    return POF_OK;
}


/***********************************************************************
 * Handle the action with POFAT_OUTPUT type.
 * Form:     uint32_t execute_OUTPUT(POFDP_ARG)
 * Input:    dpp, dp->resource
 * Output:   dpp
 * Return:   POF_OK or Error code
 * Discribe: This function will send the packet out through the physical
 *           port specified in action_data. The specified piece of metadata
 *           should be send before the packet. The metadata and packet
 *           consist a new packet.
 ***********************************************************************/
static uint32_t execute_OUTPUT(POFDP_ARG)
{
    pof_action_output *p = (pof_action_output *)dpp->act->action_data;
    struct pof_local_resource *lrPort = NULL;
    uint32_t ret, value = 0;

    struct portInfo *port, *next;//add by wenjian 2015/12/01
    if(p->packet_offset > dpp->left_len){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    if(POF_BITNUM_TO_BYTENUM_CEIL(p->metadata_len + p->metadata_offset) > dpp->metadata_len){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_METADATA_LEN_ERROR, g_upward_xid++);
    }

#ifdef POF_SD2N
	ret = pofdp_get_32value(&value, p->portId_type, &p->outputPortId, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
#else // POF_SD2N
    value = p->outputPortId;
#endif // POF_SD2N
    dpp->output_slot_id = value >> 16;
    dpp->output_port_id = value & 0xFFFF;

    if((lrPort = pofdp_get_local_resource(dpp->output_slot_id, dpp->dp)) == NULL){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_INVALID_SLOT_ID, g_upward_xid++);
    }

    dpp->output_packet_offset = p->packet_offset;   /* Byte unit. */
    /* Ouput packet from current pointer. */
    dpp->output_packet_len = dpp->left_len - dpp->output_packet_offset;   /* Byte unit. */
    dpp->output_packet_buf = dpp->buf_offset;
    /* Ouput packet from original pointer. */
//    dpp->output_packet_len = dpp->left_len + dpp->offset - dpp->output_packet_offset;   /* Byte unit. */
//    dpp->output_packet_buf = dpp->packetBuf; 
    dpp->output_metadata_len = POF_BITNUM_TO_BYTENUM_CEIL(p->metadata_len); /* Byte unit. */
    dpp->output_metadata_offset = p->metadata_offset;   /* Bit unit. */
    dpp->output_whole_len = dpp->output_packet_len + dpp->output_metadata_len;  /* Byte unit. */


    //if this is the flood portid
    if(dpp->output_port_id==255)
    {
     	POF_DEBUG_CPRINT(1,BLUE,"yes it is a flood port");
       //add by jelly
       //get all the port id execept the input port id
     	HMAP_NODES_IN_STRUCT_TRAVERSE(port, next, pofIndexNode, lrPort->portPofIndexMap){
        	POF_DEBUG_CPRINT(1,BLUE,"sysIndex=%d,pofIndex=%d,ori_port_id=%d\n",port->sysIndex,port->pofIndex,dpp->ori_port_id);
        	if(port->pofIndex!=dpp->ori_port_id&&port->pofIndex!=pofsc_conn_desc.local_port_index){
                if(port->config!=16)
                {
                	POF_DEBUG_CPRINT(1,BLUE,"config=%d",port->config);
           	        dpp->output_port_id=port->pofIndex;
                    ret = pofdp_send_raw(dpp, lrPort);
                    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
                }
        	}
        }
     }
     else
     {
     //if(dpp->output_port_id!=pofsc_conn_desc.local_port_index)
     //{
       ret = pofdp_send_raw(dpp, lrPort);
       POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
      //}
      }



    //ret = pofdp_send_raw(dpp, lrPort);
    //POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    action_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_ADD_TAG type.
 * Form:     execute_ADD_FIELD(POFDP_ARG)
 * Input:    dpp->buf_offset, dpp->left_len, dpp->act, dpp->act_num
 * Return:   POF_OK or Error code
 * Discribe: This function will add tag into the packet. The position and
 *           the value has been given by action_data. The length of packet
 *           will be changed after adding tag.
 ***********************************************************************/
static uint32_t execute_ADD_FIELD(POFDP_ARG)
{
    pof_action_add_field *p = (pof_action_add_field *)dpp->act->action_data;
    uint64_t value;
    uint32_t tag_len_b, ret;
    uint16_t tag_pos_b;
    uint8_t  *tag_value = NULL, value_byte[POFDP_PACKET_RAW_MAX_LEN] = {0};

    tag_pos_b = p->tag_pos;
    tag_len_b = p->tag_len;

    /* Get the tag_value. */
#ifdef POF_SHT_VXLAN
    /* Check len_b. */
    if(tag_len_b > POFDP_PACKET_RAW_MAX_LEN * POF_BITNUM_IN_BYTE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_FIELD_LEN_ERROR, g_upward_xid++);
    }
    /* Get value. */
    ret = pofdp_get_value_byte(value_byte, tag_len_b, p->tag_type, &p->tag, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    tag_value = value_byte;
#else // POF_SHT_VXLAN

    /* Check len_b. */
#ifdef POF_SD2N_AFTER1015
	if(tag_len_b > 128){
#else // POF_SD2N_AFTER1015
	if(tag_len_b > 64){
#endif
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_FIELD_LEN_ERROR, g_upward_xid++);
	}

    /* Get value. */
#ifdef POF_SD2N_AFTER1015
    tag_value = p->tag_value;
#else // POF_SD2N
    value = p->tag_value;
    value = POF_MOVE_BIT_LEFT(value, 64 - tag_len_b);
    value = POF_HTON64(value);
    tag_value = (uint8_t *)&value;
#endif // POF_SD2N

#endif // POF_SHT_VXLAN

    ret = insertTagToPacket(tag_pos_b, tag_len_b, tag_value, dpp, lr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,GREEN,"action_add_field has been done!");
    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf_offset,dpp->left_len,"The new packet = ");

    action_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_DELETE_TAG type.
 * Form:     uint32_t execute_DELETE_FIELD(POFDP_ARG)
 * Input:    dpp->act, dpp->act_num, dpp->buf_left, dpp->left_len
 * Return:   POF_OK or Error code
 * Discribe: This function will delete tag of packet. The position and the
 *           length of the tag has been given by action_data. The length
 *           of the packet will be changed after deleting tag.
 ***********************************************************************/
static uint32_t execute_DELETE_FIELD(POFDP_ARG)
{
    pof_action_delete_field *p = (pof_action_delete_field *)dpp->act->action_data;
    uint32_t ret, tag_len_b;
    uint16_t tag_pos_b, tag_len_b_x, len_b_behindtag;
    uint8_t  buf_temp[POFDP_PACKET_RAW_MAX_LEN] = {0};

    tag_pos_b = p->tag_pos;
#ifdef POF_SD2N
    ret = pofdp_get_32value(&tag_len_b, p->len_type, &p->tag_len, dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
#else // POF_SD2N
    tag_len_b = p->tag_len;
#endif // POF_SD2N
    tag_len_b_x = tag_len_b % POF_BITNUM_IN_BYTE;
    len_b_behindtag = dpp->left_len * POF_BITNUM_IN_BYTE - (tag_pos_b + tag_len_b);

    if((int16_t)len_b_behindtag < 0){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    pofbf_copy_bit(dpp->buf_offset, buf_temp, tag_pos_b+tag_len_b, len_b_behindtag);
    pofbf_cover_bit(dpp->buf_offset, buf_temp, tag_pos_b, len_b_behindtag);

    dpp->left_len = POF_BITNUM_TO_BYTENUM_CEIL(dpp->left_len * POF_BITNUM_IN_BYTE - tag_len_b);
    POF_PACKET_REL_LEN_DEC(dpp, (uint8_t)(tag_len_b / POF_BITNUM_IN_BYTE));

    if(tag_len_b_x != 0){
        *(dpp->buf_offset + dpp->left_len - 1) &= POF_MOVE_BIT_LEFT(0xff, tag_len_b_x);
    }

    POF_DEBUG_CPRINT_FL(1,GREEN,"action_delete_field has been done!");
    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf_offset,dpp->left_len,"The new packet = ");

    action_update(dpp);
    return POF_OK;
}

static uint32_t
execute_EXPERIMENTER(POFDP_ARG)
{
	// TODO
	POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBAC_BAD_TYPE, g_upward_xid++);
}

/***********************************************************************
 * Execute actions
 * Form:     uint32_t pofdp_action_execute(POFDP_ARG)
 * Input:    dpp, dp
 * Return:   POF_OK or Error code
 * Discribe: This function executes the actions. 
 ***********************************************************************/
uint32_t pofdp_action_execute(POFDP_ARG)
{
    uint32_t ret;

    while(dpp->packet_done == FALSE && dpp->act_num > 0){
		/* Execute the actions. */
        switch(dpp->act->type){
#define ACTION(NAME,VALUE) case POFAT_##NAME: ret = execute_##NAME(dpp, lr); break;
			ACTIONS
#undef ACTION

            default:
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBAC_BAD_TYPE, g_upward_xid++);
                break;
        }
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    }

    return POF_OK;
}

