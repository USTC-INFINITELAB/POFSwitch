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

#ifndef _POF_GLOBAL_H_
#define _POF_GLOBAL_H_

#include "pof_type.h"

/*openflow version.*/
#define POF_VERSION (0x04)

/*Define Invalid value*/
#define POF_INVALID_VALUE   (0xFFFFFFFF)

/*Define the length of device name.*/
#define POF_NAME_MAX_LENGTH   (64)

/* Bytes in an Ethernet address. */
#define POF_ETH_ALEN (6)

/* Define the string length of IPv4 address. */
#define POF_IP_ADDRESS_STRING_LEN (20)

/*Define the max length of error string.*/
#define POF_ERROR_STRING_MAX_LENGTH	(256)

/*Define the max length of string.*/
#define POF_STRING_MAX_LEN (100)

/*Define the max length of packetin.*/
#define POF_PACKET_IN_MAX_LENGTH (2048)

/*Define the MTU length, including the ether header.*/
#define POF_MTU_LENGTH (1514)

/*Define the max length in byte unit of match field.*/
#define POF_MAX_FIELD_LENGTH_IN_BYTE (16)

/*Define the max number of match field in one flow entry.*/
#define POF_MAX_MATCH_FIELD_NUM (8)

/*Define the max instruction number of one flow entry.*/
#define POF_MAX_INSTRUCTION_NUM (6)

/*Define the max field number of one protocol.*/
#define POF_MAX_PROTOCOL_FIELD_NUM (8)

/*Define the max instruction length in unit of byte.*/
#define POF_MAX_INSTRUCTION_LENGTH  (8 + POF_MAX_ACTION_NUMBER_PER_INSTRUCTION * (POF_MAX_ACTION_LENGTH + 4))

/*Define the max action number in one instruction.*/
#define POF_MAX_ACTION_NUMBER_PER_INSTRUCTION (6)

/*Define the max action number in one group.*/
#define POF_MAX_ACTION_NUMBER_PER_GROUP (4)

/*Define the max action length in unit of byte.*/
#define POF_MAX_ACTION_LENGTH (44)

/*define OpenFlow enabled FE ID*/
#define POF_FE_ID g_poflr_dev_id

/* Define value type in instructions and actions. */
#define POFVT_IMMEDIATE_NUM (0)	/* Immediate value. */
#define POFVT_FIELD			(1)	/* packet/metadata field. */

/* Define direction in instructions and actions. 
 * Forward or backward. */
#define POFD_FORWARD  (0)
#define POFD_BACKWARD (1)


#define POFSwitch_VERSION_1_1_8   (1)
#define POFSwitch_VERSION_1_1_9   (2)
#define POFSwitch_VERSION_1_4_0   (3)
#define POFSwitch_VERSION_1_5_0   (4)

#define POFSwitch_VERSION_1_1_8_STR   "POFSwitch-1.1.8"
#define POFSwitch_VERSION_1_1_9_STR   "POFSwitch-1.1.9"
#define POFSwitch_VERSION_1_4_0_STR   "POFSwitch-1.4.0"
#define POFSwitch_VERSION_1_5_0_STR   "POFSwitch-1.5.0"

/* POFSwitch version. *
#undef POFSWITCH_VERSION_NO
//#define POFSWITCH_VERSION_NO POFSwitch_VERSION_1_1_8
//#define POFSWITCH_VERSION_NO POFSwitch_VERSION_1_1_9
//#define POFSWITCH_VERSION_NO POFSwitch_VERSION_1_4_0
#define POFSWITCH_VERSION_NO POFSwitch_VERSION_1_5_0
//*/

#if     ( POFSWITCH_VERSION_NO == POFSwitch_VERSION_1_1_8 )
#define POFSwitch_VERSION_STR POFSwitch_VERSION_1_1_8_STR
#define POF_SD2N (1)
#elif   ( POFSWITCH_VERSION_NO == POFSwitch_VERSION_1_1_9 )
#define POFSwitch_VERSION_STR POFSwitch_VERSION_1_1_9_STR
#define POF_SD2N (1)
#define POF_SD2N_AFTER1015 (1)
#elif   ( POFSWITCH_VERSION_NO == POFSwitch_VERSION_1_4_0 )
#define POFSwitch_VERSION_STR POFSwitch_VERSION_1_4_0_STR
#define POF_SD2N (1)
#define POF_SD2N_AFTER1015 (1)
#define POF_MULTIPLE_SLOTS (1)
#elif   ( POFSWITCH_VERSION_NO == POFSwitch_VERSION_1_5_0 )
#define POFSwitch_VERSION_STR POFSwitch_VERSION_1_5_0_STR
#define POF_SD2N (1)
#define POF_SD2N_AFTER1015 (1)
#define POF_MULTIPLE_SLOTS (1)
#define POF_SHT_VXLAN (1)
#endif // POFSWITCH_VERSION_NO

typedef enum pof_slot_id {
    POFSID_ALL = 0xFFFF, 
}pof_slot_id;

/*Define the openflow command type.*/
typedef enum pof_type {
    /* Immutable messages. */
    POFT_HELLO = 0, /* Symmetric message */
    POFT_ERROR = 1, /* Symmetric message */
    POFT_ECHO_REQUEST = 2, /* Symmetric message */
    POFT_ECHO_REPLY = 3, /* Symmetric message */
    POFT_EXPERIMENTER = 4, /* Symmetric message */

    /* Switch configuration messages. */
    POFT_FEATURES_REQUEST = 5, /* Controller/switch message */
    POFT_FEATURES_REPLY = 6, /* Controller/switch message */
    POFT_GET_CONFIG_REQUEST = 7, /* Controller/switch message */
    POFT_GET_CONFIG_REPLY = 8, /* Controller/switch message */
    POFT_SET_CONFIG = 9, /* Controller/switch message */

    /* Asynchronous messages. */
    POFT_PACKET_IN = 10, /* Async message */
    POFT_FLOW_REMOVED = 11, /* Async message */
    POFT_PORT_STATUS = 12, /* Async message */
    POFT_RESOURCE_REPORT = 13,/* Async message */

    /* Controller command messages. */
    POFT_PACKET_OUT = 14, /* Controller/switch message */
    POFT_FLOW_MOD = 15, /* Controller/switch message */
    POFT_GROUP_MOD = 16, /* Controller/switch message */
    POFT_PORT_MOD = 17, /* Controller/switch message */
    POFT_TABLE_MOD = 18, /* Controller/switch message */

    /* Multipart messages. */
    POFT_MULTIPART_REQUEST = 19, /* Controller/switch message */
    POFT_MULTIPART_REPLY = 20, /* Controller/switch message */

    /* Barrier messages. */
    POFT_BARRIER_REQUEST = 21, /* Controller/switch message */
    POFT_BARRIER_REPLY = 22, /* Controller/switch message */

    /* Queue Configuration messages. */
    POFT_QUEUE_GET_CONFIG_REQUEST = 23, /* Controller/switch message */
    POFT_QUEUE_GET_CONFIG_REPLY = 24, /* Controller/switch message */

    /* Controller role change request messages. */
    POFT_ROLE_REQUEST = 25, /* Controller/switch message */
    POFT_ROLE_REPLY = 26, /* Controller/switch message */

    /* Asynchronous message configuration. */
    POFT_GET_ASYNC_REQUEST = 27, /* Controller/switch message */
    POFT_GET_ASYNC_REPLY = 28, /* Controller/switch message */
    POFT_SET_ASYNC = 29, /* Controller/switch message */

    /* Meters and rate limiters configuration messages. */
    POFT_METER_MOD = 30, /* Controller/switch message */

    /*Conter message*/
    POFT_COUNTER_MOD = 31, /* Controller/switch message */
    POFT_COUNTER_REQUEST = 32, /* Controller/switch message */
    POFT_COUNTER_REPLY = 33, /* Controller/switch message */

    /*Query all message*/
    POFT_QUERYALL_REQUEST = 34, /* Controller to switch message. */
    POFT_QUERYALL_FIN = 35,     /* Switch to controller message when finished sending all 
                                 * queried message. */
#ifdef POF_SHT_VXLAN
    /* Instruction Block Message. */
    POFT_INSTRUCTION_BLOCK_MOD = 36,

    /* Enable/Disable POF forwarding of one designated slot. */
    POFT_SLOT_CONFIG = 101,

    /* Slot status message from device to controller. */
    POFT_SLOT_STATUS = 102
#endif // POF_SHT_VXLAN
 }pof_type;

/* Table commands */
typedef enum pof_table_mod_command {
    POFTC_ADD = 0, /* New table. */
    POFTC_MODIFY = 1, /* Modify specified table. */
    POFTC_DELETE = 2, /* Delete specified table. */
    POFTC_QUERY = 3,
    POFTC_QUERY_RESULT = 4,
}pof_table_mod_command;

/* Meter commands */
typedef enum pof_meter_mod_command {
    POFMC_ADD = 0, /*New meter. */
    POFMC_MODIFY = 1, /*Modify specified meter. */
    POFMC_DELETE = 2, /* Delete specified meter. */
    POFMC_QUERY = 3,
    POFMC_QUERY_RESULT = 4,
}pof_meter_mod_command;

/* Group commands */
typedef enum pof_group_mod_command {
    POFGC_ADD = 0, /* New group. */
    POFGC_MODIFY = 1, /* Modify all matching groups. */
    POFGC_DELETE = 2, /* Delete all matching groups. */
    POFGC_QUERY = 3,
    POFGC_QUERY_RESULT = 4,
}pof_group_mod_command;

/* Counter commands */
typedef enum pof_counter_mod_command {
    POFCC_ADD = 0, /* New counter. */
    POFCC_DELETE = 1, /* Delete counter. */
    POFCC_CLEAR = 2,
    POFCC_QUERY = 3,
    POFCC_QUERY_RESULT = 4,
}pof_counter_mod_command;

