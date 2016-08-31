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
#include "../include/pof_conn.h"
#include "../include/pof_local_resource.h"
#include "../include/pof_byte_transfer.h"
#include "../include/pof_log_print.h"
#include "../include/pof_datapath.h"
#include "../include/pof_hmap.h"

/* Xid in OpenFlow header received from Controller. */
uint32_t g_recv_xid = POF_INITIAL_XID;

/*******************************************************************************
 * Parse the OpenFlow message received from the Controller.
 * Form:     uint32_t  pof_parse_msg_from_controller(char* msg_ptr)
 * Input:    message length, message data
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function parses the OpenFlow message received from the Controller,
 *           and execute the response.
*******************************************************************************/
uint32_t  pof_parse_msg_from_controller(char* msg_ptr, struct pof_datapath *dp){
    struct pof_local_resource *lr, *next;
    uint16_t slot = POF_SLOT_ID_BASE;
    pof_switch_config *config_ptr;
    pof_header        *header_ptr, head;
    pof_flow_entry    *flow_ptr;
    pof_packet_out    *packet_out;//add by wenjian 2015/12/01
    pof_counter       *counter_ptr;
    pof_flow_table    *table_ptr;
    pof_port          *port_ptr;
    pof_meter         *meter_ptr;
    pof_group         *group_ptr;
    struct pof_queryall_request * queryall_ptr;
    struct pof_slot_config *slotConfig;
    struct pof_instruction_block *pof_insBlock;
    uint32_t          ret = POF_OK;
    uint16_t          len;
    uint8_t           msg_type;

    header_ptr = (pof_header*)msg_ptr;
    len = POF_NTOHS(header_ptr->length);

    /* Print the parse information. */
#ifndef POF_DEBUG_PRINT_ECHO_ON
    if(header_ptr->type != POFT_ECHO_REPLY){
#endif
    POF_DEBUG_CPRINT_PACKET(msg_ptr,0,len);
#ifndef POF_DEBUG_PRINT_ECHO_ON
    }
#endif

    /* Parse the OpenFlow packet header. */
    pof_NtoH_transfer_header(header_ptr);
    msg_type = header_ptr->type;
    g_recv_xid = header_ptr->xid;

    /* Execute different responses according to the OpenFlow type. */
    switch(msg_type){
        case POFT_ECHO_REQUEST:
            if(POF_OK != pofec_reply_msg(POFT_ECHO_REPLY, g_recv_xid, 0, NULL)){
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
            }
            break;

        case POFT_SET_CONFIG:
            config_ptr = (pof_switch_config *)(msg_ptr + sizeof(pof_header));
            pof_HtoN_transfer_switch_config(config_ptr);

            ret = poflr_set_config(config_ptr->flags, config_ptr->miss_send_len);
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        case POFT_GET_CONFIG_REQUEST:
            ret = poflr_reply_config();
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

            HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                ret = poflr_reply_table_resource(lr);
                POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            }
            HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                ret = poflr_reply_port_resource(lr);
                POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            }
#ifdef POF_SHT_VXLAN
            HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                ret = poflr_reply_slot_status(lr, POFSS_UP, POFSRF_RE_SEND);
                POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            }
#endif // POF_SHT_VXLAN
            break;

        case POFT_PORT_MOD:
            port_ptr = (pof_port*)(msg_ptr + sizeof(pof_header) + sizeof(pof_port_status) - sizeof(pof_port));
            pof_NtoH_transfer_port(port_ptr);

#ifdef POF_MULTIPLE_SLOTS
            slot = port_ptr->slotID;
#endif // POF_MULTIPLE_SLOTS
            if((lr = pofdp_get_local_resource(slot, dp)) == NULL){
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_INVALID_SLOT_ID, g_recv_xid);
            }

            ret = poflr_port_openflow_enable(port_ptr->port_id, port_ptr->of_enable, lr);
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

