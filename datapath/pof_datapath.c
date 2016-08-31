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
#include "../include/pof_hmap.h"
#include "../include/pof_memory.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

/* Task id. */
task_t g_pofdp_detect_port_task_id = 0;

static uint32_t pofdp_forward(POFDP_ARG, struct pof_instruction *first_ins);
static uint32_t pofdp_recv_raw_task(void *arg_ptr);

static uint32_t 
init_packet_metadata(struct pofdp_packet *dpp, struct pofdp_metadata *metadata, size_t len)
{
	if(len < sizeof *metadata){
		POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_METADATA_LEN_ERROR);
	}

	dpp->metadata_len = len;
	dpp->metadata = metadata;

	memset(metadata, 0, len);
    POF_PACKET_REL_LEN_SET(dpp, dpp->ori_len);
	metadata->port_id = dpp->ori_port_id;

	return POF_OK;
}

static void
fill_localresource_param(struct pof_local_resource *lr, const struct pof_param *param)
{
    uint32_t i;
    lr->portNumMax = param->portNumMax;
    lr->portFlag = param->portFlag;
    for(i=0; i<POF_MAX_TABLE_TYPE; i++){
        lr->tableNumMaxEachType[i] = param->tableNumMaxEachType[i];
    }
    lr->tableSizeMax = param->tableSizeMax;
    lr->groupNumMax = param->groupNumMax;
    lr->meterNumMax = param->meterNumMax;
    lr->counterNumMax = param->counterNumMax;
    return;
}

/* Initialize all slots local resource and the slots map. */
uint32_t 
pofdp_slot_init(struct pof_datapath *dp) 
{
    uint32_t i, ret, slotID = POF_SLOT_ID_BASE;
    struct pof_local_resource *lr = NULL;

    dp->slotMap = hmap_create(dp->slotMax);
    for(i=0; i<dp->slotNum; i++){
        POF_MALLOC_SAFE_RETURN(lr, 1, POF_ERROR);
        lr->slotID = slotID;
        lr->slotNode.hash = hmap_hashForLinear(slotID);
        hmap_nodeInsert(dp->slotMap, &lr->slotNode);
        fill_localresource_param(lr, &dp->param);
    
        /* Initialize the resource. */
        ret = pof_localresource_init(lr);
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

        slotID ++;
    }
}

struct pof_local_resource *
pofdp_get_local_resource(uint16_t slot, const struct pof_datapath *dp) 
{
    struct pof_local_resource *lr, *ptr;
    return HMAP_STRUCT_GET(lr, slotNode, \
            hmap_hashForLinear(slot), dp->slotMap, ptr);
}

/***********************************************************************
 * Initial the datapath module
 * Form:     pof_datapath_init(struct pof_datapath *dp)
 * Input:    dp
 * Return:   POF_OK or Error code
 * Discribe: This function initial the datapath module, creating the send
 *           and receive queues, and creating the necessary tasks. The tasks
 *           include datapath task, send task, and receive tasks which is
 *           corresponding to the local physical ports.
 ***********************************************************************/
uint32_t pof_datapath_init(struct pof_datapath *dp){
    struct portInfo *port, *next;
    struct pof_local_resource *lr, *lrNext;
    uint32_t i, ret;

    /* Create task to receive raw packet. */
    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        HMAP_NODES_IN_STRUCT_TRAVERSE(port, next, pofIndexNode, lr->portPofIndexMap){
            if(port->taskID == POF_INVALID_TASKID){
                if(pofdp_create_port_listen_task(port) != POF_OK){
                    POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TASK_CREATE_FAIL);
                }
            }
        }
    }

    /* Create task to detect the ports. */
    ret = pofbf_task_create(NULL, (void *)poflr_port_detect_task, &g_pofdp_detect_port_task_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    return POF_OK;
}

uint32_t 
pofdp_create_port_listen_task(struct portInfo *port)
{
	uint32_t ret = POF_OK;
    task_t tid;

	ret = pofbf_task_create(port, (void *)pofdp_recv_raw_task, &port->taskID);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    POF_DEBUG_CPRINT_FL(1,BLUE,"Port %s: Start recv_raw task!", port->name);

	return POF_OK;
}