/* flow commands */
typedef enum pof_flow_mod_command {
    POFFC_ADD = 0, /* New flow. */
    POFFC_MODIFY = 1, /* Modify all matching flows. */
    POFFC_MODIFY_STRICT = 2, /* Modify entry strictly matching wildcards and priority. */
    POFFC_DELETE = 3, /* Delete all matching flows. */
    POFFC_DELETE_STRICT = 4, /* Delete entry strictly matching wildcards and priority. */
    POFFC_QUERY = 5,
    POFFC_QUERY_RESULT = 6,
}pof_flow_mod_command;

#ifdef POF_SHT_VXLAN
#define INSTRUCTIONS \
	/* Setup the next table in the lookup pipeline */		\
	INSTRUCTION(GOTO_TABLE, 1)								\
	/* Setup the metadata field for use later in pipeline */\
    /* Delete in SHT_VXLAN. */                              \
	INSTRUCTION(WRITE_METADATA, 2)							\
	/* Write the action(s) onto the datapath action set */	\
	INSTRUCTION(WRITE_ACTIONS, 3)							\
	/* Applies the action(s) immediately */					\
	INSTRUCTION(APPLY_ACTIONS, 4)							\
	/* Clears all actions from the datapath action set */	\
	INSTRUCTION(CLEAR_ACTIONS, 5)							\
	/* Apply meter (rate limiter) */						\
	INSTRUCTION(METER, 6)									\
	/* Setup the metadata field for use later in pipeline */\
    /* Delete in SHT_VXLAN. */                              \
	INSTRUCTION(WRITE_METADATA_FROM_PACKET, 7)				\
	/* Setup the next table in the lookup pipeline */		\
	INSTRUCTION(GOTO_DIRECT_TABLE, 8)						\
	/* Move the PC based on the comparison result of two
	 * compared fields. */									\
    /* Delete in SHT_VXLAN. */                              \
	INSTRUCTION(CONDITIONAL_JMP, 9)							\
	/* Calculate the packet/metadata field. */				\
    /* Delete in SHT_VXLAN. */                              \
	INSTRUCTION(CALCULATE_FIELD, 10)						\
	/* Move the packet offset forward or backward. */		\
	INSTRUCTION(MOVE_PACKET_OFFSET, 11)						\
	INSTRUCTION(SET_PACKET_OFFSET, 12)						\
	INSTRUCTION(COMPARE, 13)						        \
	INSTRUCTION(BRANCH, 14)						            \
	INSTRUCTION(JMP, 15)						            \
	/* Experimenter instruction */							\
	INSTRUCTION(EXPERIMENTER, 0xFFFF)
#else // POF_SHT_VXLAN
#ifdef POF_SD2N
#define INSTRUCTIONS \
	/* Setup the next table in the lookup pipeline */		\
	INSTRUCTION(GOTO_TABLE, 1)								\
	/* Setup the metadata field for use later in pipeline */\
	INSTRUCTION(WRITE_METADATA, 2)							\
	/* Write the action(s) onto the datapath action set */	\
	INSTRUCTION(WRITE_ACTIONS, 3)							\
	/* Applies the action(s) immediately */					\
	INSTRUCTION(APPLY_ACTIONS, 4)							\
	/* Clears all actions from the datapath action set */	\
	INSTRUCTION(CLEAR_ACTIONS, 5)							\
	/* Apply meter (rate limiter) */						\
	INSTRUCTION(METER, 6)									\
	/* Setup the metadata field for use later in pipeline */\
	INSTRUCTION(WRITE_METADATA_FROM_PACKET, 7)				\
	/* Setup the next table in the lookup pipeline */		\
	INSTRUCTION(GOTO_DIRECT_TABLE, 8)						\
	/* Move the PC based on the comparison result of two
	 * compared fields. */									\
	INSTRUCTION(CONDITIONAL_JMP, 9)							\
	/* Calculate the packet/metadata field. */				\
	INSTRUCTION(CALCULATE_FIELD, 10)						\
	/* Move the packet offset forward or backward. */		\
	INSTRUCTION(MOVE_PACKET_OFFSET, 11)						\
	/* Experimenter instruction */							\
	INSTRUCTION(EXPERIMENTER, 0xFFFF)
#else // POF_SD2N
#define INSTRUCTIONS \
	/* Setup the next table in the lookup pipeline */		\
	INSTRUCTION(GOTO_TABLE, 1)								\
	/* Setup the metadata field for use later in pipeline */\
	INSTRUCTION(WRITE_METADATA, 2)							\
	/* Write the action(s) onto the datapath action set */	\
	INSTRUCTION(WRITE_ACTIONS, 3)							\
	/* Applies the action(s) immediately */					\
	INSTRUCTION(APPLY_ACTIONS, 4)							\
	/* Clears all actions from the datapath action set */	\
	INSTRUCTION(CLEAR_ACTIONS, 5)							\
	/* Apply meter (rate limiter) */						\
	INSTRUCTION(METER, 6)									\
	/* Setup the metadata field for use later in pipeline */\
	INSTRUCTION(WRITE_METADATA_FROM_PACKET, 7)				\
	/* Setup the next table in the lookup pipeline */		\
	INSTRUCTION(GOTO_DIRECT_TABLE, 8)						\
	/* Experimenter instruction */							\
	INSTRUCTION(EXPERIMENTER, 0xFFFF)
#endif // POF_SD2N
#endif // POF_SHT_VXLAN

/*define instruction type*/
typedef enum pof_instruction_type {
#define INSTRUCTION(NAME, VALUE) POFIT_##NAME = VALUE,
	INSTRUCTIONS
#undef INSTRUCTION
}pof_instruction_type;

#ifdef POF_SHT_VXLAN
#define ACTIONS \
	/* Output to switch port. */					\
	ACTION(OUTPUT, 0)								\
	/* Set field with immediate value. */			\
	ACTION(SET_FIELD, 1)							\
	/* Set field with metadata value. */			\
	ACTION(SET_FIELD_FROM_METADATA, 2)				\
	/* Modify field by plus or minus one value.*/	\
	ACTION(CALCULATE_FIELD, 3)							\
	/* Add field. */								\
	ACTION(ADD_FIELD, 4)							\
	/* Delete field. */								\
	ACTION(DELETE_FIELD, 5)							\
	/* Calculate checksum.*/						\
	ACTION(CALCULATE_CHECKSUM, 6)					\
	/* Group action.*/								\
	ACTION(GROUP, 7)								\
	/*Drop packet*/									\
	ACTION(DROP, 8)									\
	/*upward to controller packet*/					\
	ACTION(PACKET_IN, 9)							\
	/*Count packet*/								\
	ACTION(COUNTER, 10)								\
                                                    \
    /* Advanced action. */                          \
    /* Encap VXLAN header. */                       \
	ACTION(ENCAP_VXLAN_HEADER, 101)					\
    /* Encap UDP header. */                         \
	ACTION(ENCAP_UDP_HEADER, 102)					\
    /* Encap IP header. */                          \
	ACTION(ENCAP_IP_HEADER, 103)				    \
    /* Encap MAC header. */                         \
	ACTION(ENCAP_MAC_HEADER, 104)					\
	/* Experimenter action */						\
	ACTION(EXPERIMENTER, 0xFFFF)
#else // POF_SHT_VXLAN
#define ACTIONS \
	/* Output to switch port. */					\
	ACTION(OUTPUT, 0)								\
	/* Set field with immediate value. */			\
	ACTION(SET_FIELD, 1)							\
	/* Set field with metadata value. */			\
	ACTION(SET_FIELD_FROM_METADATA, 2)				\
	/* Modify field by plus or minus one value.*/	\
	ACTION(MODIFY_FIELD, 3)							\
	/* Add field. */								\
	ACTION(ADD_FIELD, 4)							\
	/* Delete field. */								\
	ACTION(DELETE_FIELD, 5)							\
	/* Calculate checksum.*/						\
	ACTION(CALCULATE_CHECKSUM, 6)					\
	/* Group action.*/								\
	ACTION(GROUP, 7)								\
	/*Drop packet*/									\
	ACTION(DROP, 8)									\
	/*upward to controller packet*/					\
	ACTION(PACKET_IN, 9)							\
	/*Count packet*/								\
	ACTION(COUNTER, 10)								\
	/* Experimenter action */						\
	ACTION(EXPERIMENTER, 0xFFFF)
#endif // POF_SHT_VXLAN

/*define action type*/
enum pof_action_type {
#define ACTION(NAME, VALUE) POFAT_##NAME = VALUE,
	ACTIONS
#undef ACTION
};

/* Header on all OpenFlow packets. */
typedef struct pof_header{
    uint8_t  version; /* POF_VERSION. */
    uint8_t  type; /* One of the POFT_ constants. */
    uint16_t length; /* Length including this pof_header. */
    uint32_t xid; /* Transaction id associated with this packet.
                    Replies use the same id as was in the request to facilitate pairing. */
}pof_header;        //sizeof=8


/* Describe the status of connection, the last error, and the echo interval */
typedef struct pof_connect_status{
    uint8_t state;  /*openflow channel state*/
    uint8_t last_error; /*each error ID has one relative error string */
    uint16_t echo_interval;
    uint8_t pad[4];   /*8 bytes aligned*/
#ifdef POF_CONTROLLER
    time_t  startup_time; /*The startup time is got on controller.*/
#endif
}pof_connect_status;

typedef struct pof_queryall_request{
    uint16_t slotID;   /* 0xFFFF means all slots. */
    uint8_t  pad[6];    /* 8 bytes aligned. */
}pof_queryall_request; //sizeof = 8

