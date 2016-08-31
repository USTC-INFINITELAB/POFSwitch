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

#include "pof_common.h"
#include "pof_type.h"
#include "pof_global.h"
#include "pof_conn.h"
#include "pof_log_print.h"
#include "pof_local_resource.h"
#include "pof_datapath.h"
#include "pof_byte_transfer.h"
#include "pof_switch_listen.h"
#include <sys/time.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/msg.h>
#include <signal.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

/* Controller ip. */
char pofsc_controller_ip_addr[POF_IP_ADDRESS_STRING_LEN] = POF_CONTROLLER_IP_ADDR;

/* Controller port. */
uint16_t pofsc_controller_port = POF_CONTROLLER_PORT_NUM;

/* The max retry time of cnnection. */
uint32_t pofsc_conn_max_retry = POF_CONNECTION_MAX_RETRY_TIME;

/* The retry interval of cnnection if connect fails. */
uint32_t pofsc_conn_retry_interval = POF_CONNECTION_RETRY_INTERVAL;

/* Description of device connection. */
volatile pofsc_dev_conn_desc pofsc_conn_desc;

/* Openflow action id. */
uint32_t g_upward_xid = POF_INITIAL_XID;

/* Error message. */
pof_error pofsc_protocol_error;

/* Task id. */
task_t pofsc_main_task_id = 0;
task_t pofsc_send_task_id = 0;
task_t pofsc_listen_task_id = 0;

/* Message queue. */
uint32_t pofsc_send_q_id = POF_INVALID_QUEUEID;

/* Timer. */
uint32_t pofsc_echo_interval = POF_ECHO_INTERVAL;
uint32_t pofsc_echo_timer_id = 0;

/* Openflow connect state string. */
char *pofsc_state_str[] = {
    "POFCS_CHANNEL_INVALID",
    "POFCS_CHANNEL_CONNECTING",
    "POFCS_CHANNEL_CONNECTED",
    "POFCS_HELLO",
    "POFCS_REQUEST_FEATURE",
    "POFCS_SET_CONFIG",
    "POFCS_REQUEST_GET_CONFIG",
    "POFCS_CHANNEL_RUN",
};

/* Local functions. */
static uint32_t pofsc_main_task(void *arg_ptr);
static uint32_t pofsc_init();
static uint32_t pofsc_destroy(struct pof_datapath *dp);
static uint32_t pofsc_send_msg_task(void *arg_ptr);
static uint32_t pofsc_echo_timer(uint32_t timer_id, int arg);
static uint32_t pofsc_set_conn_attr(const char *controller_ip, uint16_t port, uint32_t retry_max, uint32_t retry_interval);
static uint32_t pofsc_create_socket(int *socket_fd_ptr);
static uint32_t pofsc_connect(int socket_fd, char *server_ip, uint16_t port, struct pof_datapath *dp);
static uint32_t pofsc_recv(int socket_fd, char* buf,  int buflen, int* plen, struct pof_datapath *dp);
static uint32_t pofsc_send(int socket_fd, char* buf, int len, struct pof_datapath *dp);
static uint32_t pofsc_run_process(char *message, uint16_t len, struct pof_datapath *dp);
static uint32_t pofsc_build_header(pof_header *header, uint8_t type, uint16_t len, uint32_t xid);
static uint32_t pofsc_set_error(uint16_t type, uint16_t code);
static uint32_t pofsc_build_error_msg(char *message, uint16_t *len_p);
static uint32_t pofsc_wait_exit(struct pof_datapath *dp);
static uint32_t pofsc_performance_after_ctrl_disconn(struct pof_datapath *dp);

int main(int argc, char *argv[]){
    uint32_t ret = POF_OK;
    struct pof_datapath *dp = &g_dp;

    /* Initialize the config of the Soft Switch. */
    ret = pof_set_init_config(argc, argv, dp);
    POF_CHECK_RETVALUE_TERMINATE(ret);

	/* Check whether the euid is root id. If not, QUIT. */
	ret = pofsc_check_root();
	if(POF_OK != ret){
		exit(0);
	}
    
    POF_DEBUG_CPRINT_FL(1,GREEN,"STATE:");
    poflp_states_print(&g_states);

    /* Initialize the slots map, and all slots resource. */
    ret = pofdp_slot_init(dp);
    POF_CHECK_RETVALUE_TERMINATE(ret);

    /* Start OpenFlow communication module in Soft Switch. */
    ret = pofsc_init();
    POF_CHECK_RETVALUE_TERMINATE(ret);

    /* Delay for finishing the initlization started above. */
    sleep(1);

    /* Start datapath module in Soft Switch. */
    ret = pof_datapath_init(dp);
    POF_CHECK_RETVALUE_TERMINATE(ret);

    /* Let the main task still running. */
    pofsc_wait_exit(dp);
    return ret;
}

/***********************************************************************
 * Start the OpenFlow communication module in Soft Switch.
 * Form:     uint32_t pofsc_init()
 * Input:    NONE
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will start the OpenFlow communication module
 *           in Soft Switch. This module builds the connection and
 *           communication between the Soft Switch and the Controller,
 *           which is runing on the other PC as a server.
 ***********************************************************************/
