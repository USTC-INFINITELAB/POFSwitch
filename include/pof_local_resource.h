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

#ifndef _POF_LOCALRESOURCE_H_
#define _POF_LOCALRESOURCE_H_

#include "pof_common.h"
#include "pof_hmap.h"
#include "pof_tree.h"
#include "pof_list.h"

/* The table numbers of each type. */
#define POFLR_MM_TBL_NUM   (10)
#define POFLR_LPM_TBL_NUM   (10)
#define POFLR_EM_TBL_NUM    (5)
#define POFLR_DT_TBL_NUM    (20)

/* Max size of flow table. */
#define POFLR_FLOW_TABLE_SIZE (8000)

/* The numbers of meter, counter and group. */
#define POFLR_METER_NUMBER (256)
#define POFLR_COUNTER_NUMBER (512)
#define POFLR_GROUP_NUMBER (128)

/* Max number of local physical port. */
#define POFLR_DEVICE_PORT_NUM_MAX (100)

/* Max key length. */
#define POFLR_KEY_LEN (160)

/* Max instruction block number. */
#define POFLR_INS_BLOCK_NUM     (64)

//add by wenjian
//date 2015/12/02
/* Max queues per port*/
#define PORT_MAX_QUEUES (8)

#define PORT_NAME_LEN   POF_NAME_MAX_LENGTH
#define TABLE_NAME_LEN  POF_NAME_MAX_LENGTH

#ifdef POF_SHT_VXLAN
struct insBlockInfo{
    uint16_t blockID;
    struct hnode idNode;

    uint8_t insNum;
    uint8_t tableID;
    struct pof_local_resource *lr;
    struct pof_instruction insData[0];
};
#endif 

struct entryInfo{
    uint32_t  index;
    struct hnode node;
    uint32_t counter_id;
#ifdef POF_SHT_VXLAN
    uint16_t insBlockID;
#else // POF_SHT_VXLAN
    uint8_t instruction_num;
    pof_instruction instruction[POF_MAX_INSTRUCTION_NUM]; /*The instructions*/
#endif // POF_SHT_VXLAN

    uint16_t priority;
    uint16_t keyLen;

    uint8_t match_field_num;
    struct pof_match_x match[POF_MAX_MATCH_FIELD_NUM];
    uint8_t value[POF_MAX_FIELD_LENGTH_IN_BYTE * POF_MAX_MATCH_FIELD_NUM];
    uint8_t mask[POF_MAX_FIELD_LENGTH_IN_BYTE * POF_MAX_MATCH_FIELD_NUM];
#ifdef POF_SHT_VXLAN
    uint16_t paraLen;
    uint8_t para[0];
#endif // POF_SHT_VXLAN
};

struct tableInfo{
    uint8_t id;         /* Global value. */
    struct hnode idNode;
    uint8_t type;
//    struct hnode typeNode;
    char name[TABLE_NAME_LEN];

    struct hmap *entryMap;
    uint32_t entryNum;
    uint32_t size;
    uint16_t keyLen;

    /* Only For LPM. */
    struct tree *tree;

    uint8_t match_field_num;
    pof_match match[POF_MAX_MATCH_FIELD_NUM];
};

struct groupInfo{
    uint8_t type;
    uint8_t action_number;
    uint32_t id;
    struct hnode idNode;

    uint32_t counter_id;
    pof_action action[POF_MAX_ACTION_NUMBER_PER_GROUP];
};

struct meterInfo{
    uint32_t rate;
    uint32_t id;
    struct hnode idNode;
};

struct counterInfo{
    uint32_t id;
    struct hnode idNode;
    uint64_t value;
#ifdef POF_SD2N
    uint64_t byte_value;
#endif // POF_SD2N
};
//add by wenjian 2015/12/02
enum portFlags {
    NETDEV_UP = 0x0001,         /* Device enabled? */
    NETDEV_PROMISC = 0x0002,    /* Promiscuous mode? */
    NETDEV_CARRIER = 0x0004     /* Carrier detected? */
};

struct portInfo {
    struct hnode pofIndexNode;  /* One node in Hash map 
                                   pof_local_resource.portPofIndexMap. */
    struct hnode nameNode;      /* One node in Hash map 
                                   pof_local_resource.portNameMap */
    char        name[PORT_NAME_LEN];
    uint8_t     hwaddr[POF_ETH_ALEN];
    char        ip[POF_IP_ADDRESS_STRING_LEN];
    uint32_t    sysIndex;

    uint8_t     pofIndex;
    uint32_t    pofState;       /* Bitmap of POFPS_*.  */
    uint16_t    slotID;
    uint8_t     of_enable;
    task_t      taskID;

    //add by wenjian 2015/12/02
    int queue_fd[PORT_MAX_QUEUES+1];
    uint16_t num_queues;

    uint32_t config;
};

enum portFlag {
    POFLRPF_FROM_CUSTOM = 1 << 0,
};

struct pof_local_resource {
    uint16_t slotID;
    struct hnode slotNode;