/*Upon session establishment, the controller sends an POFT_FEATURES_REQUEST message.
This message does not contain a body beyond the OpenFlow header.*/
typedef struct pof_switch_features{
    uint32_t dev_id;
#ifdef POF_MULTIPLE_SLOTS
    uint16_t slotID;
#endif // POF_MULTIPLE_SLOTS
    uint16_t port_num;
    uint16_t table_num;

#ifdef POF_MULTIPLE_SLOTS
    uint8_t pad[2];
    uint32_t capabilities;
#else //  POF_MULTIPLE_SLOTS
    uint32_t capabilities;
    uint8_t   pad[4];
#endif // POF_MULTIPLE_SLOTS

    char     vendor_id[POF_NAME_MAX_LENGTH];
    char     dev_fw_id[POF_NAME_MAX_LENGTH]; /*device forward engine ID*/
    char     dev_lkup_id[POF_NAME_MAX_LENGTH]; /*device lookup engine ID*/
}pof_switch_features;  //sizeof = 16 + 3*64 = 208

/* Switch configuration */
typedef struct pof_switch_config{
#ifdef POF_MULTIPLE_SLOTS
    uint32_t dev_id;
#endif // POF_MULTIPLE_SLOTS
    uint16_t flags;         /* POFC_* flags. */
    uint16_t miss_send_len; /* Max bytes of packet tha datapath
                               should send to the controller. See
                               pof_controller_max_len for valid values.*/
}pof_switch_config;  // sizeof() = 4

enum pof_config_flags{
    POFC_FRAG_NORMAL = 0,      /* No special handling for fragments. */
    POFC_FRAG_DROP   = 1 << 0, /* Drop fragments. */
    POFC_FRAG_REASM  = 1 << 1, /* Reassemble (only if POFC_IP_REASM set). */
    POFC_FRAG_MASK   = 3
};

/* What changed about the physical port */
enum pof_port_reason {
    POFPR_ADD = 0, /* The port was added. */
    POFPR_DELETE = 1, /* The port was removed. */
    POFPR_MODIFY = 2, /* Some attribute of the port has changed. */
};


/* Infomation of ports */
typedef enum pof_port_config {
	POFPC_PORT_UP = 0, /* Port is administratively up. */
    POFPC_PORT_DOWN = 1 << 0, /* Port is administratively down. */
    POFPC_NO_RECV = 1 << 2, /* Drop all packets received by port. */
    POFPC_NO_FWD = 1 << 5, /* Drop packets forwarded to port. */
    POFPC_NO_PACKET_IN = 1 << 6 /* Do not send packet-in msgs for port. */
}pof_port_config;

typedef enum pof_port_state {
    POFPS_LINK_DOWN = 1 << 0, /* No physical link present. */
    POFPS_BLOCKED = 1 << 1, /* Port is blocked */
    POFPS_LIVE = 1 << 2, /* Live for Fast Failover Group. */
}pof_port_state;

/* Port numbering. Ports are numbered starting from 1. */
typedef enum pof_port_id {
    /* Maximum number of physical and logical switch ports. */
    POFP_MAX = 0xffffff00,

    /* Reserved OpenFlow Port (fake output "ports"). */
    POFP_IN_PORT = 0xfffffff8, /* Send the packet out the input port. This reserved port must
                                be explicitly used in order to send back out of the input port. */
    POFP_TABLE = 0xfffffff9, /* Submit the packet to the first flow table NB: This destination port can only be
                                 used in packet-out messages. */
    POFP_NORMAL = 0xfffffffa, /* Process with normal L2/L3 switching. */
    POFP_FLOOD = 0xfffffffb, /* All physical ports in VLAN, except input port and those blocked or link down. */
    POFP_ALL = 0xfffffffc, /* All physical ports except input port. */
    POFP_CONTROLLER = 0xfffffffd, /* Send to controller. */
    POFP_LOCAL = 0xfffffffe, /* Local openflow "port". */

    POFP_ANY = 0xffffffff /* Wildcard port used only for flow mod (delete) and flow stats requests. Selects
                              all flows regardless of output port (including flows with no output port). */
}pof_port_id;

/* Capabilities supported by the datapath. */
enum pof_capabilities {
    POFC_FLOW_STATS = 1 << 0, /* Flow statistics. */
    POFC_TABLE_STATS = 1 << 1, /* Table statistics. */
    POFC_PORT_STATS = 1 << 2, /* Port statistics. */
    POFC_GROUP_STATS = 1 << 3, /* Group statistics. */
    POFC_IP_REASM = 1 << 5, /* Can reassemble IP fragments. */
    POFC_QUEUE_STATS = 1 << 6, /* Queue statistics. */
    POFC_PORT_BLOCKED = 1 << 8 /* Switch will block looping ports. */
};


/* Features of ports available in a datapath. */
typedef enum pof_port_features{
    POFPF_10MB_HD = 1 << 0, /* 10 Mb half-duplex rate support. */
    POFPF_10MB_FD = 1 << 1, /* 10 Mb full-duplex rate support. */
    POFPF_100MB_HD = 1 << 2, /* 100 Mb half-duplex rate support. */
    POFPF_100MB_FD = 1 << 3, /* 100 Mb full-duplex rate support. */
    POFPF_1GB_HD = 1 << 4, /* 1 Gb half-duplex rate support. */
    POFPF_1GB_FD = 1 << 5, /* 1 Gb full-duplex rate support. */
    POFPF_10GB_FD = 1 << 6, /* 10 Gb full-duplex rate support. */
    POFPF_40GB_FD = 1 << 7, /* 40 Gb full-duplex rate support. */
    POFPF_100GB_FD = 1 << 8, /* 100 Gb full-duplex rate support. */
    POFPF_1TB_FD = 1 << 9, /* 1 Tb full-duplex rate support. */
    POFPF_OTHER = 1 << 10, /* Other rate, not in the list. */
    POFPF_COPPER = 1 << 11, /* Copper medium. */
    POFPF_FIBER = 1 << 12, /* Fiber medium. */
    POFPF_AUTONEG = 1 << 13, /* Auto-negotiation. */
    POFPF_PAUSE = 1 << 14, /* Pause. */
    POFPF_PAUSE_ASYM = 1 << 15 /* Asymmetric pause. */
}pof_port_features;

/* Why is this packet being sent to the controller? */
enum pof_packet_in_reason {
    POFR_NO_MATCH = 0, /* No matching flow (table-miss flow entry). */
    POFR_ACTION = 1, /* Action explicitly output to controller. */
    POFR_INVALID_TTL = 2, /* Packet has invalid TTL */
};


typedef struct pof_port{
#ifdef POF_MULTIPLE_SLOTS
    uint16_t slotID;
    uint16_t port_id;
#else // POF_MULTIPLE_SLOTS
    uint32_t port_id;  /*Port numberring */
#endif // POF_MULTIPLE_SLOTS
    uint32_t device_id; /*The device id*/

    uint8_t hw_addr[POF_ETH_ALEN];
    uint8_t pad[2];

    char    name[POF_NAME_MAX_LENGTH];

    uint32_t config; /*Bitmap of POFPC_* */
    uint32_t state; /* Bitmap of POFPS_**/

    /*Port features described by POFPF_* */
    uint32_t curr;   /* port current features described by POFPF_* */
    uint32_t advertised;   /* Advertised features described by POFPF_* */
    uint32_t supported;   /* Supported features described by POFPF_* */
    uint32_t peer;   /* features advertised by peer.  */

    uint32_t curr_speed;
    uint32_t max_speed;

    uint8_t of_enable; /*indicate whether openflow is enabled */
    uint8_t pad2[7];   /*8 bytes aligned*/
}pof_port;  //sizeof=88

typedef struct pof_port_status{
    uint8_t reason; /* One of POFPR_*. */
    uint8_t pad[7]; /* Align to 64-bits. */
    pof_port desc;
}pof_port_status;


typedef enum pof_table_type{
    POF_MM_TABLE = 0,
    POF_LPM_TABLE,
    POF_EM_TABLE,
    POF_LINEAR_TABLE,
    POF_MAX_TABLE_TYPE
}pof_table_type;

typedef struct pof_table_resource_desc{
    uint32_t device_id;
    uint8_t  type; /*table type: MM or EM or LPM */
    uint8_t  tbl_num; /*table number*/
    uint16_t key_len;   /*key length*/

    uint32_t total_size; /*the  total number of EM entry*/
    uint8_t pad[4];   /*8 bytes aligned*/
}pof_table_resource_desc;       //sizeof=16

typedef struct pof_flow_table_resource{
    uint8_t resourceType;
#ifdef POF_MULTIPLE_SLOTS
    uint8_t pad;    /* 8 bytes aligned. */
    uint16_t slotID;
#else // POF_MULTIPLE_SLOTS
    uint8_t pad[3];   /*8 bytes aligned*/
#endif // POF_MULTIPLE_SLOTS
    uint32_t counter_num; /*Counter number*/

    uint32_t meter_num; /*Meter number*/
    uint32_t group_num; /*Group number*/

    pof_table_resource_desc tbl_rsc_desc[POF_MAX_TABLE_TYPE]; /*All table resource information*/

}pof_flow_table_resource;       //sizeof=POF_MAX_TABLE_TYPE * 16 + 16 = 80

typedef struct pof_error{
    uint16_t type;  /*error type*/
    uint16_t code;  /*error code*/
    uint32_t device_id; /*The device in which error occurs.*/
#ifdef POF_MULTIPLE_SLOTS
    uint16_t slotID;
    uint8_t pad[6];
#endif // POF_MULTIPLE_SLOTS
    char     err_str[POF_ERROR_STRING_MAX_LENGTH];
}pof_error;     //sizeof=8 + 256 = 264