static uint32_t pofsc_init(){
    /* Set the signal handle function. */
    signal(SIGINT, terminate_handler);
    signal(SIGTERM, terminate_handler);
    signal(SIGPIPE, SIG_IGN);

    /* Set OpenFlow connection attributes. */
    (void)pofsc_set_conn_attr(pofsc_controller_ip_addr, \
                              pofsc_controller_port, \
                              pofsc_conn_max_retry, \
                              pofsc_conn_retry_interval);

    /* Create one message queue for storing messages to be sent to controller. */
    if (POF_OK != pofbf_queue_create(&(pofsc_send_q_id))){
        POF_ERROR_CPRINT_FL("\nCreate message queue, fail and return!");
        return POF_ERROR;
    }

    /* Create connection and state machine task. */
    if (POF_OK != pofbf_task_create(NULL, (void *)pofsc_main_task, &pofsc_main_task_id)){
        POF_ERROR_CPRINT_FL("\nCreate openflow main task, fail and return!");
        return POF_ERROR;
    }
    POF_DEBUG_CPRINT_FL(1,GREEN,">>Startup openflow task!");

    /* Create one task for sending  message to controller asynchronously. */
    if (POF_OK != pofbf_task_create(NULL, (void *)pofsc_send_msg_task, &pofsc_send_task_id)){
        POF_ERROR_CPRINT_FL("\nCreate openflow main task, fail and return!");
        return POF_ERROR;
    }
    POF_DEBUG_CPRINT_FL(1,GREEN,">>Startup task for sending message!");

    /* Create one task for listening pofsctrl. */
    if (POF_OK != pofbf_task_create(NULL, (void *)pof_switch_listen_task, &pofsc_listen_task_id)){
        POF_ERROR_CPRINT_FL("\nCreate switch listen task, fail and return!");
        return POF_ERROR;
    }
    POF_DEBUG_CPRINT_FL(1,GREEN,">>Startup task for listening pofsctrl!");

    /* Create one timer for sending echo message. */
    if (POF_OK != pofbf_timer_create( 1000, pofsc_echo_interval, (void *)pofsc_echo_timer, &pofsc_echo_timer_id)){
        POF_ERROR_CPRINT_FL("\nCreate echo timer, fail and return!");
        return POF_ERROR;
    }

    return POF_OK;
}

/***********************************************************************
 * The task function for connection and state machine task.
 * Form:     void pofsc_main_task(void *arg_ptr)
 * Input:    NONE
 * Output:   NONE
 * Return:   VOID
 * Discribe: This task function keeps running the state machine of Soft
 *           Switch. The Soft Switch always works on one of states.
 *           Before the POFCS_CHANNEL_RUN state, this function
 *           builds the connection with the Cntroller by sending and
 *           receiving the "Hello" packet, replying the requests from the
 *           Controller, and so on. During the POFCS_CHANNEL_RUN state,
 *           it receive OpenFlow messages from the Controller and send
 *           them to the other modules to handle.
 ***********************************************************************/
