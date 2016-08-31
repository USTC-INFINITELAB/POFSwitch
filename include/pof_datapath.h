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

#ifndef _POF_DATAPATH_H_
#define _POF_DATAPATH_H_

#include <linux/if_packet.h>
#include "pof_global.h"
#include "pof_local_resource.h"
#include "pof_common.h"

/* Max length of the raw packet received by local physical port. */
#define POFDP_PACKET_RAW_MAX_LEN    (2048)
#define POFDP_PACKET_PREBUF_LEN     (128)
#define PFODP_PACKET_BUF_TOTAL_LEN  (POFDP_PACKET_RAW_MAX_LEN + POFDP_PACKET_PREBUF_LEN)
/* Max length of the metadata. */
#define POFDP_METADATA_MAX_LEN (128)
/* The field offset of packet received port's ID infomation in metadata. */
#define POFDP_PORT_ID_FIELD_OFFSET_IN_METADATA_B (0)
/* The field length of packet received port's ID infomation in metadata. */
#define POFDP_PORT_ID_FIELD_LEN_IN_METADATA_B (1)
/* First table ID. */
#define POFDP_FIRST_TABLE_ID (0)
/* Define the metadata field id. */
#define POFDP_METADATA_FIELD_ID (0xFFFF)

#ifdef POF_SHT_VXLAN
/* Define the table parameter field id. The max four bits is 1000 (0x8). */
#define POFDP_PARA_FIELD_ID (0x8000)
#endif // POF_SHT_VXLAN

/* For TCP listening. */
#define POFDP_TCP_LISTEN_IP     "127.0.0.1"
#define POFDP_TCP_LISTEN_PORT   (6634)

#define POF_SLOT_ID_BASE    (0)
#define POF_SLOT_NUM        (1)
#define POF_SLOT_MAX        (16)

/**
#define POF_PACKET_REL_LEN_GET(DPP)         (0)
#define POF_PACKET_REL_LEN_SET(DPP, LEN)    (0)
#define POF_PACKET_REL_LEN_INC(DPP, LEN)    (0)
#define POF_PACKET_REL_LEN_DEC(DPP, LEN)    (0)
/*/
#define POF_PACKET_REL_LEN_GET(DPP) \
            POF_HTONS( (DPP)->metadata->len )

#define POF_PACKET_REL_LEN_SET(DPP, LEN)                    \
            (DPP)->metadata->len =                          \
            POF_HTONS( (LEN) )

#define POF_PACKET_REL_LEN_INC(DPP, LEN)                    \
            (DPP)->metadata->len =                          \
            POF_HTONS( POF_PACKET_REL_LEN_GET(DPP) + (LEN) )

#define POF_PACKET_REL_LEN_DEC(DPP, LEN)                    \
            (DPP)->metadata->len =                          \
            POF_HTONS( POF_PACKET_REL_LEN_GET(DPP) - (LEN) )
//*/

/* The bit number of one compare result field in metadata. */
#define POF_COMP_RES_FIELD_BITNUM     (2)


/* Packet infomation including data, length, received port. */
struct pofdp_packet{
    struct pof_datapath *dp;
    /* Input information. */
    uint32_t ori_port_id;       /* The original packet input port index. */
    uint32_t ori_len;           /* The original packet length.
                                 * If packet length has been changed, such as
                                 * in add field action, the ori_len will NOT
                                 * change. The len in metadata will update
                                 * immediatley in this situation. */
    uint8_t buf[POFDP_PACKET_RAW_MAX_LEN];  /* The memery which stores the whole packet. */
    uint8_t *packetBuf;         /* Points to the original packet buffer.
                                 * packetBuf = buf + POFDP_PACKET_PREBUF_LEN */

    /* Output. */
    uint16_t output_port_id;    /* The output port index. */
    uint16_t output_slot_id;
	uint16_t output_packet_len;			/* Byte unit. */
								/* The length of output packet. */
    uint16_t output_packet_offset;		/* Byte unit. */
								/* The start position of output. */
	uint16_t output_metadata_len;		/* Byte unit. */
	uint16_t output_metadata_offset;	/* Bit unit. */
								/* The output metadata length and offset. 
								 * Packet data output right behind the metadata. */
	uint16_t output_whole_len;  /* = output_packet_len + output_metadata_len. */
	uint8_t buf_out[POFDP_PACKET_RAW_MAX_LEN];	/* The memery which store the whole output data. 
                                                 * Including the metadata and
                                                 * the packet.*/
    uint8_t *output_packet_buf;      /* Points to the first byte of packet to output. */

    /* Offset. */
    int16_t offset;				/* Byte unit. Can be negative.*/
								/* The base offset of table field and actions. */
    uint8_t *buf_offset;        /* The packet pointer shift offset. 
                                 * buf_offset = packetBuf + offset */
    int32_t left_len;           /* Length of left packet after shifting offset. */