#ifdef POF_SHT_VXLAN
        case POFT_SLOT_CONFIG:
            slotConfig = (struct pof_slot_config *)(msg_ptr + sizeof(pof_header));
            pof_HtoN_transfer_slot_config(slotConfig);

            if((lr = pofdp_get_local_resource(slotConfig->slotID, dp)) == NULL){
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_INVALID_SLOT_ID, g_recv_xid);
            }

            struct portInfo *port, *nextPort;
            /* Traverse all ports. */
            HMAP_NODES_IN_STRUCT_TRAVERSE(port, nextPort, pofIndexNode, lr->portPofIndexMap){
                ret = poflr_port_openflow_enable(port->pofIndex, slotConfig->flag, lr);
                POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            }

            break;

        case POFT_INSTRUCTION_BLOCK_MOD:
            pof_insBlock = (struct pof_instruction_block *)(msg_ptr + sizeof(pof_header));
            pof_NtoH_transfer_insBlock(pof_insBlock);

            if(pof_insBlock->command == POFFC_ADD){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_add_insBlock(pof_insBlock, lr);
                }
            }else if(pof_insBlock->command == POFFC_MODIFY){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_modify_insBlock(pof_insBlock, lr);
                }
            }else if(pof_insBlock->command == POFFC_DELETE){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_delete_insBlock(pof_insBlock->instruction_block_id, lr);
                }
            }else{
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_INSBLOCK_MOD_FAILED, POFIMFC_BAD_COMMAND, g_recv_xid);
            }

            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;