static uint32_t pofsc_main_task(void *arg_ptr){
    pofsc_dev_conn_desc *conn_desc_ptr = (pofsc_dev_conn_desc *)&pofsc_conn_desc;
    pof_header          *head_ptr, head;
    int total_len = 0, tmp_len, left_len, rcv_len = 0, process_len = 0, packet_len = 0;
    int socket_fd;
    uint32_t ret;
    struct pof_datapath *dp = &g_dp;
    struct pof_local_resource *lr, *lrNext;

    /* Clear error record. */
    pofsc_protocol_error.type = 0xffff;

    /* State machine of the control module in Soft Switch. */
    while(1)
    {
        if(conn_desc_ptr->conn_status.state != POFCS_CHANNEL_RUN && !conn_desc_ptr->conn_retry_count){
            POF_DEBUG_CPRINT_FL(1,BLUE, ">>Openflow Channel State: %s", pofsc_state_str[conn_desc_ptr->conn_status.state]);
        }

        switch(conn_desc_ptr->conn_status.state){
            case POFCS_CHANNEL_INVALID:

                /* Create openflow channel socket. */
                ret = pofsc_create_socket(&socket_fd);
                if(ret == POF_OK){
                    conn_desc_ptr->sfd = socket_fd;
                    conn_desc_ptr->conn_status.state = POFCS_CHANNEL_CONNECTING;
                }else{
                    POF_ERROR_CPRINT_FL(">>Create socket FAIL!");
                    terminate_handler();
                }
                break;

            case POFCS_CHANNEL_CONNECTING:
                /* Connect controller. */
				if(!conn_desc_ptr->conn_retry_count){
					POF_DEBUG_CPRINT(1,GREEN,">>Connecting to POFController...\n");
				}
                ret = pofsc_connect(conn_desc_ptr->sfd, conn_desc_ptr->controller_ip, \
                        conn_desc_ptr->controller_port, dp);
                if(ret == POF_OK){
                    POF_DEBUG_CPRINT_FL(1,GREEN,">>Connect to controler SUC! %s: %u", \
                                        pofsc_controller_ip_addr, POF_CONTROLLER_PORT_NUM);
                    conn_desc_ptr->conn_status.state = POFCS_CHANNEL_CONNECTED;
					conn_desc_ptr->conn_retry_count = 0;
                }else{
					if(!conn_desc_ptr->conn_retry_count){
						POF_DEBUG_CPRINT_FL(1,RED,">>Connect to controler FAIL!");
					}
                    /* Delay several seconds. */
                    pofbf_task_delay(conn_desc_ptr->conn_retry_interval * 1000);
                    conn_desc_ptr->conn_retry_count++;
                    conn_desc_ptr->conn_status.last_error = (uint8_t)(POF_CONNECT_SERVER_FAILURE); /**/
                    conn_desc_ptr->sfd = 0;
                    conn_desc_ptr->conn_status.state = POFCS_CHANNEL_INVALID;
                }
                break;

            case POFCS_CHANNEL_CONNECTED:
                /* Send hello to controller. Hello message has no body. */
                pofsc_build_header(&head, \
                                   POFT_HELLO, \
                                   sizeof(pof_header), \
                                   g_upward_xid++);
                /* send hello message. */
                ret = pofsc_send(conn_desc_ptr->sfd, (char*)&head, sizeof(pof_header), dp);
                if(ret == POF_OK){
                    conn_desc_ptr->conn_status.state = POFCS_HELLO;
                }else{
                    POF_ERROR_CPRINT_FL("Send HELLO FAIL!");
                }

                break;

            case POFCS_HELLO:
                /* Receive hello from controller. */
                total_len = 0;
                left_len = 0;
                rcv_len = 0;
                process_len = 0;
                ret = pofsc_recv(conn_desc_ptr->sfd, conn_desc_ptr->recv_buf , \
                        POF_RECV_BUF_MAX_SIZE, &total_len, dp);
                if(ret == POF_OK){
                    POF_DEBUG_CPRINT_FL(1,GREEN,">>Recevie HELLO packet SUC!");
                    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
					    poflr_clear_resource(lr);
                    }
                    conn_desc_ptr->conn_status.state = POFCS_REQUEST_FEATURE;
                }else{
                    POF_ERROR_CPRINT_FL("Recv HELLO FAILE!");
                    break;
                }
                rcv_len += total_len;

                /* Parse. */
                head_ptr = (pof_header *)conn_desc_ptr->recv_buf;
                while(total_len < POF_NTOHS(head_ptr->length)){
                    ret = pofsc_recv(conn_desc_ptr->sfd, conn_desc_ptr->recv_buf  + rcv_len, \
                            POF_RECV_BUF_MAX_SIZE -rcv_len,  &tmp_len, dp);
                    if(ret != POF_OK){
                        POF_ERROR_CPRINT_FL("Recv HELLO FAILE!");
                        break;
                    }

                    total_len += tmp_len;
                    rcv_len += tmp_len;
                }

                if(conn_desc_ptr->conn_status.state == POFCS_CHANNEL_INVALID){
                    break;
                }

                /* Check any error. */
                if(head_ptr->version > POF_VERSION){
                    POF_ERROR_CPRINT_FL("Version of recv-packet is higher than support!");
                    close(conn_desc_ptr->sfd);
                    conn_desc_ptr->conn_status.state = POFCS_CHANNEL_INVALID;
                }else if(head_ptr->type != POFT_HELLO){
                    POF_ERROR_CPRINT_FL("Type of recv-packet is not HELLO, which we want to recv!");
                    close(conn_desc_ptr->sfd);
                    conn_desc_ptr->conn_status.state = POFCS_CHANNEL_INVALID;
                }

                process_len += POF_NTOHS(head_ptr->length);
                left_len = rcv_len - process_len;
                if(left_len == 0){
                    rcv_len = 0;
                    process_len = 0;
                }
                break;

            case POFCS_REQUEST_FEATURE:
                /* Wait to receive feature request from controller. */
                head_ptr = (pof_header *)(conn_desc_ptr->recv_buf  + process_len);
                if(!((left_len >= sizeof(pof_header))&&(left_len >= POF_NTOHS(head_ptr->length)))){
                    ret = pofsc_recv(conn_desc_ptr->sfd, (conn_desc_ptr->recv_buf  + rcv_len), \
                            POF_RECV_BUF_MAX_SIZE - rcv_len, &total_len, dp);
                    if(ret == POF_OK){
                        conn_desc_ptr->conn_status.state = POFCS_SET_CONFIG;
                    }else{
                        POF_ERROR_CPRINT_FL("Feature request FAIL!");
                        break;
                    }

                    rcv_len += total_len;
                    total_len += left_len;

                    head_ptr = (pof_header *)(conn_desc_ptr->recv_buf + process_len);
                    while(total_len < POF_NTOHS(head_ptr->length)){
                        ret = pofsc_recv(conn_desc_ptr->sfd, ((conn_desc_ptr->recv_buf  + rcv_len)), \
                                POF_RECV_BUF_MAX_SIZE-rcv_len ,&tmp_len, dp);
                        if(ret != POF_OK){
                            POF_ERROR_CPRINT_FL("Feature request FAIL!");
                            break;
                        }
                        total_len += tmp_len;
                        rcv_len += tmp_len;
                    }
                }

                if(conn_desc_ptr->conn_status.state == POFCS_CHANNEL_INVALID){
                    break;
                }

                head_ptr = (pof_header *)(conn_desc_ptr->recv_buf  + process_len);

                /* Check any error. */
                if(head_ptr->type != POFT_FEATURES_REQUEST){
                    close(conn_desc_ptr->sfd);
                    conn_desc_ptr->conn_status.state = POFCS_CHANNEL_INVALID;
                    break;
                }

                POF_DEBUG_CPRINT_FL(1,GREEN,">>Recevie FEATURE_REQUEST packet SUC!");
                conn_desc_ptr->conn_status.state = POFCS_SET_CONFIG;
                packet_len = POF_NTOHS(head_ptr->length);

                ret = pof_parse_msg_from_controller(conn_desc_ptr->recv_buf + process_len, dp);

                if(ret != POF_OK){
                    POF_ERROR_CPRINT_FL("Features request FAIL!");
                    terminate_handler();
                    break;
                }

                process_len += packet_len;
                left_len = rcv_len - process_len;
                if(left_len == 0){
                    rcv_len = 0;
                    process_len = 0;
                }

                break;

            case POFCS_SET_CONFIG:
                /* Receive set_config message from controller. */
                head_ptr = (pof_header *)(conn_desc_ptr->recv_buf  + process_len);
                if(!((left_len >= sizeof(pof_header))&&(left_len >= POF_NTOHS(head_ptr->length)))){
                    ret = pofsc_recv(conn_desc_ptr->sfd, (conn_desc_ptr->recv_buf  + rcv_len), \
                            POF_RECV_BUF_MAX_SIZE - rcv_len, &total_len, dp);
                    if(ret == POF_OK){
                        conn_desc_ptr->conn_status.state = POFCS_REQUEST_GET_CONFIG;
                    }else{
                        POF_ERROR_CPRINT_FL("Set config FAIL!");
                        break;
                    }

                    rcv_len += total_len;
                    total_len += left_len;

                    head_ptr = (pof_header *)(conn_desc_ptr->recv_buf + process_len);
                    while(total_len < POF_NTOHS(head_ptr->length)){
                        ret = pofsc_recv(conn_desc_ptr->sfd, ((conn_desc_ptr->recv_buf  + rcv_len)), \
                                POF_RECV_BUF_MAX_SIZE-rcv_len ,&tmp_len, dp);
                        if(ret != POF_OK){
                            POF_ERROR_CPRINT_FL("Set config FAIL!");
                            break;
                        }
                        total_len += tmp_len;
                        rcv_len += tmp_len;
                    }
                }

                if(conn_desc_ptr->conn_status.state == POFCS_CHANNEL_INVALID){
                    break;
                }

                head_ptr = (pof_header *)(conn_desc_ptr->recv_buf  + process_len);

                /* Check any error. */
                if(head_ptr->version > POF_VERSION){
                    POF_ERROR_CPRINT_FL("Version of recv-packet is higher than support!");
                    POF_ERROR_CPRINT_FL("Set config FAIL!");
                    close(conn_desc_ptr->sfd);
                    conn_desc_ptr->conn_status.state = POFCS_CHANNEL_INVALID;
                    break;
                }else if(head_ptr->type != POFT_SET_CONFIG){
                    POF_ERROR_CPRINT_FL("Type of recv-packet is not SET_CONFIG, which we want to recv!");
                    POF_ERROR_CPRINT_FL("Set config FAIL!");
                    close(conn_desc_ptr->sfd);
                    conn_desc_ptr->conn_status.state = POFCS_CHANNEL_INVALID;
                    break;
                }

                POF_DEBUG_CPRINT_FL(1,BLUE,">>Recevie SET_CONFIG packet SUC!");
                conn_desc_ptr->conn_status.state = POFCS_REQUEST_GET_CONFIG;
                packet_len = POF_NTOHS(head_ptr->length);

                ret = pof_parse_msg_from_controller(conn_desc_ptr->recv_buf + process_len, dp);

                if(ret != POF_OK){
                    POF_ERROR_CPRINT_FL("Set config FAIL!");
                    terminate_handler();
                    break;
                }

                process_len += packet_len;
                left_len = rcv_len - process_len;

                if(left_len == 0){
                    rcv_len = 0;
                    process_len = 0;
                }
                break;

            case POFCS_REQUEST_GET_CONFIG:
                /* Wait to receive feature request from controller. */
                head_ptr = (pof_header *)(conn_desc_ptr->recv_buf  + process_len);
                if(!((left_len >= sizeof(pof_header)) && (left_len >= POF_NTOHS(head_ptr->length)))){
                    ret = pofsc_recv(conn_desc_ptr->sfd, (conn_desc_ptr->recv_buf  + rcv_len), \
                            POF_RECV_BUF_MAX_SIZE - rcv_len, &total_len, dp);
                    if(ret == POF_OK){
                        conn_desc_ptr->conn_status.state = POFCS_CHANNEL_RUN;
                    }else{
                        POF_ERROR_CPRINT_FL("Get config FAIL!");
                        break;
                    }

                    rcv_len += total_len;
                    total_len += left_len;

                    head_ptr = (pof_header *)(conn_desc_ptr->recv_buf + process_len);
                    while(total_len < POF_NTOHS(head_ptr->length)){
                        ret = pofsc_recv(conn_desc_ptr->sfd, ((conn_desc_ptr->recv_buf  + rcv_len)), \
                                POF_RECV_BUF_MAX_SIZE-rcv_len ,&tmp_len, dp);
                        if(ret != POF_OK){
                            POF_ERROR_CPRINT_FL("Get config FAIL!");
                            break;
                        }

                        total_len += tmp_len;
                        rcv_len += tmp_len;
                    }
                }

                if(conn_desc_ptr->conn_status.state == POFCS_CHANNEL_INVALID){
                    break;
                }

                head_ptr = (pof_header *)(conn_desc_ptr->recv_buf  + process_len);
                /* Check any error. */
                if(head_ptr->type != POFT_GET_CONFIG_REQUEST){
                    POF_ERROR_CPRINT_FL("Get config FAIL!");
                    close(conn_desc_ptr->sfd);
                    conn_desc_ptr->conn_status.state = POFCS_CHANNEL_INVALID;
                    break;
                }

                POF_DEBUG_CPRINT_FL(1,GREEN,">>Recevie GET_CONFIG_REQUEST packet SUC!");
                packet_len = POF_NTOHS(head_ptr->length);

                ret = pof_parse_msg_from_controller(conn_desc_ptr->recv_buf + process_len, dp);
                if(ret != POF_OK){
                    POF_ERROR_CPRINT_FL("Get config FAIL!");
                    terminate_handler();
                    break;
                }

                process_len += packet_len;
                left_len = rcv_len - process_len;

                if(left_len == 0){
                    rcv_len = 0;
                    process_len = 0;
                }

				sleep(1);
                conn_desc_ptr->conn_status.state = POFCS_CHANNEL_RUN;
				POF_DEBUG_CPRINT(1,GREEN,">>Connect to POFController successfully!\n");

                break;

            case POFCS_CHANNEL_RUN:
                /* Wait to receive feature request from controller. */
                head_ptr = (pof_header *)(conn_desc_ptr->recv_buf  + process_len);
                if(!((left_len >= sizeof(pof_header))&&(left_len >= POF_NTOHS(head_ptr->length)))){
                    /* Resv_buf has no space, so should move the left data to the head of the buf. */
                    if(POF_RECV_BUF_MAX_SIZE == rcv_len){
                        memcpy(conn_desc_ptr->recv_buf,  conn_desc_ptr->recv_buf  + process_len, left_len);
                        rcv_len = left_len;
                        process_len = 0;
                    }
                    ret = pofsc_recv(conn_desc_ptr->sfd, (conn_desc_ptr->recv_buf  + rcv_len), \
                            POF_RECV_BUF_MAX_SIZE - rcv_len, &total_len, dp);
                    if(ret != POF_OK){
                        break;
                    }

                    rcv_len += total_len;
                    total_len += left_len;

                    head_ptr = (pof_header *)(conn_desc_ptr->recv_buf + process_len);
                    while(total_len < POF_NTOHS(head_ptr->length)){
                        left_len = rcv_len - process_len;
                        /* Resv_buf has no space, so should move the left data to the head of the buf. */
                        if(POF_RECV_BUF_MAX_SIZE == rcv_len){
                            memcpy(conn_desc_ptr->recv_buf,  conn_desc_ptr->recv_buf  + process_len, left_len);
                            rcv_len = left_len;
                            process_len = 0;
                        }

                        ret = pofsc_recv(conn_desc_ptr->sfd, ((conn_desc_ptr->recv_buf  + rcv_len)), \
                                POF_RECV_BUF_MAX_SIZE-rcv_len ,&tmp_len, dp);
                        if(ret != POF_OK){
                            break;
                        }
                    total_len += tmp_len;
                    rcv_len += tmp_len;
                    }
                }

                if(conn_desc_ptr->conn_status.state == POFCS_CHANNEL_INVALID){
                    break;
                }

                head_ptr = (pof_header *)(conn_desc_ptr->recv_buf  + process_len);
                packet_len = POF_NTOHS(head_ptr->length);
                /* Handle the message. Echo messages will be processed here and other messages will be forwarded to LUP. */
                ret = pofsc_run_process(conn_desc_ptr->recv_buf + process_len, packet_len, dp);

                process_len += packet_len;
                left_len = rcv_len - process_len;

                if(left_len == 0){
                    rcv_len = 0;
                    process_len = 0;
                }
                break;

            default:
                conn_desc_ptr->conn_status.last_error = (uint8_t)POF_WRONG_CHANNEL_STATE;
                break;
        }

        /* If any error is detected, reply to controller immediately. */
        if(pofsc_protocol_error.type != 0xffff){
            tmp_len = 0;
            /* Build error message. */
            (void)pofsc_build_error_msg(conn_desc_ptr->send_buf, (uint16_t*)&tmp_len);

            /* Write error message in queue for sending. */
            ret = pofbf_queue_write(pofsc_send_q_id, conn_desc_ptr->send_buf, (uint32_t)tmp_len, POF_WAIT_FOREVER);
            POF_CHECK_RETVALUE_TERMINATE(ret);
        }
    }
    return;
}

