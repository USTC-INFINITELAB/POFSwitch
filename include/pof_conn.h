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

#ifndef _POF_CONN_H_
#define _POF_CONN_H_

#include "pof_datapath.h"

/* Define the server's port number. */
#define POF_CONTROLLER_PORT_NUM (6633)

/* Define the server's IP. */
#define POF_CONTROLLER_IP_ADDR "192.168.1.1"

/* Define the max retry time of cnnection. */
#define POF_CONNECTION_MAX_RETRY_TIME (0XFFFFFFFF)

/* Define the retry interval of connection if connection fails. */
#define POF_CONNECTION_RETRY_INTERVAL (2)  /* Seconds. */

/* Define max size of sending buffer. */
#define POF_SEND_BUF_MAX_SIZE (POF_MESSAGE_SIZE)

/* Define max size of receiving buffer. */
#define POF_RECV_BUF_MAX_SIZE (POF_MESSAGE_SIZE)

/* Define echo interval .*/
#define POF_ECHO_INTERVAL (2000)  /* Unit is millisecond. */

/* Message queue attributes. */
#define POF_QUEUE_MESSAGE_LEN (POF_MESSAGE_SIZE)

extern char pofsc_controller_ip_addr[POF_IP_ADDRESS_STRING_LEN];
extern uint16_t pofsc_controller_port;

/* Openflow device connection description. */
typedef struct pofsc_dev_conn_desc{
    /* Controller information. */
    char controller_ip[POF_IP_ADDRESS_STRING_LEN];  /* Ipv4 address of openflow controller. */
    uint16_t controller_port;

    /* Connection socket id and socket buffers. */
    int sfd; /* Scket id. */
    char send_buf[POF_SEND_BUF_MAX_SIZE];
    char recv_buf[POF_RECV_BUF_MAX_SIZE];
    char msg_buf[POF_QUEUE_MESSAGE_LEN];

    /* Connection retry count and connection state. */
    uint32_t conn_retry_interval; /* Unit is second. */
    uint32_t conn_retry_max;
    uint32_t conn_retry_count;
    pof_connect_status conn_status;

    /* Last echo reply time. */
    time_t last_echo_time;

    //add by wenjian 2015/12/02
    uint8_t local_port_index;
}  pofsc_dev_conn_desc;

/* Define Soft Switch control module state. */
typedef enum{
    POFCS_CHANNEL_INVALID       = 0,
    POFCS_CHANNEL_CONNECTING    = 1,
    POFCS_CHANNEL_CONNECTED     = 2,
    POFCS_HELLO                 = 3,
    POFCS_REQUEST_FEATURE       = 4,
    POFCS_SET_CONFIG            = 5,
    POFCS_REQUEST_GET_CONFIG    = 6,
    POFCS_CHANNEL_RUN           = 7,
    POFCS_STATE_MAX             = 8,
} pof_channel_state;

/* Description of device connection. */
extern volatile pofsc_dev_conn_desc pofsc_conn_desc;

extern uint32_t pof_set_init_config(int argc, char *argv[], struct pof_datapath *);
extern uint32_t pof_auto_clear();
extern uint32_t pofsc_set_controller_ip(char *ip_str);
extern uint32_t pofsc_set_controller_port(uint16_t port);

/* parse and encap. */
extern uint32_t pof_parse_msg_from_controller(char* msg_ptr, struct pof_datapath *);
extern uint32_t pofec_reply_error(uint16_t type, uint16_t code, char *s, uint32_t xid);
extern uint32_t pofec_set_error(uint16_t type, char *type_str, uint16_t code, char *error_str);
extern uint32_t pofec_reply_msg(uint8_t  type, \
                                uint32_t xid, \
                                uint32_t msg_len, \
                                uint8_t  *msg_body);

extern uint32_t pofsc_check_root();

#endif // _POF_CONN_H_