#endif // POF_SHT_VXLAN

        case POFT_FEATURES_REQUEST:
            ret = poflr_reset_dev_id();
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

            HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                ret = poflr_reply_feature_resource(lr);
                POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            }
            break;

        case POFT_TABLE_MOD:
            table_ptr = (pof_flow_table*)(msg_ptr + sizeof(pof_header));
            pof_NtoH_transfer_flow_table(table_ptr);

            if(table_ptr->command == POFTC_ADD){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_create_flow_table(table_ptr->tid,               \
                                                  table_ptr->type,              \
                                                  table_ptr->key_len,           \
                                                  table_ptr->size,              \
                                                  table_ptr->table_name,        \
                                                  table_ptr->match_field_num,   \
                                                  table_ptr->match,             \
                                                  lr);
                }
            }else if(table_ptr->command == POFTC_DELETE){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_delete_flow_table(table_ptr->tid, table_ptr->type, lr);
                }
            }else{
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_COMMAND, g_recv_xid);
            }

            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        case POFT_FLOW_MOD:
            flow_ptr = (pof_flow_entry*)(msg_ptr + sizeof(pof_header));
            pof_NtoH_transfer_flow_entry(flow_ptr);

            if(flow_ptr->command == POFFC_ADD){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_add_flow_entry(flow_ptr, lr);
                }
            }else if(flow_ptr->command == POFFC_DELETE){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_delete_flow_entry(flow_ptr, lr);
                }
            }else if(flow_ptr->command == POFFC_MODIFY){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_modify_flow_entry(flow_ptr, lr);
                }
            }else{
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_COMMAND, g_recv_xid);
            }
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
//            usr_cmd_tables();

            break;

        /*add by wenjian 2015/12/01*/
        case POFT_PACKET_OUT:
         //first move the pointer to the packet_out from header
            packet_out=(pof_packet_out*)(msg_ptr+sizeof(pof_header));
         //take transfer from n to h
            pof_NtoH_transfer_packet_out(packet_out);
         //POF_DEBUG_CPRINT_OX_NO_ENTER(packet_out,sizeof(packet_out));
         struct pofdp_packet dpp[1] = {0};
         //POF_DEBUG_CPRINT(1,BLUE,"===============start memset\n");
         memset(dpp, 0, sizeof *dpp);
         //POF_DEBUG_CPRINT(1,BLUE,"===============memset success\n");
         //apply the packet_out to the pofdp_packet
         dpp->packetBuf = dpp->buf;
         //POF_DEBUG_CPRINT(1,BLUE,"===============%d point packetbuf to memory success\n",packet_out->packetLen);
         memcpy(dpp->buf,packet_out->data,packet_out->packetLen);
         //POF_DEBUG_CPRINT(1,BLUE,"===============memcpy success\n");
         /* Store packet data, length, received port infomation into the message queue. */
         // dpp->output_port_id=packet_out->inPort;
         dpp->ori_port_id=packet_out->inPort;
         dpp->ori_len = packet_out->packetLen;
         dpp->left_len = dpp->ori_len;
         dpp->buf_offset = dpp->packetBuf;
         dpp->dp = dp;
         dpp->act = (pof_action *)(packet_out->actionList);
         POF_DEBUG_CPRINT_0X_NO_ENTER(dpp->act,48*6);
         dpp->act_num =packet_out->actionNum;
         //POF_DEBUG_CPRINT(1,BLUE,"===============dpp initialize success\n");
         ret = pofdp_action_execute(dpp, lr);
         //POF_DEBUG_CPRINT(1,BLUE,"===============excute actions success\n");
         POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
         //free(dp);
         break;



         case POFT_METER_MOD:
            meter_ptr = (pof_meter*)(msg_ptr + sizeof(pof_header));
            pof_NtoH_transfer_meter(meter_ptr);

            if(meter_ptr->command == POFMC_ADD){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_add_meter_entry(meter_ptr->meter_id, meter_ptr->rate, lr);
                }
            }else if(meter_ptr->command == POFMC_MODIFY){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_modify_meter_entry(meter_ptr->meter_id, meter_ptr->rate, lr);
                }
            }else if(meter_ptr->command == POFMC_DELETE){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_delete_meter_entry(meter_ptr->meter_id, meter_ptr->rate, lr);
                }
            }else{
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_BAD_COMMAND, g_recv_xid);
            }

            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        case POFT_GROUP_MOD:
            group_ptr = (pof_group*)(msg_ptr + sizeof(pof_header));
            pof_NtoH_transfer_group(group_ptr);

            if(group_ptr->command == POFGC_ADD){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_add_group_entry(group_ptr, lr);
                }
            }else if(group_ptr->command == POFGC_MODIFY){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_modify_group_entry(group_ptr, lr);
                }
            }else if(group_ptr->command == POFGC_DELETE){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_delete_group_entry(group_ptr, lr);
                }
            }else{
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_BAD_COMMAND, g_recv_xid);
            }

            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        case POFT_COUNTER_MOD:
            counter_ptr = (pof_counter*)(msg_ptr + sizeof(pof_header));
            pof_NtoH_transfer_counter(counter_ptr);

            if(counter_ptr->command == POFCC_CLEAR){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_counter_clear(counter_ptr->counter_id, lr);
                }
            }else if(counter_ptr->command == POFCC_ADD){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_counter_init(counter_ptr->counter_id, lr);
                }
            }else if(counter_ptr->command == POFCC_DELETE){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    ret = poflr_counter_delete(counter_ptr->counter_id, lr);
                }
            }else{
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_BAD_COMMAND, g_recv_xid);
            }

            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        case POFT_COUNTER_REQUEST:
            counter_ptr = (pof_counter*)(msg_ptr + sizeof(pof_header));
            pof_NtoH_transfer_counter(counter_ptr);

            HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                ret = poflr_get_counter_value(counter_ptr->counter_id, lr);
                POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            }
            break;

        case POFT_QUERYALL_REQUEST:
            queryall_ptr = (struct pof_queryall_request *)(msg_ptr + sizeof(pof_header));
            pof_HtoN_transfer_queryall_request(queryall_ptr);

            if(queryall_ptr->slotID == POFSID_ALL){
                HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
                    /* Query all resource on all slots. TODO */
                    ret = poflr_reply_queryall(lr);
                    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
                }
            }else{
                if((lr = pofdp_get_local_resource(queryall_ptr->slotID, dp)) == NULL){
                    POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_INVALID_SLOT_ID, g_recv_xid);
                }
                /* Query all resource on one slots. TODO */
                ret = poflr_reply_queryall(lr);
                POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            }

            if(POF_OK != pofec_reply_msg(POFT_QUERYALL_FIN, g_recv_xid, 0, NULL)){
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
            }
            break;
            
        default:
            POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_REQUEST, POFBRC_BAD_TYPE, g_recv_xid);
            break;
    }
    return ret;
}