/***********************************************************************
 * OpenFlow communication module task for sending message asynchronously.
 * Form:     uint32_t pofsc_send_msg_task(void *arg_ptr)
 * Input:    NONE
 * Output:   NONE
 * Return:   VOID
 * Discribe: Any message is first sent into messaage queue, and the task
 *           always check the message queue for sending. The messages
 *           include two types:
 *           1. Reply to controllers' request.
 *           2. Asynchrous message.
 *           The two types messages are built and sent to queue by two
 *           different tasks.
 ***********************************************************************/
static uint32_t pofsc_send_msg_task(void *arg_ptr){
    pofsc_dev_conn_desc *conn_desc_ptr = (pofsc_dev_conn_desc *)&pofsc_conn_desc;
    pof_header *head_ptr;
    uint32_t   ret;
    struct pof_datapath *dp = &g_dp;

    /* Polling the message queue. If valid, fetch one message and send it to controller. */
    while(1){
        /* Set the pthread cancel point. */
        pthread_testcancel();
        switch(conn_desc_ptr->conn_status.state){
            case POFCS_CHANNEL_INVALID:
            case POFCS_CHANNEL_CONNECTING:
            case POFCS_CHANNEL_CONNECTED:
            case POFCS_HELLO:
                pofbf_task_delay(100);
                break;
            case POFCS_SET_CONFIG:
            case POFCS_REQUEST_FEATURE:
            case POFCS_REQUEST_GET_CONFIG:
            case POFCS_CHANNEL_RUN:
                /* Fetch one message from message queue. */
                ret = pofbf_queue_read(pofsc_send_q_id, conn_desc_ptr->msg_buf, POF_QUEUE_MESSAGE_LEN, POF_WAIT_FOREVER);
                if(ret != POF_OK){
                    pofsc_set_error(POFET_SOFTWARE_FAILED, ret);
                    break;
                }

                /* Send message to server. */
                head_ptr = (pof_header*)conn_desc_ptr->msg_buf;
                ret = pofsc_send(conn_desc_ptr->sfd, conn_desc_ptr->msg_buf, POF_HTONS(head_ptr->length), dp);
                if(ret != POF_OK){
                    /* Return to inalid state. */
                    conn_desc_ptr->conn_status.last_error = (uint8_t)ret;
                    conn_desc_ptr->sfd = 0;
                    conn_desc_ptr->conn_status.state = POFCS_CHANNEL_INVALID;

                    /* Put the message back to queue for sendding next time. */
                    ret = pofbf_queue_write(pofsc_send_q_id, conn_desc_ptr->msg_buf, POF_HTONS(head_ptr->length), POF_WAIT_FOREVER);
                    if(ret != POF_OK){
                        pofsc_set_error(POFET_SOFTWARE_FAILED, ret);
                        break;
                    }
                }
                break;
            default:
                conn_desc_ptr->conn_status.last_error = (uint8_t)POF_WRONG_CHANNEL_STATE;
                break;
        }
    }
    return POF_OK;
}