static void 
set_goto_first_table_instruction(struct pof_instruction *p)
{
	struct pof_instruction_goto_table *pigt = \
			(struct pof_instruction_goto_table *)p->instruction_data;

	p->type = POFIT_GOTO_TABLE;
	p->len = sizeof(pigt);
	pigt->next_table_id = POFDP_FIRST_TABLE_ID;
	return;
}


/***********************************************************************
 * Forward function
 * Form:     static uint32_t pofdp_forward(POFDP_ARG, \
 *                                  struct pof_instruction *first_ins)
 * Input:    dpp, dp, first_ins
 * Return:   POF_OK or Error code
 * Discribe: This function forwards the packet between the flow tables.
 *           The new packet will be send into the MM0 table, which is
 *           head flow table. Then according to the matched flow entry,
 *           the packet will be forwarded between the other flow tables
 *           or execute the instruction and action corresponding to the
 *           matched flow entry.
 ***********************************************************************/
static uint32_t pofdp_forward(POFDP_ARG, struct pof_instruction *first_ins)
{
	uint8_t metadata[POFDP_METADATA_MAX_LEN] = {0};
	uint32_t ret;

	POF_DEBUG_CPRINT(1,BLUE,"\n");
	POF_DEBUG_CPRINT_FL(1,BLUE,"Receive a raw packet! len_B = %d, port id = %u", \
			dpp->ori_len, dpp->ori_port_id);
	POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->packetBuf,dpp->left_len,"Input packet data is ");

	/* Initialize the metadata. */
	ret = init_packet_metadata(dpp, (struct pofdp_metadata *)metadata, sizeof(metadata));
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	/* Set the first instruction to the Datapath packet. */
	dpp->ins = first_ins;
	dpp->ins_todo_num = 1;

	ret = pofdp_instruction_execute(dpp, lr);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    return POF_OK;
}

/***********************************************************************
 * The task function of receive task
 * Form:     static void pofdp_recv_raw_task(void *arg_ptr)
 * Input:    port infomation
 * Output:   NONE
 * Return:   VOID
 * Discribe: This is the task function of receive task, which is infinite
 *           loop running. It receives RAW packet by binding socket to the
 *           local physical net port spicified in the port infomation.
 *           After filtering, the packet data and port infomation will be
 *           assembled with format of struct pofdp_packet, and be send
 *           into the receive queue. The only parameter arg_ptr is the
 *           pointer of the local physical net port infomation which has
 *           been assembled with format of struct pof_port.
 * NOTE:     This task will be terminated if any ERRORs occur.
 *           If the openflow function of this physical port is disable,
 *           it will be still loop running but nothing will be received.
 ***********************************************************************/