/* Describe the packet struct upward to Controller. */
typedef struct pof_packet_in {
    uint32_t buffer_id; /*Buffer ID assigned by datapath. 0xffffffff means invalid buffer id*/
    uint16_t total_len; /*Full length of the packet. */
    uint8_t  reason;  /*Reason that packet is sent.*/
    uint8_t  table_id; /*ID of the table that was looked up*/

    uint64_t cookie; /*Cookie of the flow entry that was looked up*/

    uint32_t device_id;
#if 0
#ifdef POF_MULTIPLE_SLOTS
    uint16_t slotID;
    uint16_t port_id;
#else // POF_MULTIPLE_SLOTS
    uint8_t pad[4];   /*8 bytes aligned*/
#endif // POF_MULTIPLE_SLOTS
#endif // caiqishen 00219933 要求packetIn带有port信息。
    uint16_t slotID;
    uint16_t port_id;

    char    data[POF_PACKET_IN_MAX_LENGTH];
} pof_packet_in;    //sizeof=24 + 2048 = 2072

/* Describe the match struct, including the location, the length and the value. */
typedef struct pof_match{
    uint16_t field_id;  /*0xffff means metadata, 
                          0x8XXX means from table parameter,
                          otherwise means from packet data. */
    uint16_t offset; /*bit unit*/
    uint16_t len;   /*length in bit unit*/
    uint8_t pad[2];   /*8 bytes aligned*/
}pof_match;         //sizeof=8
typedef struct pof_match_x{
    uint16_t field_id;  /*0xffff means metadata,
                          0x8XXX means from table parameter,
                          otherwise means from packet data. */
    uint16_t offset;  /*bit unit*/
    uint16_t len;    /*length in bit unit*/
    uint8_t pad[2];   /*8 bytes aligned*/

    uint8_t value[POF_MAX_FIELD_LENGTH_IN_BYTE];
    uint8_t mask[POF_MAX_FIELD_LENGTH_IN_BYTE];
}pof_match_x;       //sizeof=8+2*16=40

/* Discribe the instruction struct. */
typedef struct pof_instruction{
    uint16_t type;
    uint16_t len;
    uint8_t pad[4];   /*8 bytes aligned*/
#ifdef POF_SHT_VXLAN
    uint8_t  instruction_data[0];
#else // POF_SHT_VXLAN
    uint8_t  instruction_data[POF_MAX_INSTRUCTION_LENGTH];
                     /*Store the real instruction data such as "Goto-Table" */
#endif // POF_SHT_VXLAN
}pof_instruction;       //sizeof=8+(8+(6*(44+4)))=304

/* Describe the flow table struct including key length, table type.*/
typedef struct pof_flow_table{
    uint8_t command;
    uint8_t tid;              /*table ID*/
    uint8_t type;            /*table type*/
    uint8_t match_field_num;  /*the number of match fields.*/
    uint32_t size;            /*table size*/

    uint16_t key_len;         /*The max sum of length of all match fields*/
    uint16_t slotID;            /* For multiple slots. */
    uint8_t pad[4];             /*8 bytes aligned*/

    char table_name[POF_NAME_MAX_LENGTH];
	pof_match match[POF_MAX_MATCH_FIELD_NUM];
}pof_flow_table;        //size = 16 + 64 + 8*8 = 144

/* Discribe the flow entry struct. */
#ifdef POF_SHT_VXLAN
typedef struct pof_flow_entry{
    uint8_t command;
    uint8_t match_field_num;
    uint8_t pad[2];   /*8 bytes aligned*/
    uint32_t counter_id;

    uint64_t cookie;
    uint64_t cookie_mask;

    uint8_t table_id;
    uint8_t table_type;   /*table type: MM,LPM,EM,DT*/
    uint16_t idle_timeout;
    uint16_t hard_timeout;
    uint16_t priority;

    uint32_t  index;
    uint16_t slotID;            /* For multiple slots. */
    uint8_t pad2[2];   /*8 bytes aligned*/

    pof_match_x match[POF_MAX_MATCH_FIELD_NUM];    /*The match fields.  */
    /* The id of the target instruction block.
     * The value 0xFFFFFFFF means the instruction block is carried in this 
     * flow entry structure directly.*/
    uint16_t instruction_block_id;
    uint16_t parameter_length;  /* Length of the following parameters fields in unit of bit. */
    uint8_t pad3[4];
    uint8_t parameters[0];  /* The parameters of the flow instructions. */
}pof_flow_entry;        //sizeof=40+8*40+6*304=2184
#else // POF_SHT_VXLAN
typedef struct pof_flow_entry{
    uint8_t command;
    uint8_t match_field_num;
    uint8_t instruction_num;
    uint8_t pad[1];   /*8 bytes aligned*/
    uint32_t counter_id;

    uint64_t cookie;
    uint64_t cookie_mask;

    uint8_t table_id;
    uint8_t table_type;   /*table type: MM,LPM,EM,DT*/
    uint16_t idle_timeout;
    uint16_t hard_timeout;
    uint16_t priority;

    uint32_t  index;
    uint16_t slotID;            /* For multiple slots. */
    uint8_t pad2[2];   /*8 bytes aligned*/

    pof_match_x match[POF_MAX_MATCH_FIELD_NUM];    /*The match fields.  */
    pof_instruction instruction[POF_MAX_INSTRUCTION_NUM]; /*The instructions*/
}pof_flow_entry;        //sizeof=40+8*40+6*304=2184
#endif // POF_SHT_VXLAN

enum pof_enable {
    POFE_DISABLE    = 0,
    POFE_ENABLE     = 1,
};

#ifdef POF_SHT_VXLAN
/* instruction block message. */
struct pof_instruction_block{
    uint8_t command;    /* POFFC_*. */
    uint8_t instruction_num;
    uint8_t related_table_id;
    uint8_t pad;
    uint16_t slotID;
    uint16_t instruction_block_id;
    struct pof_instruction instruction[0];
};

/* Enable POF forwarding for one designated slot. */
struct pof_slot_config{
    uint16_t slotID;
    uint8_t flag;       /* 1: enable pof forwarding; 
                         * 0: disable pof forwarding. */
    uint8_t rsv[5];
};

enum pof_slot_status_enum {
    POFSS_DOWN  = 0,
    POFSS_UP    = 1,
};

enum pof_slot_resend_flag {
    POFSRF_NOT_SEND = 0,
    POFSRF_RE_SEND  = 1,
};

/* Slot status message from device to controller. */
struct pof_slot_status{
    uint16_t slotID;
    uint8_t slot_status;    /* 0: down; 1: up. */
    uint8_t resend_flag;    /* Request controller to re-send data.
                             * 0: notSend; 1: re send previous table/insBlock. */
    uint8_t rsv[4];
};
#endif // POF_SHT_VXLAN

/* Discribe a particular instruction struct with each type. */
typedef struct pof_instruction_goto_table{
    uint8_t next_table_id;
    uint8_t match_field_num;
    uint16_t packet_offset; /*pos of next layer (offset from current layer) , unit is byte. If is still in current layer, the packet_offset is 0 */
    uint8_t pad[4];   /*8 bytes aligned*/

    pof_match match[POF_MAX_MATCH_FIELD_NUM];
}pof_instruction_goto_table;    //sizeof=8+8*8 = 72

#ifdef POF_SD2N
typedef struct pof_instruction_goto_direct_table{
	uint8_t next_table_id;
	uint8_t index_type;		/* 0:immediate value; 1:packet/metadata field. */
	uint16_t packet_offset;
	uint8_t pad[4];

	union{
		uint32_t value;
		pof_match field;
	}table_entry_index;
}pof_instruction_goto_direct_table;		/* sizeof = 16. */
#else // POF_SD2N
typedef struct pof_instruction_goto_direct_table{
    uint8_t next_table_id;
    uint8_t pad;   /*8 bytes aligned*/
	uint16_t packet_offset;
    uint32_t table_entry_index;
}pof_instruction_goto_direct_table;     //sizeof= 8
#endif // POF_SD2N

typedef struct pof_instruction_meter{
#ifdef POF_SHT_VXLAN
    /* 0: immediate value; 1: comes from field. */
    uint8_t id_type;
    uint8_t rsv[7];
    union{
        uint32_t value;
        pof_match field;
    } meter_id;
#else // POF_SHT_VXLAN
    uint32_t   meter_id;
    uint8_t pad[4];   /*8 bytes aligned*/
#endif // POF_SHT_VXLAN
}pof_instruction_meter; //sizeof= 8

typedef struct pof_instruction_write_metadata{
    uint16_t metadata_offset; /*bit unit*/
    uint16_t len;  /*bit unit*/
#ifdef POF_SD2N_AFTER1015
    uint8_t value[POF_MAX_FIELD_LENGTH_IN_BYTE];    /* suport 128bit. */
    uint8_t pad[4];     /* 8 bytes aligned. */
#else // POF_SD2N_AFTER1015
    uint32_t value;
#endif // POF_SD2N_AFTER1015
}pof_instruction_write_metadata;    //sizeof= 24

typedef struct pof_instruction_write_metadata_from_packet{
    uint16_t metadata_offset;  /*bit unit*/
    uint16_t packet_offset;  /*bit unit*/
    uint16_t len;   /*bit unit*/
    uint8_t pad[2];   /*8 bytes aligned*/
}pof_instruction_write_metadata_from_packet;    //sizeof= 8

/* Describe the action struct. */
typedef struct pof_action{
    uint16_t type;
    uint16_t len;
#ifdef POF_SHT_VXLAN
    uint8_t rsv[4];
    uint8_t action_data[0];
#else // POF_SHT_VXLAN
    uint8_t  action_data[POF_MAX_ACTION_LENGTH];
#endif // POF_SHT_VXLAN
}pof_action;    //sizof=4+44=48, NOTES: POFAction header size is 4


/*Describe the packet out struct comes from controller*/
/* add by wenjian 2015/12/01*/
typedef struct pof_packet_out{
    uint32_t bufferId;
    uint32_t inPort;
    uint8_t actionNum;
    uint8_t padding[3];
    uint32_t packetLen;
    pof_action actionList[POF_MAX_ACTION_NUMBER_PER_INSTRUCTION];
    char data[POF_PACKET_IN_MAX_LENGTH];
}pof_packet_out;