    /* Port. */
    struct hmap *portPofIndexMap;   /* Hash map with portInfo.pofIndexNode. */
    struct hmap *portNameMap;       /* Hash map with portInfo.nameNode. */
    uint16_t portNumMax;
    uint16_t portNum;
    uint32_t portFlag;   /* POFLRPF_*. */

    /* Table. */
    struct hmap *tableIdMap;        /* Hash map with tableInfo.idNode. */
//    struct hmap *tableTypeMap;      /* Hash map with tableInfo.typeNode. */
    uint16_t tableNumMax;
    uint16_t tableNumMaxEachType[POF_MAX_TABLE_TYPE];
    uint16_t tableNum;
//    uint32_t tableFlag;
    uint32_t tableSizeMax;

    /* Group. */
    struct hmap *groupMap;          /* Hash map with groupInfo.idNode. */
    uint32_t groupNumMax;
    uint32_t groupNum;
//    uint32_t groupFlag;

    /* Meter. */
    struct hmap *meterMap;          /* Hash map with meterInfo.idNode. */
    uint32_t meterNumMax;
    uint32_t meterNum;
//    uint32_t meterFlag;

    /* Counter. */
    struct hmap *counterMap;        /* Hash map with counterInfo.idNode. */
    uint32_t counterNumMax;
    uint32_t counterNum;
//    uint32_t counterFlag;

#ifdef POF_SHT_VXLAN
    /* Instruction Block. */
    struct hmap *insBlockMap;   /* Hash map with insBlockInfo.idNode. */
    uint32_t insBlockNumMax;
    uint32_t insBlockNum;
#endif // POF_SHT_VXLAN
};

/* Switch ID. */
extern uint32_t g_poflr_dev_id;

/* Local_resource. */
extern uint32_t pof_localresource_init(struct pof_local_resource *);
extern uint32_t poflr_set_config(uint16_t flags, uint16_t miss_send_len);
extern uint32_t poflr_reply_config();
extern uint32_t poflr_reply_feature_resource(struct pof_local_resource *);
extern uint32_t poflr_reply_table_resource(struct pof_local_resource *);
#ifdef POF_SHT_VXLAN
extern uint32_t poflr_reply_slot_status(struct pof_local_resource *lr,  \
                                        enum pof_slot_status_enum status,    \
                                        enum pof_slot_resend_flag flag);
#endif // POF_SHT_VXLAN
extern uint32_t poflr_reply_port_resource(struct pof_local_resource *);
extern uint32_t poflr_reply_queryall(struct pof_local_resource *lr);
extern uint32_t poflr_clear_resource(struct pof_local_resource *);
extern uint32_t poflr_get_switch_config(pof_switch_config **config_ptrptr);
extern uint32_t poflr_get_switch_feature(pof_switch_features **feature_ptrptr);
extern uint32_t poflr_get_flow_table_resource(pof_flow_table_resource **flow_table_resource_ptrptr);
extern uint32_t poflr_init_table_resource(struct pof_local_resource *);
extern uint32_t poflr_reset_dev_id();

/* Port. */
extern uint32_t poflr_init_port(struct pof_local_resource *);
extern uint32_t poflr_port_detect_task();
extern uint32_t poflr_port_report(uint8_t reason, const struct portInfo *port);
extern uint32_t poflr_get_hwaddr_by_ipaddr(uint8_t *hwaddr, char *ipaddr_stri, struct pof_local_resource *);
extern uint32_t poflr_port_openflow_enable(uint32_t port_id, uint8_t ulFlowEnableFlg, \
                                           struct pof_local_resource *);
extern uint32_t poflr_disable_all_port(struct pof_local_resource *);
extern uint32_t poflr_del_port(const char *ethName, struct pof_local_resource *);
extern struct portInfo *poflr_get_port_with_pofindex(uint8_t pofIndex, const struct pof_local_resource *);
extern uint32_t poflr_ports_task_delete(struct pof_local_resource *);
extern uint32_t poflr_add_port(const char *name, struct pof_local_resource *lr);
extern uint32_t poflr_add_port_only_name(const char *name, uint16_t slotID);

/* Flow table. */
extern uint32_t poflr_create_flow_table(uint8_t id,                      \
                                        uint8_t type,                    \
                                        uint16_t key_len,                \
                                        uint32_t size,                   \
                                        char *name,                      \
                                        uint8_t match_field_num,         \
                                        pof_match *match,                \
                                        struct pof_local_resource *lr);
extern uint32_t poflr_delete_flow_table(uint8_t id,                      \
                                        uint8_t type,                    \
                                        struct pof_local_resource *lr);
extern uint32_t poflr_init_flow_table(struct pof_local_resource *);
extern uint32_t poflr_empty_flow_table(struct pof_local_resource *);
extern uint32_t poflr_table_ID_to_id(uint8_t table_ID,             \
                                     uint8_t *type_ptr,            \
                                     uint8_t *table_id_ptr,        \
                                     const struct pof_local_resource *);