static uint32_t pofdp_recv_raw_task(void *arg_ptr){
    struct portInfo *port_ptr = (struct portInfo *)arg_ptr;
    struct pof_datapath *dp = &g_dp;
    struct pof_local_resource *lr = NULL;
    struct pofdp_packet dpp[1] = {0};
    struct pof_instruction first_ins[1] = {0};
    struct   sockaddr_ll sockadr = {0}, from = {0};
    uint32_t from_len = sizeof(struct sockaddr_ll), len_B, ret;
    int      sockRecv, sockSend;

    if((lr = pofdp_get_local_resource(port_ptr->slotID, dp)) == NULL){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_INVALID_SLOT_ID, g_upward_xid++);
    }

	/* Set GOTO_TABLE instruction to go to the first flow table. */
	set_goto_first_table_instruction(first_ins);

    /* Create socket, and bind it to the specific port. */
    if((sockRecv = socket(AF_PACKET, SOCK_RAW, POF_HTONS(ETH_P_ALL))) == -1){
        POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE, g_upward_xid++);
        /* Delay 0.1s to send error message upward to the Controller. */
        pofbf_task_delay(100);
        terminate_handler();
    }

    /* Create socket, and bind it to the specific port. */
    if((sockSend = socket(AF_PACKET, SOCK_RAW, POF_HTONS(ETH_P_ALL))) == -1){
        POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE, g_upward_xid++);
        /* Delay 0.1s to send error message upward to the Controller. */
        pofbf_task_delay(100);
        terminate_handler();
    }

    sockadr.sll_family = AF_PACKET;
    sockadr.sll_protocol = POF_HTONS(ETH_P_ALL);
    sockadr.sll_ifindex = port_ptr->sysIndex;

    if(bind(sockRecv, (struct sockaddr *)&sockadr, sizeof(struct sockaddr_ll)) != 0){
       POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_BIND_SOCKET_FAILURE, g_upward_xid++);
        /* Delay 0.1s to send error message upward to the Controller. */
        pofbf_task_delay(100);
        terminate_handler();
    }

    /* Receive the raw packet through the specific port. */
    while(1){
		pthread_testcancel();

        /* Initialize the dpp. */
		memset(dpp, 0, sizeof *dpp);
        dpp->packetBuf = &(dpp->buf[POFDP_PACKET_PREBUF_LEN]);
		dpp->sockSend = sockSend;

        /* Receive the raw packet. */
        if((len_B = recvfrom(sockRecv, dpp->packetBuf, POFDP_PACKET_RAW_MAX_LEN, 0, \
                        (struct sockaddr *)&from, &from_len)) <=0){
            POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_RECEIVE_MSG_FAILURE, g_upward_xid++);
            continue;
        }

        /* Check whether the OpenFlow-enabled of the port is on or not. */
        if(port_ptr->of_enable == POFE_DISABLE || from.sll_pkttype == PACKET_OUTGOING){
            continue;
        }

        /* Check the packet length. */
        if(len_B > POF_MTU_LENGTH){
            POF_DEBUG_CPRINT_FL(1,RED,"The packet received is longer than MTU. DROP!");
            continue;
        }

        /* Filter the received raw packet by some rules. */
        if(dp->filter(dpp->packetBuf, port_ptr, from) != POF_OK){
            continue;
        }
		
        /* Store packet data, length, received port infomation into the message queue. */
        dpp->ori_port_id = port_ptr->pofIndex;
        dpp->ori_len = len_B;
        dpp->left_len = dpp->ori_len;
        dpp->buf_offset = dpp->packetBuf;

        dpp->dp = dp;

		/* Check whether the first flow table exist. */
		if(!(poflr_get_table_with_ID(POFDP_FIRST_TABLE_ID, lr))){
			POF_DEBUG_CPRINT_FL(1,RED,"Received a packet, but the first flow table does NOT exist.");
			continue;
		}

        /* Forward the packet. */
        ret = pofdp_forward(dpp, lr, first_ins);
        POF_CHECK_RETVALUE_NO_RETURN_NO_UPWARD(ret);

		dp->pktCount ++;
        POF_DEBUG_CPRINT_FL(1,GREEN,"one packet_raw has been processed!\n");
    }

    close(sockRecv);
    close(sockSend);
    return POF_OK;
}

/***********************************************************************
 * The task function of send task
 * Form:     send_raw(struct pofdp_packet *dpp, struct pof_local_resource *lr)
 * Input:    NONE
 * Output:   NONE
 * Return:   VOID
 * Discribe: This is the task function of send task, which is infinite
 *           loop running. It reads the send queue to get the packet data
 *           and the sending port infomation. Then it sends the packet
 *           out by binding the socket to the local physical net port
 *           spicified in the port infomation.
 * NOTE:     This task will be terminated if any ERRORs occur.
 ***********************************************************************/
static uint32_t 
send_raw(const struct pofdp_packet *dpp, const struct pof_local_resource *lr)
{
    struct portInfo *port = NULL;
    struct   sockaddr_ll sll = {0};
    //int      sock = dpp->sockSend;
    uint32_t sysIndex = 0;

    if((port = poflr_get_port_with_pofindex(dpp->output_port_id, lr)) == NULL){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_PTR_NULL);
    }

    /* Send the packet data out through the port. */
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = port->sysIndex;
    sll.sll_protocol = POF_HTONS(ETH_P_ALL);

    if(sendto(port->queue_fd[1], dpp->buf_out, dpp->output_whole_len, 0, (struct sockaddr *)&sll, sizeof(sll)) == -1){
    //if(sendto(sock, dpp->buf_out, dpp->output_whole_len, 0, (struct sockaddr *)&sll, sizeof(sll)) == -1){
    	printf("here error!!\n");
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_SEND_MSG_FAILURE, g_upward_xid++);
    }

    return POF_OK;
}