    /* Metadata. */
    struct pofdp_metadata *metadata;
                                /* The memery which stores the packet metadata. 
								 * The packet WHOLE length and input port index 
								 * has been stored in metadata. */
    uint16_t metadata_len;      /* The length of packet metadata in byte. */

    /* Flow. */
    uint8_t table_type;         /* Type of table which contains the packet now. */
    uint8_t table_id;           /* Index of table which contains the packet now. */
    struct entryInfo *flow_entry; /* The flow entry which match the packet. */

    /* Instruction & Actions. */
#ifdef POF_SHT_VXLAN
    struct insBlockInfo *insBlock;  /* Points to instruction block to be execute. */
#endif // POF_SHT_VXLAN
    struct pof_instruction *ins;/* The memery which stores the instructions need
                                 * to be implemented. */
    uint8_t ins_todo_num;       /* Number of instructions need to be implemented. */
	uint8_t ins_done_num;       /* Number of instructions have been done. */
    struct pof_action *act;     /* Memery which stores the actions need to be
                                 * implemented. */
    uint8_t act_num;            /* Number of actions need to be implemented. */
#ifdef POF_SHT_VXLAN
    uint8_t *para;              /* Parameter of the entry. */
    uint16_t paraLen;           /* The length of the parameter. */
#endif // POF_SHT_VXLAN
    uint8_t packet_done;        /* Indicate whether the packet processing is */
                                /* already done. 1 means done, 0 means not. */

	/* Meter. */
	uint16_t rate;				/* Rate. 0 means no limitation. */

	/* Socket. */
	int sockSend;
};

/* Define Metadata structure. */
struct pofdp_metadata{
    uint16_t len;
    uint8_t port_id;
    uint8_t reserve;

    uint8_t compRes;

    uint8_t data[];
};

struct pof_param {
    /* Port. */
    uint16_t portNumMax;
    uint32_t portFlag;      /* POFLRPF_*. */
    /* Tables. */
    uint16_t tableNumMaxEachType[POF_MAX_TABLE_TYPE];
    uint32_t tableSizeMax;
    /* Group, Meter, Counter. */
    uint32_t groupNumMax;
    uint32_t meterNumMax;
    uint32_t counterNumMax;
};

/* Define datapath struction. */
struct pof_datapath{
    /* NONE promisc packet filter function. */
    uint32_t (*no_promisc)(uint8_t *packet, struct portInfo *port_ptr, struct sockaddr_ll sll);

    /* Promisc packet filter function. */
    uint32_t (*promisc)(uint8_t *packet, struct portInfo *port_ptr, struct sockaddr_ll sll);

    /* Set RAW packet filter function. */
    uint32_t (*filter)(uint8_t *packet, struct portInfo *port_ptr, struct sockaddr_ll sll);

//   struct pof_local_resource resource;
    struct pof_param param;

    /* Slot resource. */
    struct hmap *slotMap;
    uint32_t slotNum;
    uint32_t slotMax;

    char listenIP[POF_IP_ADDRESS_STRING_LEN];
    uint16_t listenPort;

	uint32_t pktCount;
};

extern struct pof_datapath g_dp;

#define POFDP_ARG	struct pofdp_packet *dpp, struct pof_local_resource *lr

extern uint32_t pof_datapath_init(struct pof_datapath *dp);
extern uint32_t pofdp_slot_init(struct pof_datapath *dp);
extern struct pof_local_resource * \
           pofdp_get_local_resource(uint16_t slot, const struct pof_datapath *dp);
extern uint32_t pofdp_create_port_listen_task(struct portInfo *);
extern uint32_t pofdp_send_raw(struct pofdp_packet *dpp, const struct pof_local_resource *lr);
extern uint32_t pofdp_send_packet_in_to_controller(uint16_t len,        \
                                                   uint8_t reason,      \
                                                   uint8_t table_id,    \
                                                   uint32_t device_id,  \
                                                   uint8_t port_id,     \
                                                   uint16_t slotID,     \
                                                   uint8_t *packet);
extern uint32_t pofdp_instruction_execute(POFDP_ARG);
extern uint32_t pofdp_action_execute(POFDP_ARG);

extern uint32_t pofdp_write_32value_to_field(uint32_t value, const struct pof_match *pm, \
											 struct pofdp_packet *dpp);
extern uint32_t pofdp_get_32value(uint32_t *value, uint8_t type, void *u_, const struct pofdp_packet *dpp);
extern uint8_t *pofdp_get_field_buf(const struct pof_match *, const struct pofdp_packet *);
extern uint32_t pofdp_get_16value(uint16_t *value, uint8_t type, void *u_, const struct pofdp_packet *dpp);
#ifdef POF_SHT_VXLAN
extern uint32_t pofdp_get_value_byte(uint8_t *, uint16_t, uint8_t, void *, const struct pofdp_packet *);
extern uint32_t pofdp_get_value_byte_match(uint8_t *value,                  \
                                            uint16_t len_b,                 \
                                            struct pof_match field,         \
                                            const struct pofdp_packet *dpp);
#endif // POF_SHT_VXLAN

#endif // _POF_DATAPATH_H_