/* Discribe a particular action struct with each type. */
typedef struct pof_instruction_apply_actions{
    uint8_t action_num;
    uint8_t pad[7];   /*8 bytes aligned*/
#ifdef POF_SHT_VXLAN
    pof_action action[0];
#else // POF_SHT_VXLAN
    pof_action  action[POF_MAX_ACTION_NUMBER_PER_INSTRUCTION];
#endif // POF_SHT_VXLAN
}pof_instruction_apply_actions; //sizeof=8+6*48=296

#ifdef POF_SD2N
/* This Conditional Jump instruction is used to move the PC based on the  
 * comparison result of two compared fields. If compare_field1 is less than
 * compare_field2, then jump to offset1; and if compare_field1 equals 
 * compare_field2, then jump to offset2; and if compare_field1 is greater
 * than compare_field2, then jump to offset3. Here, the offset to be jump to
 * is the number of instructions. */
typedef struct pof_instruction_conditional_jump{
	uint8_t field2_type;	/* The type of field2. Value mean immediate value 
							 * and value 1 means packet/metadata field. */
	uint8_t offset1_direction;	
							/* Jump direction. 0: forward; 1:backward. */
	uint8_t offset1_type;	/* The type of jump offset. Value 0 means immediate
							 * value and value 1means packet/metadata field. */
	uint8_t offset2_direction;
	uint8_t offset2_type;
	uint8_t offset3_direction;
	uint8_t offset3_type;

	uint8_t pad;

	struct pof_match compare_field1;
							/* The first packet/metadata field to be compared. */
	union{
		uint32_t value;
		struct pof_match field;
	}compare_field2;		/* The second value or packet/metadata field */

	union{
		uint32_t value;
		struct pof_match field;
	}offset1;				/* field1 < field2. */

	union{
		uint32_t value;
		struct pof_match field;
	}offset2;				/* field1 == field2. */

	union{
		uint32_t value;
		struct pof_match field;
	}offset3;				/* field1 > field2. */

}pof_instruction_conditional_jump;	/* sizeof = 48 byte. */

/* The following is the arithmetic and logic operation suported by  
 * CALCULATE_FIELD instruction. */
#define CALCULATIONS			\
	CALCULATION(ADD,0)			\
	CALCULATION(SUBTRACT,1)		\
	CALCULATION(LEFT_SHIFT,2)	\
	CALCULATION(RIGHT_SHIFT,3)	\
	CALCULATION(BITWISE_ADD,4)	\
	CALCULATION(BITWISE_OR,5)	\
	CALCULATION(BITWISE_XOR,6)	\
	CALCULATION(BITWISE_NOR,7)

enum pof_calc_type{
#define CALCULATION(NAME,VALUE) POFCT_##NAME = VALUE,
	CALCULATIONS
#undef CALCULATION
};

/* The Calculate Field instruction can support add/subtract/right shift/
 * left shift/and/or arithmetic and logic calculation. The destination
 * field to be calculated is either a packet field or a metadata field, and
 * the source operand can be an immediate value or another packet/metadata/
 * parameter field. */
#ifdef POF_SHT_VXLAN
typedef struct pof_action_calc_field{
	uint16_t calc_type;		/* POFCT_*, the calculation tpe. */
	uint8_t src_type;		/* The type of source operand. Value 0 means 
							 * immediate value and value 1 means packet/metadta
							 * field. */
	uint8_t pad[5];

	struct pof_match dst_field;		/* The destination packet/metadata field
									 * to be accumulated. */
	union{
		uint32_t value;
		struct pof_match src_field;
	}src_operand;			/* The source operand for the Increase Field
							 * instruction. */
}pof_action_calc_field;	/* sizeof = 24 byte. */
#endif // POF_SHT_VXLAN

typedef struct pof_instruction_calc_field{
	uint16_t calc_type;		/* POFCT_*, the calculation tpe. */
	uint8_t src_type;		/* The type of source operand. Value 0 means 
							 * immediate value and value 1 means packet/metadta
							 * field. */
	uint8_t pad[5];

	struct pof_match dst_field;		/* The destination packet/metadata field
									 * to be accumulated. */
	union{
		uint32_t value;
		struct pof_match src_field;
	}src_operand;			/* The source operand for the Increase Field
							 * instruction. */
}pof_instruction_calc_field;	/* sizeof = 24 byte. */

/* The Move Packet Offset instruction is used to move the packet offset 
 * forward or backward. The movement can be an immediate value or from a
 * packet/metadata/parameter field. */
typedef struct pof_instruction_mov_packet_offset{
	uint8_t direction;	/* 0: move forward; 1: move backward. */
	uint8_t value_type;	/* The type of movement value. Value 0 means index is
						 * immediate value and value 1 means index comes from 
						 * packet/metadata field. */
	uint8_t pad[6];

	union{
		uint32_t value;
		struct pof_match field;	/* Packet or Metadta field. */
	}movement;			/* The value to be moved, and it is byte unit. */
}pof_instruction_mov_packet_offset;		/* sizeof = 16 bytes. */
#endif //POF_SD2N

#ifdef POF_SHT_VXLAN
/* The 'Set packet Offset' instruction is used to set the packet window offset
 * to be new value. The new packet window offset can be an immediate value or
 * from a packet/metadta/parameter field. */
struct pof_instruction_set_packet_offset {
    /* The type of new packet window offset. Value 0 means new packet offset is
     * immediate value and value 1 means new packet offset comes from
     * packet/metadata field.*/
    uint8_t offset_type;
    uint8_t pad[7];
    union {
        int32_t value;
        pof_match field;
    } offset;               /* The new packet window offset, byte unit. */
};

enum pof_compare_result {
    POFCR_EQUAL     = 0,
    POFCR_BIGER     = 1,
    POFCR_SMALLER   = 2,
};

enum pof_compare_result_field {
    POFCR_FIELD_0   = 0,
    POFCR_FIELD_1   = 1,
    POFCR_FIELD_2   = 2,
    POFCR_FIELD_3   = 3,
};

/* Compare instruction. */
struct pof_instruction_compare {
    uint8_t operand2_type;
    /* There are four metadata fields used for storing compare result:
     * compare_result_0 ~ compare_result_3. User should designate which one to
     * be used for storing the compare result.
     * The compare result is defined as following:
     * 0: oprand1 = oprand2;
     * 1: oprand1 > oprand2;
     * 2: oprand1 < oprand2. */
    uint8_t comp_result_field_num;
    uint8_t rsv[6];
    pof_match operand1;     /* The first operand to be compared. */
    union {
        uint32_t value;
        pof_match field;
    } operand2;             /* The second operand to be compared. */
};

/* Branch instrucion. */
struct pof_instruction_branch {
    uint8_t operand2_type;
    uint8_t rsv[3];
    /* The instruction number to be skipped if opperand1 <> operand2. */
    uint32_t skip_instruction_num;
    pof_match operand1;   /* The first operand to be compared. */
    union {
        uint32_t value;
        pof_match field;
    } operand2;                 /* The second operand to be compared. */
};

/* Jmp instruction. */
struct pof_instruction_jmp {
    uint8_t direction;  /* 0: jump forward; 1: jump backward. */
    uint8_t rsv[3];
    uint32_t instruction_num;   /* The instruction number to be jumped over. */
};

/* Encap the VXLAN header. */
struct pof_action_encap_vxlan_header {
    uint8_t vni_type;
    uint8_t rsv[7];
    union {
        uint8_t value[3];   /* The least significant 0-24bit is valid. */
        pof_match field;
    } vni;
};

/* Encap the UDP header. */
struct pof_action_encap_udp_header {
    uint8_t sport_type;
    uint8_t dport_type;
    uint8_t rsv[6];
    union {
        uint16_t value;
        pof_match field;
    } sport;
    union {
        uint16_t value;
        pof_match field;
    } dport;
};

/* Encap the IP header. */
struct pof_action_encap_ip_header {
    uint8_t sip_type;
    uint8_t dip_type;
    uint8_t tos_type;
    uint8_t protocol;
    uint8_t rsv[4];
    union {
        uint8_t value;
        pof_match field;
    } tos;
    union {
        uint8_t value[4];
        pof_match field;
    } sip;
    union {
        uint8_t value[4];
        pof_match field;
    } dip;
};

/* Encap the MAC header. */
struct pof_action_encap_mac_header {
    uint8_t smac_type;
    uint8_t dmac_type;
    uint16_t ethType;
    uint8_t rsv[4];
    union {
        uint8_t value[6];   /* The least significant 0-48bit is valid. */
        pof_match field;
    } smac;
    union {
        uint8_t value[6];   /* The least significant 0-48bit is valid. */
        pof_match field;
    } dmac;
};
#endif // POF_SHT_VXLAN

typedef struct pof_action_set_field{
#ifdef POF_SHT_VXLAN
    pof_match dst_field;
    /* 0: immediate value; 1: comes from field. */
    uint8_t src_type;
    uint8_t pad[7];
    union{
        pof_match field;
        uint8_t value[POF_MAX_FIELD_LENGTH_IN_BYTE]; /* support 128bit. */
    } src;
#else // POF_SHT_VXLAN
    pof_match_x  field_setting;
#endif // POF_SHT_VXLAN
}pof_action_set_field;      //sizeof=pof_match_x.sizeof=40

typedef struct pof_action_set_field_from_metadata{
    pof_match  field_setting;
    uint16_t   metadata_offset; /*bit unit*/
    uint8_t pad[6];   /*8 bytes aligned*/
}pof_action_set_field_from_metadata;  //sizeof=8+8=16

typedef struct pof_action_modify_field{
    pof_match  field;
    int    increment; /*negative means minus.*/
    uint8_t    pad[4];   /*8 bytes aligned*/
}pof_action_modify_field;   //sizeof=8+8=16