/***********************************************************************
 * OpenFlow echo timer routine.
 * Form:     void pofsc_echo_timer(uint32_t timer_id, int arg)
 * Input:    NONE
 * Output:   NONE
 * Return:   VOID
 * Discribe: The routine is only used to send echo request and not wait
 *           for echo reply.Echo reply is received and processed by main
 *           task.
 ***********************************************************************/
static uint32_t pofsc_echo_timer(uint32_t timer_id, int arg){
    pofsc_dev_conn_desc *conn_desc_ptr = (pofsc_dev_conn_desc *)&pofsc_conn_desc;
    pof_header head;
    uint16_t len;
    uint32_t ret;

    /* If valid, fetch one message and send it to controller. */
    switch(conn_desc_ptr->conn_status.state){
        case POFCS_CHANNEL_INVALID:
        case POFCS_CHANNEL_CONNECTING:
        case POFCS_CHANNEL_CONNECTED:
        case POFCS_HELLO:
        case POFCS_REQUEST_FEATURE:
            break;
        case POFCS_CHANNEL_RUN:
            /* Send echo to controller. */
            /* Build echo message. */
            len = sizeof(pof_header);
            pofsc_build_header(&head, POFT_ECHO_REQUEST, len, g_upward_xid++);

            /* Write error message into queue for sending. */
            ret = pofbf_queue_write(pofsc_send_q_id, (char*)&head, len, POF_WAIT_FOREVER);
            if(ret != POF_OK){
                pofsc_set_error(POFET_SOFTWARE_FAILED, ret);
            }
            break;
        default:
            break;
    }
    return POF_OK;
}