/***********************************************************************
 * Send packet out function
 * Form:     uint32_t pofdp_send_raw(dpp)
 * Input:    dpp
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function send the packet data out through the port
 *           corresponding the port_id. The length of packet data is len.
 *           It assembles the packet data, the packet length
 *           and the output port id with format of struct pofdp_packet,
 *           and write it to the send queue. Caller should make sure that
 *           output_packet_offset plus output_packet_len is less than the
 *           whole packet_len, and that output_metadata_offset plus 
 *           output_metadata_len is less than the whole metadata_len.
 ***********************************************************************/
uint32_t pofdp_send_raw(struct pofdp_packet *dpp, const struct pof_local_resource *lr){
	/* Copy metadata to output buffer. */
    pofbf_copy_bit((uint8_t *)dpp->metadata, dpp->buf_out, dpp->output_metadata_offset, \
			dpp->output_metadata_len * POF_BITNUM_IN_BYTE);
	/* Copy packet to output buffer right behind metadata. */
    memcpy(dpp->buf_out + dpp->output_metadata_len, dpp->output_packet_buf + dpp->output_packet_offset, \
            dpp->output_packet_len);

    POF_DEBUG_CPRINT_FL(1,GREEN,"One packet is about to be sent out! port_id = %d, slot_id = %u, packet_len = %u, metadata_len = %u, total_len = %u", \
			            dpp->output_port_id, dpp->output_slot_id, dpp->output_packet_len, \
                        dpp->output_metadata_len, dpp->output_whole_len);
    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->output_packet_buf + dpp->output_packet_offset, dpp->output_packet_len, \
			"The packet is ");
    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf_out,dpp->output_metadata_len,"The metatada is ");
    POF_DEBUG_CPRINT_FL_0X(1,BLUE,dpp->buf_out, dpp->output_whole_len,"The whole output packet is ");

    /* Check the packet lenght. */
    if(dpp->output_whole_len > POF_MTU_LENGTH){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    if(send_raw(dpp, lr) != POF_OK){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_SEND_MSG_FAILURE);
    }

    return POF_OK;
}

/***********************************************************************
 * Send packet upward to the Controller
 * Form:     uint32_t pofdp_send_packet_in_to_controller(uint16_t len, \
                                                         uint8_t reason,     \
                                                         uint8_t table_id,   \
                                                         uint32_t device_id, \
                                                         uint8_t port_id,    \
                                                         uint16_t slotID,    \
 *                                                       uint8_t *packet)
 * Input:    packet length, upward reason, current table id, cookie,
 *           device id, packet data
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function send the packet data upward to the controller.
 *           It assembles the packet data, length, reason, table id,
 *           cookie and device id with format of struct pof_packet_in,
 *           and encapsulate to a new openflow packet. Then the new packet
 *           will be send to the mpu module in order to send upward to the
 *           Controller.
 ***********************************************************************/