typedef struct pof_action_add_field{

#ifdef POF_SHT_VXLAN
    uint16_t   tag_pos; /*the position to add the tag into packet, bit unit*/
    uint16_t   tag_len;  /*bit number, max length is 64*/
    /* 0: come from tag_value; 1: come from tag_field. */
    uint8_t tag_type;
    uint8_t rsv[3];
    union{
        uint8_t value[POF_MAX_FIELD_LENGTH_IN_BYTE];    /* Support 128bit. */
        pof_match field;    /* This is used only when tag_type equals 1. */
    } tag;

#else // POF_SHT_VXLAN
    uint16_t   tag_id;  /*protocol id*/
    uint16_t   tag_pos; /*the position to add the tag into packet, bit unit*/
    uint32_t tag_len;   /* bit number, max length is 64. */

#ifdef POF_SD2N_AFTER1015
    uint8_t    tag_value[POF_MAX_FIELD_LENGTH_IN_BYTE];
#else // POF_SD2N_AFTER1015
    uint64_t   tag_value;
#endif // POF_SD2N_AFTER1015

#endif // POF_SHT_VXLAN
}pof_action_add_field;    //sizeof=24

#ifdef POF_SD2N
typedef struct pof_action_delete_field{
    uint16_t   tag_pos;  /* the position of the tag to be deleted, bit unit*/
    uint8_t    len_type; /* length type:0 means length is immediate value, 
                          *  1 means length comes from packet/metadata field. */
    uint8_t    pad[5];   /*8 bytes aligned*/
    union{
        uint32_t value;
        pof_match field;
    }tag_len;            /* bit number, max length is 64. */
}pof_action_delete_field; //sizeof= 16
#else // POF_SD2N
typedef struct pof_action_delete_field{
    uint16_t   tag_pos;  /* the position of the tag to be deleted, bit unit*/
    uint8_t    pad[2];   /*8 bytes aligned*/
    uint32_t   tag_len;  /*bit number, max length is 64*/
}pof_action_delete_field; //sizeof= 8
#endif // POF_SD2N

#ifdef POF_SD2N
typedef struct pof_action_output{
	uint8_t portId_type;		/* 0:immediate value, 1:packet/metadata field. */
	uint8_t pad;
	uint16_t metadata_offset;	/* Metadata to be outputed, bit unit. */
	uint16_t metadata_len;		/* bit unit. */
	uint16_t packet_offset;		/* Packet data to be outputed, byte unit. */

	union{
		uint32_t value;         /* 16-31 bits are slot id, and the least significant
                                 * 16(0-15) bits are port id.*/
		pof_match field;
	}outputPortId;				/* The port ID from whichthe packet will be outputed. */
}pof_action_output;		/* sizeof = 24. */
#else // POF_SD2N
typedef struct pof_action_output{
    uint32_t   outputPortId;  /* The port  from which the packet will be outputted*/
    uint16_t   metadata_offset;  /*Metadata to be outputed, bit unit*/
    uint16_t   metadata_len;    /*bit unit*/

    uint16_t   packet_offset;     /*Packet data to be outputed, byte unit*/
    uint8_t    pad[6];
}pof_action_output;     //sizeof= 16
#endif // POF_SD2N

typedef struct pof_action_calculate_checksum{
#ifdef POF_SD2N_AFTER1015
    uint8_t checksum_pos_type;      /* 0 means packet; 1 means metadata. */
    uint8_t cal_startpos_type;      /* 0 means packet; 1 means metadata. */
    uint16_t   checksum_pos;      /*the position of checksum field, bit unit*/
    uint16_t   checksum_len;      /*checksum length, bit unit*/
    uint16_t   cal_startpos;          /*The start position of data to be calculated, bit unit*/
    uint16_t   cal_len;                 /*The length of data to be calculated, bit unit*/
    uint8_t pad[6];                 /* 8 bytes aligned. */
#else // POF_SD2N_AFTER1015
    uint16_t   checksum_pos;      /*the position of checksum field, bit unit*/
    uint16_t   checksum_len;      /*checksum length, bit unit*/
    uint16_t   cal_startpos;          /*The start position of data to be calculated, bit unit*/
    uint16_t   cal_len;                 /*The length of data to be calculated, bit unit*/
#endif // POF_SD2N_AFTER1015
}pof_action_calculate_checksum; //sizeof= 16

typedef struct pof_action_counter{
#ifdef POF_SHT_VXLAN
    uint8_t id_type;
    uint8_t rsv[7];
    union{
        uint32_t value;
        pof_match field;
    } counter_id;
#else // POF_SHT_VXLAN
    uint32_t   counter_id;
    uint8_t    pad[4];   /*8 bytes aligned*/
#endif // POF_SHT_VXLAN
}pof_action_counter;     //sizeof= 8

typedef struct pof_action_group{
    uint32_t   group_id;
    uint8_t    pad[4];   /*8 bytes aligned*/
}pof_action_group;      //szieof= 8

typedef struct pof_action_drop{
#ifdef POF_SHT_VXLAN
    uint8_t code_type;
    uint8_t rsv[7];
    union {
        uint32_t value;
        pof_match field;
    } reason_code;
#else // POF_SHT_VXLAN
    uint32_t    reason_code;
    uint8_t     pad[4];
#endif // POF_SHT_VXLAN
}pof_action_drop;

typedef struct pof_action_packet_in{
#ifdef POF_SHT_VXLAN
    uint8_t code_type;
    uint8_t rsv[7];
    union {
        uint32_t value;
        pof_match field;
    } reason_code;
#else // POF_SHT_VXLAN
    uint32_t    reason_code;
    uint8_t     pad[4];
#endif // POF_SHT_VXLAN
}pof_action_packet_in;

/* Describe the meter struct. */
typedef struct pof_meter{
    uint8_t command;
    uint8_t pad1;
#ifdef POF_MULTIPLE_SLOTS
    uint16_t slotID;    /* For multiple slots. */
    uint32_t meter_id;  /* Meter ID. */
    uint32_t rate;      /* old: uint16_t rate; old maxrate was 65536kbps = 64Mbps.
                         * which is too smal.*/
    uint8_t pad2[4];
#else // POF_MULTIPLE_SLOTS
    uint16_t rate;
    uint32_t   meter_id;  /*Meter ID*/
#endif // POF_MULTIPLE_SLOTS
}pof_meter;             //sizeof= 8

/* Describe the group struct. */
typedef struct pof_group{
    uint8_t command;
    uint8_t    type;  /*Group Type: all, indirect*/
    uint8_t    action_number;
    uint8_t    pad[1];   /*8 bytes aligned*/
    uint32_t   group_id;  /*Group ID*/

    uint32_t   counter_id;  /*packet counter, driver need add a ActionCount in action[]*/
    uint16_t   slotID;            /* For multiple slots. */
    uint8_t    pad2[2];
    pof_action  action[POF_MAX_ACTION_NUMBER_PER_GROUP];
}pof_group;     //sizeof=16+4*48=208

/* Describe the counter struct. */
typedef struct pof_counter{
    uint8_t command;
#ifdef POF_MULTIPLE_SLOTS
    uint8_t   pad;   /*8 bytes aligned*/
    uint16_t slotID;
#else // POF_MULTIPLE_SLOTS
    uint8_t pad[3];
#endif // POF_MULTIPLE_SLOTS
    uint32_t  counter_id;  /*packet counter.*/

    uint64_t  value;
#ifdef POF_SD2N
    uint64_t  byte_value;
#endif // POF_SD2N
}pof_counter;   //sizeof=24

/* Values for 'type' in pof_error_message. These values are immutable: they
* will not change in future versions of the protocol (although new values may
* be added). */
typedef enum pof_error_type {
    POFET_HELLO_FAILED = 0, /* Hello protocol failed. */
    POFET_BAD_REQUEST = 1, /* Request was not understood. */
    POFET_BAD_ACTION = 2, /* Error in action description. */
    POFET_BAD_INSTRUCTION = 3, /* Error in instruction list. */
    POFET_BAD_MATCH = 4, /* Error in match. */
    POFET_FLOW_MOD_FAILED = 5, /* Problem modifying flow entry. */
    POFET_GROUP_MOD_FAILED = 6, /* Problem modifying group entry. */
    POFET_PORT_MOD_FAILED = 7, /* Port mod request failed. */
    POFET_TABLE_MOD_FAILED = 8, /* Table mod request failed. */
    POFET_QUEUE_OP_FAILED = 9, /* Queue operation failed. */
    POFET_SWITCH_CONFIG_FAILED = 10, /* Switch config request failed. */
    POFET_ROLE_REQUEST_FAILED = 11, /* Controller Role request failed. */
    POFET_METER_MOD_FAILED = 12, /* Error in meter. */
    POFET_TABLE_FEATURES_FAILED = 13, /* Setting table features failed. */
    POFET_SOFTWARE_FAILED = 14, /* Software err, the err code is in pof_soft_error. */
    POFET_COUNTER_MOD_FAILED = 15, /* Error in counter. */
#ifdef POF_SHT_VXLAN
    POFET_INSBLOCK_MOD_FAILED = 16, /* Error in instruction block. */
#endif // POF_SHT_VXLAN
    POFET_EXPERIMENTER = 0xffff /* Experimenter error messages. */
} pof_error_type;

/* pof_error_msg 'code' values for POFET_HELLO_FAILED. 'data' contains an
* ASCII text string that may give failure details. */
enum pof_hello_failed_code {
    POFHFC_INCOMPATIBLE = 0, /* No compatible version. */
    POFHFC_EPERM = 1, /* Permissions error. */
};