/***********************************************************************
 * Set connection atributes.
 * Form:     uint32_t pofsc_set_conn_attr(const char *controller_ip, \
 *                                        uint16_t port, \
 *                                        uint32_t retry_max, \
 *                                        uint32_t retry_interval)
 * Input:    controller IP address, port, retry max, retry interval
 * Output:   pofsc_conn_desc
 * Return:   POF_OK or ERROR code
 * Discribe: This function sets connection atributes and stores it in
 *           pofsc_conn_desc
 ***********************************************************************/
static uint32_t pofsc_set_conn_attr(const char *controller_ip, \
                                    uint16_t port, \
                                    uint32_t retry_max, \
                                    uint32_t retry_interval)
{
    memset((void *)&pofsc_conn_desc, 0, sizeof(pofsc_dev_conn_desc));

    memcpy((void*)pofsc_conn_desc.controller_ip, (void*)controller_ip, strlen(controller_ip));
    pofsc_conn_desc.controller_port = port;
    pofsc_conn_desc.conn_retry_max = retry_max;
    pofsc_conn_desc.conn_retry_interval = retry_interval;
    pofsc_conn_desc.conn_status.echo_interval = pofsc_echo_interval;

    return POF_OK;
}

/***********************************************************************
 * Create socket.
 * Form:     uint32_t pofsc_create_socket(int *socket_fd_ptr)
 * Input:    NONE
 * Output:   socket_fd
 * Return:   POF_OK or ERROR code
 * Discribe: This function create the OpenFlow client socket with TCP
 *           channel.
 ***********************************************************************/
static uint32_t pofsc_create_socket(int *socket_fd_ptr){
    /* Socket file descriptor. */
    int socket_fd;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        POF_DEBUG_CPRINT_FL (1,RED,"Create socket failure!");
        return (POF_CREATE_SOCKET_FAILURE);
    }
    *socket_fd_ptr = socket_fd;

    return POF_OK;
}

/***********************************************************************
 * Connect controller.
 * Form:     uint32_t pofsc_connect(int socket_fd, char *server_ip, \
 *                                  uint16_t port, struct pof_datapath *dp)
 * Input:    socket_fd, server IP string, port
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function connect the Soft Switch with the Conteroller
 *           by using the socket_fd
 ***********************************************************************/