extern uint32_t poflr_table_id_to_ID(uint8_t type,                 \
                                     uint8_t id,                   \
                                     uint8_t *ID_ptr,              \
                                     const struct pof_local_resource *);
extern struct tableInfo *poflr_get_table_with_ID(uint8_t, const struct pof_local_resource *);
extern uint32_t poflr_set_key_len(uint32_t key_len);

extern uint32_t poflr_add_flow_entry(pof_flow_entry *flow_ptr, struct pof_local_resource *);
extern uint32_t poflr_modify_flow_entry(pof_flow_entry *flow_ptr, struct pof_local_resource *);
extern uint32_t poflr_delete_flow_entry(pof_flow_entry *flow_ptr, struct pof_local_resource *);
extern struct entryInfo *poflr_entry_get_with_index(uint32_t index, const struct tableInfo *table);
extern struct entryInfo *poflr_entry_lookup_Linear(uint32_t index, const struct tableInfo *table);
extern struct entryInfo *poflr_entry_lookup(const uint8_t *packet,          \
                                            const uint8_t *metadata,        \
                                            const struct tableInfo *table);

/* Meter. */
extern uint32_t poflr_add_meter_entry(uint32_t meter_id, uint32_t rate, struct pof_local_resource *);
extern uint32_t poflr_modify_meter_entry(uint32_t meter_id, uint32_t rate, struct pof_local_resource *);
extern uint32_t poflr_delete_meter_entry(uint32_t meter_id, uint32_t rate, struct pof_local_resource *);
extern uint32_t poflr_init_meter(struct pof_local_resource *);
extern uint32_t poflr_empty_meter(struct pof_local_resource *);
extern struct meterInfo *poflr_get_meter_with_ID(uint32_t id, \
                    const struct pof_local_resource *lr);

/* Group. */
extern uint32_t poflr_modify_group_entry(pof_group *group_ptr, struct pof_local_resource *);
extern uint32_t poflr_add_group_entry(pof_group *group_ptr, struct pof_local_resource *);
extern uint32_t poflr_delete_group_entry(pof_group *group_ptr, struct pof_local_resource *);
extern uint32_t poflr_init_group(struct pof_local_resource *);
extern uint32_t poflr_empty_group(struct pof_local_resource *);
extern struct groupInfo *poflr_get_group_with_ID(uint32_t id, \
                    const struct pof_local_resource *lr);

/* Counter. */
extern uint32_t poflr_counter_init(uint32_t counter_id, struct pof_local_resource *);
extern uint32_t poflr_counter_delete(uint32_t counter_id, struct pof_local_resource *);
extern uint32_t poflr_counter_clear(uint32_t counter_id, struct pof_local_resource *);
extern uint32_t poflr_get_counter_value(uint32_t counter_id, struct pof_local_resource *);
#ifdef POF_SD2N
extern uint32_t poflr_counter_increace(uint32_t counter_id, uint16_t byte_len, struct pof_local_resource *);
#else // POF_SD2N
extern uint32_t poflr_counter_increace(uint32_t counter_id, struct pof_local_resource *);
#endif // POF_SD2N
extern uint32_t poflr_init_counter(struct pof_local_resource *);
extern uint32_t poflr_empty_counter(struct pof_local_resource *);
extern struct counterInfo *poflr_get_counter_with_ID(uint32_t id, \
                    const struct pof_local_resource *lr);

#ifdef POF_SHT_VXLAN
/* Instruction Block. */
uint32_t poflr_init_insBlock(struct pof_local_resource *lr);
uint32_t poflr_add_insBlock(struct pof_instruction_block *, struct pof_local_resource *);
uint32_t poflr_delete_insBlock(uint16_t blockID, struct pof_local_resource *lr);
uint32_t poflr_modify_insBlock(struct pof_instruction_block *, struct pof_local_resource *);
uint32_t poflr_empty_insBlock(struct pof_local_resource *lr);
struct insBlockInfo * poflr_get_insBlock_with_ID(uint32_t id, const struct pof_local_resource *lr);
#endif //POF_SHT_VXLAN

/* Reply the query. */
extern uint32_t poflr_reply_table(const struct pof_local_resource *lr, uint8_t id, uint8_t type);
extern uint32_t poflr_reply_table_all(const struct pof_local_resource *lr);
extern uint32_t poflr_reply_entry(const struct pof_local_resource *lr, uint8_t table_id, \
            uint8_t table_type, uint32_t index);
extern uint32_t poflr_reply_entry_all(const struct pof_local_resource *lr);
extern uint32_t poflr_reply_group(const struct pof_local_resource *lr, uint32_t groupID);
extern uint32_t poflr_reply_group_all(const struct pof_local_resource *lr);
extern uint32_t poflr_reply_meter(const struct pof_local_resource *lr, uint32_t meterID);
extern uint32_t poflr_reply_meter_all(const struct pof_local_resource *lr);
extern uint32_t poflr_reply_counter_all(const struct pof_local_resource *lr);

#endif // _POF_LOCALRESOURCE_H_