/* pof_error_msg 'code' values for POFET_BAD_REQUEST. 'data' contains at least
* the first 64 bytes of the failed request. */
enum pof_bad_request_code {
    POFBRC_BAD_VERSION = 0, /* pof_header.version not supported. */
    POFBRC_BAD_TYPE = 1, /* pof_header.type not supported. */
    POFBRC_BAD_MULTIPART = 2, /* pof_multipart_request.type not supported. */
    POFBRC_BAD_EXPERIMENTER = 3, /* Experimenter id not supported
                                  * (in pof_experimenter_header or
                                  * pof_multipart_request or
                                  * pof_multipart_reply). */
    POFBRC_BAD_EXP_TYPE = 4, /* Experimenter type not supported. */
    POFBRC_EPERM = 5, /* Permissions error. */
    POFBRC_BAD_LEN = 6, /* Wrong request length for type. */
    POFBRC_BUFFER_EMPTY = 7, /* Specified buffer has already been used. */
    POFBRC_BUFFER_UNKNOWN = 8, /* Specified buffer does not exist. */
    POFBRC_BAD_TABLE_ID = 9, /* Specified table-id invalid or does not
                              * exist. */
    POFBRC_IS_SLAVE = 10, /* Denied because controller is slave. */
    POFBRC_BAD_PORT = 11, /* Invalid port. */
    POFBRC_BAD_PACKET = 12, /* Invalid packet in packet-out. */
    POFBRC_MULTIPART_BUFFER_OVERFLOW = 13, /* pof_multipart_request
                                            * overflowed the assigned buffer. */
};

/* pof_error_msg 'code' values for POFET_BAD_ACTION. 'data' contains at least
* the first 64 bytes of the failed request. */
enum pof_bad_action_code {
    POFBAC_BAD_TYPE = 0, /* Unknown action type. */
    POFBAC_BAD_LEN = 1, /* Length problem in actions. */
    POFBAC_BAD_EXPERIMENTER = 2, /* Unknown experimenter id specified. */
    POFBAC_BAD_EXP_TYPE = 3, /* Unknown action for experimenter id. */
    POFBAC_BAD_OUT_PORT = 4, /* Problem validating output port. */
    POFBAC_BAD_ARGUMENT = 5, /* Bad action argument. */
    POFBAC_EPERM = 6, /* Permissions error. */
    POFBAC_TOO_MANY = 7, /* Can't handle this many actions. */
    POFBAC_BAD_QUEUE = 8, /* Problem validating output queue. */
    POFBAC_BAD_OUT_GROUP = 9, /* Invalid group id in forward action. */
    POFBAC_MATCH_INCONSISTENT = 10, /* Action can't apply for this match,
    or Set-Field missing prerequisite. */
    POFBAC_UNSUPPORTED_ORDER = 11, /* Action order is unsupported for the
    action list in an Apply-Actions instruction */
    POFBAC_BAD_TAG = 12, /* Actions uses an unsupported
    tag/encap. */
    POFBAC_BAD_SET_TYPE = 13, /* Unsupported type in SET_FIELD action. */
    POFBAC_BAD_SET_LEN = 14, /* Length problem in SET_FIELD action. */
    POFBAC_BAD_SET_ARGUMENT = 15, /* Bad argument in SET_FIELD action. */
    POFBAC_BAD_TABLE_ID,
};

/* pof_error_msg 'code' values for POFET_BAD_INSTRUCTION. 'data' contains at least
* the first 64 bytes of the failed request. */
enum pof_bad_instruction_code {
    POFBIC_UNKNOWN_INST = 0, /* Unknown instruction. */
    POFBIC_UNSUP_INST = 1, /* Switch or table does not support the
    instruction. */
    POFBIC_BAD_TABLE_ID = 2, /* Invalid Table-ID specified. */
    POFBIC_BAD_TABLE_TYPE = 2, /* Invalid Table-ID specified. */
    POFBIC_UNSUP_METADATA = 3, /* Metadata value unsupported by datapath. */
    POFBIC_UNSUP_METADATA_MASK = 4, /* Metadata mask value unsupported by
    datapath. */
    POFBIC_BAD_EXPERIMENTER = 5, /* Unknown experimenter id specified. */
    POFBIC_BAD_EXP_TYPE = 6, /* Unknown instruction for experimenter id. */
    POFBIC_BAD_LEN = 7, /* Length problem in instructions. */
    POFBIC_EPERM = 8, /* Permissions error. */
    POFBIC_TOO_MANY_ACTIONS = 9, /*too many actions in one POFIT_APPLY_ACTIONS instruction*/
    POFBIC_TABLE_UNEXIST = 17, /* Flow table does not exist. */
    POFBIC_ENTRY_UNEXIST = 18, /* Flow entry does not exist in GOTO_DIRECT_TABLE action. */
	POFBIC_BAD_OFFSET = 19, /* Bad packet/metadata offset. */
	POFBIC_JUM_TO_INVALID_INST = 20, /* Jump to an invalid instruction. */
};

/* pof_error_msg 'code' values for POFET_BAD_MATCH. 'data' contains at least
* the first 64 bytes of the failed request. */
enum pof_bad_match_code {
    POFBMC_BAD_TYPE = 0, /* Unsupported match type specified by the match */
    POFBMC_BAD_LEN = 1, /* Length problem in match. */
    POFBMC_BAD_TAG = 2, /* Match uses an unsupported tag/encap. */
    POFBMC_BAD_DL_ADDR_MASK = 3, /* Unsupported datalink addr mask - switch
                                  * does not support arbitrary datalink
                                  * address mask. */
    POFBMC_BAD_NW_ADDR_MASK = 4, /* Unsupported network addr mask - switch
                                  * does not support arbitrary network address mask. */
    POFBMC_BAD_WILDCARDS = 5, /* Unsupported combination of fields masked
                               * or omitted in the match. */
    POFBMC_BAD_FIELD = 6, /* Unsupported field type in the match. */
    POFBMC_BAD_VALUE = 7, /* Unsupported value in a match field. */
    POFBMC_BAD_MASK = 8, /* Unsupported mask specified in the match,
                          * field is not dl-address or nw-address. */
    POFBMC_BAD_PREREQ = 9, /* A prerequisite was not met. */
    POFBMC_DUP_FIELD = 10, /* A field type was duplicated. */
    POFBMC_EPERM = 11, /* Permissions error. */
};

/* pof_error_msg 'code' values for POFET_FLOW_MOD_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum pof_flow_mod_failed_code {
    POFFMFC_UNKNOWN = 0, /* Unspecified error. */
    POFFMFC_TABLE_FULL = 1, /* Flow not added because table was full. */
    POFFMFC_BAD_TABLE_ID = 2, /* Table does not exist */
    POFFMFC_OVERLAP = 3, /* Attempted to add overlapping flow with
                          * CHECK_OVERLAP flag set. */
    POFFMFC_EPERM = 4, /* Permissions error. */
    POFFMFC_BAD_TIMEOUT = 5, /* Flow not added because of unsupported
                              * idle/hard timeout. */
    POFFMFC_BAD_COMMAND = 6, /* Unsupported or unknown command. */
    POFFMFC_BAD_FLAGS = 7, /* Unsupported or unknown flags. */
    POFFMFC_ENTRY_EXIST = 8, /* The specified flow entry has allready exist. */
    POFFMFC_ENTRY_UNEXIST = 9, /* The specified flow entry does not exist. */
    POFFMFC_BAD_TABLE_TYPE,
    POFFMFC_BAD_ENTRY_ID,
    POFFMFC_BAD_COUNTER_ID,
};

/* pof_error_msg 'code' values for POFET_FLOW_MOD_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum pof_table_mod_failed_code {
    POFTMFC_UNKNOWN = 0, /* Unspecified error. */
    POFTMFC_BAD_COMMAND = 1, /* Unsupported or unknown command. */
    POFTMFC_BAD_TABLE_TYPE = 2, /* Unsupported or unknown table type. */
    POFTMFC_BAD_TABLE_ID = 3, /* Table id is invalid. */
    POFTMFC_TABLE_EXIST,
    POFTMFC_TABLE_UNEXIST,
    POFTMFC_BAD_TABLE_SIZE,
    POFTMFC_BAD_KEY_LEN,
    POFTMFC_TABLE_UNEMPTY,
};

/* pof_error_msg 'code' values for POFET_GROUP_MOD_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum pof_group_mod_failed_code {
    POFGMFC_GROUP_EXISTS = 0, /* Group not added because a group ADD attempted to replace an already-present group. */
    POFGMFC_INVALID_GROUP = 1, /* Group not added because Group specified is invalid. */
    POFGMFC_WEIGHT_UNSUPPORTED = 2, /* Switch does not support unequal load sharing with select groups. */
    POFGMFC_OUT_OF_GROUPS = 3, /* The group table is full. */
    POFGMFC_OUT_OF_BUCKETS = 4, /* The maximum number of action buckets for a group has been exceeded. */
    POFGMFC_CHAINING_UNSUPPORTED = 5, /* Switch does not support groups that forward to groups. */
    POFGMFC_WATCH_UNSUPPORTED = 6, /* This group cannot watch the watch_port or watch_group specified. */
    POFGMFC_LOOP = 7, /* Group entry would cause a loop. */
    POFGMFC_UNKNOWN_GROUP = 8, /* Group not modified because a groups MODIFY attempted to modify a non-existent group. */
    POFGMFC_CHAINED_GROUP = 9, /* Group not deleted because another group is forwarding to it. */
    POFGMFC_BAD_TYPE = 10, /* Unsupported or unknown group type. */
    POFGMFC_BAD_COMMAND = 11, /* Unsupported or unknown command. */
    POFGMFC_BAD_BUCKET = 12, /* Error in bucket. */
    POFGMFC_BAD_WATCH = 13, /* Error in watch port/group. */
    POFGMFC_EPERM = 14, /* Permissions error. */
    POFGMFC_BAD_COUNTER_ID,
};