static uint32_t pofsc_connect(int socket_fd, char *server_ip, uint16_t port, struct pof_datapath *dp){
	pofsc_dev_conn_desc *conn_desc_ptr = (pofsc_dev_conn_desc *)&pofsc_conn_desc;//add by wenjian 2015/12/02
    socklen_t sockaddr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in serverAddr, localAddr;
    char localIP[POF_IP_ADDRESS_STRING_LEN] = "\0";
    uint8_t hwaddr[POF_ETH_ALEN] = {0};
	uint8_t port_id;

	uint8_t local_port_index;
	int ifindex;
    struct pof_local_resource *lr, *lrNext;

    /* Build server socket address. */
    memset ((char *) &serverAddr, 0,  sizeof (struct sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = POF_HTONS(port);
    serverAddr.sin_addr.s_addr = inet_addr(server_ip);

    if(connect(socket_fd, (struct sockaddr *)&serverAddr, sizeof (struct sockaddr_in)) == -1){
        close(socket_fd);
        return (POF_CONNECT_SERVER_FAILURE);
    }

    if(g_poflr_dev_id == 0){
        if(getsockname(socket_fd, (struct sockaddr *)&localAddr, &sockaddr_len) != POF_OK){
            POF_ERROR_CPRINT_FL("Get socket name fail!");
            close(socket_fd);
            return POF_ERROR;
        }

        strcpy(localIP, inet_ntoa(localAddr.sin_addr));
        HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        	//add by wenjian 2015/12/02
            //if(poflr_get_hwaddr_by_ipaddr(hwaddr, localIP, lr) == POF_OK){
        	if((local_port_index=poflr_get_hwaddr_by_ipaddr(hwaddr, localIP, lr)) != POF_ERROR){
                break;
            }
        }
        //add by wenjian
        //give a value to pofsc_dev_conn_desc
        conn_desc_ptr->local_port_index=local_port_index;
        /* Get the device id using the low 32bit of hardware address of local
         * port connecting to the Controller. */
        memcpy(&g_poflr_dev_id, hwaddr+2, POF_ETH_ALEN-2);
        POF_NTOHL_FUNC(g_poflr_dev_id);
        if(g_poflr_dev_id == 0){
            g_poflr_dev_id = 1;
        }
        sprintf(g_states.devID.cont, "%u", g_poflr_dev_id);

        POF_DEBUG_CPRINT_FL(1,GREEN,"Local physical port ip is %s", localIP);
        POF_DEBUG_CPRINT_FL(1,GREEN,"g_poflr_dev_id = %d", g_poflr_dev_id);
    }else{
        POF_DEBUG_CPRINT_FL(1,GREEN,"g_poflr_dev_id = %d", g_poflr_dev_id);
    }

    return POF_OK;
}

/***********************************************************************
 * Receive message.
 * Form:     uint32_t pofsc_recv(int socket_fd, char* buf, int buflen, \
 *                              int* plen, struct pof_datapath *dp)
 * Input:    socket_fd, the max length of the buffer
 * Output:   data buffer, data length
 * Return:   POF_OK or ERROR code
 * Discribe: This function receive the messages from the Controller.
 ***********************************************************************/
static uint32_t pofsc_recv(int socket_fd, char* buf, int buflen, int* plen, struct pof_datapath *dp){
    pof_header *header_ptr;
    int len;

    if (buflen == 0){
        POF_ERROR_CPRINT_FL("The length of receive buf is zero.");
        return (POF_RECEIVE_MSG_FAILURE);
    }

    if ((len = read(socket_fd, buf, buflen)) <= 0){
        POF_ERROR_CPRINT_FL("closed socket fd!");
        close(socket_fd);
        pofsc_performance_after_ctrl_disconn(dp);
        return (POF_RECEIVE_MSG_FAILURE);
    }
    *plen = len;

    if(pofsc_conn_desc.conn_status.state == POFCS_CHANNEL_RUN){
        return POF_OK;
    }

#ifndef POF_DEBUG_PRINT_ECHO_ON
    header_ptr = (pof_header *)buf;
    if(header_ptr->type != POFT_ECHO_REPLY){
#endif
    POF_DEBUG_CPRINT_PACKET(buf,0,len);
#ifndef POF_DEBUG_PRINT_ECHO_ON
    }
#endif

    return POF_OK;
}

/***********************************************************************
 * Send message.
 * Form:     uint32_t pofsc_send(int socket_fd, char* buf, int len, \
 *                                  struct pof_datapath *dp)
 * Input:    socket_fd, data buffer, data length
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function send messages to the Controller in send task.
 ***********************************************************************/
static uint32_t pofsc_send(int socket_fd, char* buf, int len, struct pof_datapath *dp){
    int ret;
#ifndef POF_DEBUG_PRINT_ECHO_ON
    pof_header *header_ptr = (pof_header *)buf;
    if(header_ptr->type != POFT_ECHO_REQUEST){
#endif
    POF_DEBUG_CPRINT_PACKET(buf,1,len);
#ifndef POF_DEBUG_PRINT_ECHO_ON
    }
#endif

    /* Send message to server. */
    if ((ret = write(socket_fd, (char *)buf, len) == -1)){
        POF_ERROR_CPRINT_FL("Socket write ERROR!");
        close(socket_fd);
        pofsc_performance_after_ctrl_disconn(dp);
        return (POF_SEND_MSG_FAILURE);
    }

    return (POF_OK);
}

/***********************************************************************
 * The process function during the POFCS_CHANNEL_RUN state.
 * Form:     uint32_t pofsc_run_process(char *message, uint16_t len, \
 *                                      struct pof_datapath *dp)
 * Input:    message, length
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will be called when the communication module
 *           receive a message from the Controller during the
 *           POFCS_CHANNEL_RUN state.
 ***********************************************************************/
static uint32_t pofsc_run_process(char *message, uint16_t len, struct pof_datapath *dp){
    uint32_t ret = POF_OK;
    pof_header *head_ptr;
    pofsc_dev_conn_desc *conn_desc_ptr = (pofsc_dev_conn_desc *)&pofsc_conn_desc;

    head_ptr = (pof_header *)message;
    if(POF_NTOHS(head_ptr->length)!= len){
        pofsc_set_error(POFET_BAD_REQUEST, POFBRC_BAD_LEN);
        return POF_OK;
    }

    /* Handle echo reply message. */
    if(head_ptr->type == POFT_ECHO_REPLY){
        /* Record last echo time. */
        conn_desc_ptr->last_echo_time = time(NULL);
    }else{
        /* Forward to LPU board through IPC channel. */
        ret = pof_parse_msg_from_controller(message, dp);
		POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    }

    return POF_OK;
}

/***********************************************************************
 * Build the OpenFlow header.
 * Form:     void pofsc_build_header(pof_header *header, \
                                     uint8_t type, \
                                     uint16_t len, \
                                     uint32_t xid)
 * Input:    OpenFlow packet type, packet length, packet xid
 * Output:   header
 * Return:   VOID
 * Discribe: This function builds the OpenFlow header.
 ***********************************************************************/
static uint32_t pofsc_build_header(pof_header *header, \
                                   uint8_t type, \
                                   uint16_t len, \
                                   uint32_t xid)
{
    header->version = POF_VERSION;
    header->type = type;
    header->length = len;
    header->xid = xid;
	pof_HtoN_transfer_header(header);

    return POF_OK;
}

/***********************************************************************
 * Set error.
 * Form:     void pofsc_set_error(uint16_t type, uint16_t code)
 * Input:    error type, error code
 * Output:   pofsc_protocol_error
 * Return:   VOID
 * Discribe: This function sets error which occurs in control module in
 *           the Soft Switch.
 ***********************************************************************/
static uint32_t pofsc_set_error(uint16_t type, uint16_t code){
    pofsc_protocol_error.type = POF_HTONS(type);
    pofsc_protocol_error.code = POF_HTONS(code);
    pofsc_protocol_error.device_id = POF_HTONL(POF_FE_ID);
#ifdef POF_MULTIPLE_SLOTS
    pofsc_protocol_error.slotID = POF_HTONS(POF_SLOT_ID_BASE);
#endif // POF_MULTIPLE_SLOTS
    return POF_OK;
}

/***********************************************************************
 * Build error message and send it to message queue.
 * Form:     uint32_t pofsc_build_error_msg(char *message, uint16_t *len_p)
 * Input:    message data, message length
 * Output:   message data
 * Return:   VOID
 * Discribe: This function build error message and send it to message
 *           queueu.
 ***********************************************************************/
static uint32_t pofsc_build_error_msg(char *message, uint16_t *len_p){
    pof_header *head_ptr = (pof_header*)message;
    uint32_t ret = POF_OK;
    uint16_t len = sizeof(pof_header) + sizeof(pof_error);

    /* Build header. */
     pofsc_build_header(head_ptr, POFT_ERROR, len, g_upward_xid++);

    /* Copy error content into message. */
    memcpy((message + sizeof(pof_header)), &pofsc_protocol_error, sizeof(pof_error));

    /* Clear error record. */
    pofsc_protocol_error.type = 0xFFFF;

    *len_p = len;
    return ret;
}

/* Send packet upward to the Contrller through OpenFlow channel. */
uint32_t pofsc_send_packet_upward(uint8_t *packet, uint32_t len){
    if(POF_OK != pofbf_queue_write(pofsc_send_q_id, packet, len, POF_WAIT_FOREVER)){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE);
    }

	return POF_OK;
}