uint32_t pofdp_send_packet_in_to_controller(uint16_t len,       \
                                            uint8_t reason,     \
                                            uint8_t table_id,   \
                                            uint32_t device_id, \
                                            uint8_t port_id,    \
                                            uint16_t slotID,    \
                                            uint8_t *packet)
{
    pof_packet_in packetin = {0};
    uint32_t      packet_in_len;

    /* The length of the packet in data upward to the Controller is the real length
     * instead of the max length of the packet_in. */
    packet_in_len = sizeof(pof_packet_in) - POF_PACKET_IN_MAX_LENGTH + len;

    /* Check the packet length. */
    if(len > POF_PACKET_IN_MAX_LENGTH){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    packetin.buffer_id = 0xffffffff;
    packetin.total_len = len;
    packetin.reason = reason;
    packetin.table_id = table_id;
    packetin.cookie = 0;
    packetin.device_id = device_id;
//#ifdef POF_MULTIPLE_SLOTS
    packetin.slotID = POF_SLOT_ID_BASE;
    packetin.port_id = port_id;
//#endif // POF_MULTIPLE_SLOTS


    memcpy(packetin.data, packet, len);

    pof_NtoH_transfer_packet_in(&packetin);

    if(POF_OK != pofec_reply_msg(POFT_PACKET_IN, g_upward_xid++, packet_in_len, (uint8_t *)&packetin)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
    }

    return POF_OK;
}

static uint32_t pofdp_promisc(uint8_t *packet, struct portInfo *port_ptr, struct sockaddr_ll sll){
    uint32_t *daddr, ret = POF_OK;
    uint16_t *ether_type;
    uint8_t  *ip_protocol, *eth_daddr;

#ifdef POF_RECVRAW_ANYPACKET
	return POF_OK;
#endif // POF_RECVRAW_ANYPACKET

#ifdef POF_RECVRAW_TESTPACKET
    /* Any packet which source MAC address is equal to the match_buf
     * will be received. */
    uint8_t match_buf[POF_ETH_ALEN] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06
    };
    if(memcmp((uint8_t *)packet + POF_ETH_ALEN, (uint8_t *)match_buf, POF_ETH_ALEN) == 0){
        return POF_OK;
    }else{
		return POF_ERROR;
	}
#endif // POF_TEST_ON

#ifdef POF_RECVRAW_ETHTYPE_IP
    /* The ether type have to be IP. */
    ether_type = (uint16_t *)(packet + 2 * POF_ETH_ALEN);
    if(POF_NTOHS(*ether_type) != ETHERTYPE_IP){
//    if(POF_NTOHS(*ether_type) != 0x0888){
        return POF_ERROR;
    }
#endif // POF_RECVRAW_ETHTYPE_IP

#ifdef POF_RECVRAW_IPPROTO_ICMP
    /* The IP protocol have to be ICMP. */
    ip_protocol = packet + (2*POF_ETH_ALEN+2) + (4*2+1);
    if(*ip_protocol != IPPROTO_ICMP){
        return POF_ERROR;
    }
#endif // POF_RECVRAW_IPPROTO_ICMP

//    return POF_OK;
    return POF_OK;
}

/***********************************************************************
 * NONE promisc mode packet filter
 * Form:     static uint32_t pofdp_no_promisc(uint8_t *packet, pof_port *port_ptr, struct sockaddr *sll)
 * Input:    packet data, port infomation
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function filter the RAW packet received by the local
 *           physical net port.
 ***********************************************************************/
static uint32_t pofdp_no_promisc(uint8_t *packet, struct portInfo *port_ptr, struct sockaddr_ll sll){
    uint32_t *daddr, ret = POF_OK;
    uint16_t *ether_type;
    uint8_t  *ip_protocol, *eth_daddr;
    uint8_t broadcast[POF_ETH_ALEN] = {
        0xff,0xff,0xff,0xff,0xff,0xff
    };

#ifdef POF_RECVRAW_DHWADDR_LOCAL
    eth_daddr = (uint8_t *)packet;
    if(memcmp(eth_daddr, port_ptr->hw_addr, POF_ETH_ALEN) != 0 && \
            memcmp(eth_daddr, broadcast, POF_ETH_ALEN) != 0){
		return POF_ERROR;
    }
#endif // POF_RECVRAW_DHWADDR_LOCAL

    if(sll.sll_pkttype == PACKET_OTHERHOST){
        return POF_ERROR;
    }

    return POF_OK;
}

/* Global datapath structure. */
struct pof_datapath g_dp = {
    pofdp_no_promisc,
    pofdp_promisc,
#ifdef POF_PROMISC_ON
    pofdp_promisc,
#else // POF_PROMISC_ON
    pofdp_no_promisc,
#endif // POF_PROMISC_ON
    {
        /* Ports. */
        POFLR_DEVICE_PORT_NUM_MAX,0,
        /* Tables. */
        {POFLR_MM_TBL_NUM, POFLR_LPM_TBL_NUM, 
         POFLR_EM_TBL_NUM, POFLR_DT_TBL_NUM},POFLR_FLOW_TABLE_SIZE,
        /* Group, Meter, Counter. */
        POFLR_GROUP_NUMBER, POFLR_METER_NUMBER, POFLR_COUNTER_NUMBER,
    },
    /* Slot Hash Map. */
    NULL, POF_SLOT_NUM, POF_SLOT_MAX,
    /* TCP listen. */
    POFDP_TCP_LISTEN_IP,
    POFDP_TCP_LISTEN_PORT,
};