/* pof_error_msg 'code' values for POFET_METER_MOD_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum pof_meter_mod_failed_code {
    POFMMFC_UNKNOWN = 0, /* Unspecified error. */
    POFMMFC_METER_EXISTS = 1, /* Meter not added because a Meter ADD
                               * attempted to replace an existing Meter. */
    POFMMFC_INVALID_METER = 2, /* Meter not added because Meter specified is invalid. */
    POFMMFC_UNKNOWN_METER = 3, /* Meter not modified because a Meter
                                * MODIFY attempted to modify a non-existent Meter. */
    POFMMFC_BAD_COMMAND = 4, /* Unsupported or unknown command. */
    POFMMFC_BAD_FLAGS = 5, /* Flag configuration unsupported. */
    POFMMFC_BAD_RATE = 6, /* Rate unsupported. */
    POFMMFC_BAD_BURST = 7, /* Burst size unsupported. */
    POFMMFC_BAD_BAND = 8, /* Band unsupported. */
    POFMMFC_BAD_BAND_VALUE = 9, /* Band value unsupported. */
    POFMMFC_OUT_OF_METERS = 10, /* No more meters available. */
    POFMMFC_OUT_OF_BANDS = 11, /* The maximum number of properties for a meter has been exceeded. */
};

/* pof_error_msg 'code' values for POFET_METER_MOD_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum pof_port_mod_failed_code{
    POFPMFC_UNKNOWN = 0, /* Unspecified error. */
    POFPMFC_BAD_PORT_ID,
};

/* pof_error_msg 'code' values for POFET_METER_MOD_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum pof_counter_mod_failed_code{
    POFCMFC_UNKNOWN = 0, /* Unspecified error. */
    POFCMFC_BAD_COUNTER_ID = 1, /* Counter id is invalid. */
    POFCMFC_BAD_COMMAND, /* Unknown or unsupported command. */
    POFCMFC_COUNTER_UNEXIST, /* The specified counter does not exist. */
    POFCMFC_COUNTER_EXIST, /* The specified counter has existed. */
};

#ifdef POF_SHT_VXLAN
enum pof_insBlock_mod_failed_code{
    POFIMFC_UNKNOWN = 0,
    POFIMFC_BAD_INSBLOCK_ID = 1,
    POFIMFC_BAD_COMMAND,
    POFIMFC_INSBLOCK_UNEXIST,
    POFIMFC_INSBLOCK_EXIST,
};
#endif 

/*define error code in openflow software*/
typedef enum pof_soft_error{
    POF_OK = 0,

    POF_GET_PORT_INFO_FAILURE = 0X1000,
    POF_PACKET_LEN_ERROR = 0X1001,
    POF_METADATA_LEN_ERROR = 0X1002,
    POF_PARA_LEN_ERROR = 0X1003,
    POF_FIELD_LEN_ERROR = 0X1004,

    POF_ALLOCATE_RESOURCE_FAILURE = 0X5001,
    POF_ADD_EXIST_FLOW = 0X5002,
    POF_DELETE_UNEXIST_FLOW = 0X5003,
    POF_COUNTER_REQUEST_FAILURE = 0x5004,
    POF_DELETE_NOT_EMPTY_TABLE = 0X5005,
    POF_INVALID_SLOT_ID = 0x5006,

    POF_INVALID_TABLE_TYPE = 0X6000,
    POF_INVALID_KEY_LENGTH = 0X6001,
    POF_INVALID_TABLE_SIZE = 0X6002,
    POF_INVALID_MATCH_KEY = 0X6003,
    POF_UNSUPPORT_INSTRUTION_LENGTH = 0X6004,
    POF_UNSUPPORT_INSTRUTION_TYPE = 0X6005,
    POF_UNSUPPORT_ACTION_LENGTH = 0X6006,
    POF_UNSUPPORT_ACTION_TYPE = 0X6007,
    POF_TABLE_NOT_CREATED   = 0X6008,
    POF_UNSUPPORT_COMMAND = 0x6009,
    POF_UNSUPPORT_FLOW_TABLE_COMMAND = 0x600A,
    POF_UPWARD_TOO_LARGE_PACKET = 0x600B,
    POF_BAD_TABLE_ID = 0x600C,
    POF_BAD_TABLE_TYPE = 0x600D,
	POF_PORT_DELETE_FAIL = 0x600E,
    POF_BAD_INS_BLOCK_ID = 0x600F,


    POF_CREATE_SOCKET_FAILURE = 0X7001,
    POF_CONNECT_SERVER_FAILURE = 0X7002,
    POF_SEND_MSG_FAILURE = 0X7003,
    POF_RECEIVE_MSG_FAILURE = 0X7004,
    POF_WRONG_CHANNEL_STATE = 0X7005,
    POF_WRITE_MSG_QUEUE_FAILURE = 0X7006,
    POF_READ_MSG_QUEUE_FAILURE = 0X7007,
    POF_MESSAGE_SIZE_TOO_BIG = 0X7008,
    POF_BIND_SOCKET_FAILURE = 0X7009,
    POF_QUEUE_CREATE_FAIL = 0X700A,
    POF_QUEUE_DELETE_FAIL = 0X700B,
    POF_TASK_CREATE_FAIL = 0X700C,
    POF_TASK_DELETE_FAIL = 0X700D,
    POF_TIMER_CREATE_FAIL = 0X700E,
    POF_TIMER_DELETE_FAIL = 0X700F,
    POF_PTR_NULL =0X7010,

    POF_IPC_SEND_FAILURE = 0X8001,
    POF_ERROR = 0xffff
} pof_soft_error;

typedef struct pofec_error{
    uint16_t type;
    char type_str[POF_ERROR_STRING_MAX_LENGTH];
    uint16_t code;
    char error_str[POF_ERROR_STRING_MAX_LENGTH];
}pofec_error;

/* Error message. */
extern pofec_error g_pofec_error;

#define POF_DEBUG_CPRINT_ERR() \
	POF_ERROR_CPRINT_FL("[ERROR:] type = %s(%d), code = %s(0x%.4x)", g_pofec_error.type_str, \
	g_pofec_error.type, g_pofec_error.error_str, g_pofec_error.code)

#define POF_ERROR_HANDLE_RETURN_UPWARD(type,code,xid) \
    pofec_set_error(type,#type,code,#code); \
	POF_DEBUG_CPRINT_ERR(); \
    pofec_reply_error(type,code,#code,xid); \
    return code | 0xF0000000

#define POF_ERROR_HANDLE_RETURN_NO_UPWARD(type,code) \
    pofec_set_error(type,#type,code,#code); \
	POF_DEBUG_CPRINT_ERR(); \
    return code | 0xF0000000

#define POF_ERROR_HANDLE_NO_RETURN_UPWARD(type,code,xid) \
    pofec_set_error(type,#type,code,#code); \
	POF_DEBUG_CPRINT_ERR(); \
    pofec_reply_error(type,code,#code,xid)

#define POF_ERROR_HANDLE_NO_RETURN_NO_UPWARD(type,code) \
    pofec_set_error(type,#type,code,#code); \
	POF_DEBUG_CPRINT_ERR(); \

#define POF_ERROR_HANDLE_TERMINATE(type,code) \
    pofec_set_error(type,#type,coded,#code); \
	POF_DEBUG_CPRINT_ERR(); \
    terminate_handler()

#define POF_MALLOC_ERROR_HANDLE_RETURN_UPWARD(ptr,xid) \
    if((ptr) == NULL){ \
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE,xid); \
    }

#define POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(ptr) \
    if((ptr) == NULL){ \
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE); \
    }

#define POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret) \
    if(POF_OK != ret){ \
		POF_DEBUG_CPRINT_ERR(); \
        return ret; \
    }

#define POF_CHECK_RETVALUE_NO_RETURN_NO_UPWARD(ret) \
    if(POF_OK != ret){ \
		POF_DEBUG_CPRINT_ERR(); \
    }

#define POF_CHECK_RETVALUE_TERMINATE(ret) \
    if(POF_OK != ret){ \
		POF_DEBUG_CPRINT_ERR(); \
        terminate_handler(); \
    }

#define POF_CHECK_VALUES_ERROR_TERMINATE(value1, value2, terminate) \
    if(value1 == value2){                                           \
        POF_ERROR_CPRINT_FL(#value1" = "#value2);                   \
        terminate();                                                \
    }

/* Global variables. */
extern uint32_t g_recv_xid;
extern uint32_t g_upward_xid;

/* Task routine. */
typedef void (*POF_TASK_FUNC)(void *arg_ptr);

/* Timer routine. */
typedef void (*POF_TIMER_FUNC)(uint32_t timerid, int arg);

/* Basic function interface. */
extern uint32_t pofbf_task_create(void *arg, POF_TASK_FUNC task_func, task_t *task_id_ptr0);
extern uint32_t pofbf_task_delay(uint32_t delay);
extern uint32_t pofbf_task_delete(task_t *task_id_ptr);
extern uint32_t pofbf_queue_create(uint32_t *queue_id_ptr);
extern uint32_t pofbf_queue_delete( uint32_t *queue_id_ptr );
extern uint32_t pofbf_queue_read( uint32_t queue_id, void *buf, uint32_t max_len, int timeout);
extern uint32_t pofbf_queue_write( uint32_t queue_id, const void *message, uint32_t msg_len, int timeout);
extern uint32_t pofbf_timer_create(uint32_t delay, \
                              uint32_t interval, \
                              POF_TIMER_FUNC timer_handler, \
                              uint32_t *timer_id_ptr);
extern uint32_t pofbf_timer_delete(uint32_t *task_id_ptr);
extern void pofbf_cover_bit(uint8_t *data_ori, const uint8_t *value, uint16_t pos_b, uint16_t len_b);
extern void pofbf_copy_bit(const uint8_t *data_ori, uint8_t *data_res, uint16_t offset_b, uint16_t len_b);
extern void pofbf_split_str(char *strSrc, const char *split, char *strDst[], uint32_t count);
extern uint32_t pofsc_send_packet_upward(uint8_t *packet, uint32_t len);
extern void terminate_handler();

#endif