/* Set the Controller's IP address. */
uint32_t pofsc_set_controller_ip(char *ip_str){
	strncpy(pofsc_controller_ip_addr, ip_str, POF_IP_ADDRESS_STRING_LEN);
	strncpy(g_states.ctrl_ip.cont, ip_str, POF_IP_ADDRESS_STRING_LEN);
	return POF_OK;
}

/* Set the Controller's port. */
uint32_t pofsc_set_controller_port(uint16_t port){
	pofsc_controller_port = port;
    sprintf(g_states.conn_port.cont, "%u", port);
	return POF_OK;
}

/* Check whether the euid is the root id. */
uint32_t pofsc_check_root(){
	/* Root id = 0 */
	if(geteuid() == 0){
		return POF_OK;
	}else{
		printf("pofswitch ERROR: Permission denied.\n");
		return POF_ERROR;
	}
}

/***********************************************************************
 * Destroy task, timer and queue.
 * Form:     uint32_t pofsc_destroy(struct pof_datapath *dp)
 * Input:    NONE
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function destroys all of the tasks, timers and queues
 *           in Soft Switch in order to reclaim the resource.
 ***********************************************************************/
static uint32_t pofsc_destroy(struct pof_datapath *dp){
    uint32_t i;
	uint16_t port_number = 0;
    struct pof_local_resource *lr, *lrNext;

    /* Free task,timer and queue. */
    if(pofsc_main_task_id != POF_INVALID_TASKID){
        pofbf_task_delete(&pofsc_main_task_id);
    }

    if(pofsc_send_task_id != POF_INVALID_TASKID){
        pofbf_task_delete(&pofsc_send_task_id);
    }

    if(pofsc_listen_task_id != POF_INVALID_TASKID){
        pofbf_task_delete(&pofsc_listen_task_id);
    }

    if(pofsc_echo_timer_id != POF_INVALID_TIMERID){
        pofbf_timer_delete(&pofsc_echo_timer_id);
    }

    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        poflr_ports_task_delete(lr);
    }

    if(pofsc_send_q_id != POF_INVALID_QUEUEID){
        pofbf_queue_delete(&pofsc_send_q_id);
    }

	pof_close_log_file();

    return POF_OK;
}

/***********************************************************************
 * The endless function which control module is running.
 * Form:     static void pofsc_wait_exit()
 * Input:    NONE
 * Output:   NONE
 * Return:   VOID
 * Discribe: After all of the initialization function, the main task will
 *           keep running in this function until "quit" is inputed by
 *           user command line.
 ***********************************************************************/
static uint32_t pofsc_wait_exit(struct pof_datapath *dp){
    if(strcmp(g_states.verbosity.cont,"MUTE") != POF_OK){
        pof_runing_command(dp);
    }else{
        while(1){
            continue;
        }
    }
    terminate_handler();
    return POF_OK;
}

/***********************************************************************
 * The quit function
 * Form:     void terminate_handler()
 * Input:    NONE
 * Output:   NONE
 * Return:   VOID
 * Discribe: This function, which will reclaim all of the resource and
 *           terminate all of the task, is called when an unexpected crush
 *           happens, or we want to shut down the Soft Switch.
 ***********************************************************************/
uint32_t pofsc_terminate_flag = FALSE;
void terminate_handler(){
    struct pof_datapath *dp = &g_dp;
    if(pofsc_terminate_flag == TRUE){
        return;
    }
    pofsc_terminate_flag = TRUE;
    POF_DEBUG_CPRINT_FL(1,RED,"Call terminate_handler!");
    pofsc_destroy(dp);
    exit(0);
}

static uint32_t pofsc_performance_after_ctrl_disconn(struct pof_datapath *dp){
    struct pof_local_resource *lr, *lrNext;
#if (POF_PERFORM_AFTER_CTRL_DISCONN == POF_AFTER_CTRL_DISCONN_SHUT_DOWN)
    terminate_handler();
#elif (POF_PERFORM_AFTER_CTRL_DISCONN == POF_AFTER_CTRL_DISCONN_RECONN)
    pofsc_conn_desc.conn_status.state = POFCS_CHANNEL_INVALID;
	if(pof_auto_clear()){
        HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
		    poflr_clear_resource(lr);
        }
	}
#endif // POF_PERFORM_AFTER_CTRL_DISCONN
    return POF_OK;
}
