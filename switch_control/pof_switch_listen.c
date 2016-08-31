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
#include "pof_command.h"
#include "pof_hmap.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/if.h>

#define LISTEN_PRINT(cont,...)          POF_DEBUG_CPRINT_FL(1,BLUE,"[Listen] "cont,##__VA_ARGS__);
#define LISTEN_PRINT_ERROR(cont,...)    POF_ERROR_CPRINT_FL("[Listen] "cont,##__VA_ARGS__);
#define LISTEN_ARG      char *arg, int sockfd, struct pof_datapath *dp

static uint32_t 
listen_help(LISTEN_ARG) {return POF_OK;}

static uint32_t
listen_quit(LISTEN_ARG) {return POF_OK;}

static uint32_t
listen_exit(LISTEN_ARG) {return POF_OK;}

static uint32_t
listen_switch_config(LISTEN_ARG)
{
    struct pof_switch_config *p = NULL;
    poflr_get_switch_config(&p);
    if(send(sockfd, p, sizeof(*p), 0) <= 0){
        return POF_ERROR;
    }
    return POF_OK;
}

static uint32_t
listen_switch_feature(LISTEN_ARG)
{
    struct pof_switch_features *p = NULL;
    poflr_get_switch_feature(&p);
    if(send(sockfd, p, sizeof(*p), 0) <= 0){
        return POF_ERROR;
    }
    return POF_OK;
}

static uint32_t
listen_table_resource(LISTEN_ARG)
{
    struct pof_flow_table_resource *p = NULL;
    poflr_get_flow_table_resource(&p);
    if(send(sockfd, p, sizeof(*p), 0) <= 0){
        return POF_ERROR;
    }
    return POF_OK;
}

static uint32_t
listen_ports(LISTEN_ARG)
{
    struct portInfo *p, *next;
    struct pof_local_resource *lr, *lrNext;
    struct responseHead respSlots[1] = {
        dp->slotNum, "slots"
    };

    if(send(sockfd, respSlots, sizeof(*respSlots), 0) <= 0){
        return POF_ERROR;
    }

    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        struct responseHead respPorts[1] = {
            lr->portNum, "ports"
        };
        if(send(sockfd, respPorts, sizeof(*respPorts), 0) <= 0){
            return POF_ERROR;
        }
        HMAP_NODES_IN_STRUCT_TRAVERSE(p, next, pofIndexNode, lr->portPofIndexMap){
            if(send(sockfd, p, sizeof(*p), 0) <= 0){
                return POF_ERROR;
            }
        }
    }

    return POF_OK;
}

static uint32_t
listen_tables(LISTEN_ARG)
{
    struct tableInfo *table, *tableNext;
    struct entryInfo *entry, *entryNexty;
    struct pof_local_resource *lr, *lrNext;;
    struct responseHead respSlots[] = {
        dp->slotNum, "slots"
    };

    if(send(sockfd, respSlots, sizeof(*respSlots), 0) <= 0){
        return POF_ERROR;
    }

    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        /* Send tableNum. */
        struct responseHead respTable[] = {
            lr->tableNum, "tables"
        };
        if(send(sockfd, respTable, sizeof(*respTable), 0) <= 0){
            return POF_ERROR;
        }

        /* Send all tables. */
        HMAP_NODES_IN_STRUCT_TRAVERSE(table, tableNext, idNode, lr->tableIdMap){
            /* Send one table. */
            if(send(sockfd, table, sizeof(*table), 0) <= 0){
                return POF_ERROR;
            }
            /* Send entryNum. */
            struct responseHead respEntry[] = {
                table->entryNum, "entries"
            };
            if(send(sockfd, respEntry, sizeof(*respEntry), 0) <= 0){
                return POF_ERROR;
            }
            /* Send all entries */
            HMAP_NODES_IN_STRUCT_TRAVERSE(entry, entryNexty, node, table->entryMap){
                /* Send one entry. */
                if(send(sockfd, entry, sizeof(*entry), 0) <= 0){
                    return POF_ERROR;
                }
            }
        }
    }

    return POF_OK;
}

static uint32_t
listen_groups(LISTEN_ARG)
{
    struct groupInfo *p, *next;
    struct pof_local_resource *lr, *lrNext;
    struct responseHead respSlots[1] = {
        dp->slotNum, "slots"
    };

    if(send(sockfd, respSlots, sizeof(*respSlots), 0) <= 0){
        return POF_ERROR;
    }

    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        struct responseHead resp[1] = {
            lr->groupNum, "groups"
        };
        if(send(sockfd, resp, sizeof(*resp), 0) <= 0){
            return POF_ERROR;
        }
        HMAP_NODES_IN_STRUCT_TRAVERSE(p, next, idNode, lr->groupMap){
            if(send(sockfd, p, sizeof(*p), 0) <= 0){
                return POF_ERROR;
            }
        }
    }
    return POF_OK;
}

static uint32_t
listen_meters(LISTEN_ARG)
{
    struct meterInfo *p, *next;
    struct pof_local_resource *lr, *lrNext;
    struct responseHead respSlots[1] = {
        dp->slotNum, "slots"
    };

    if(send(sockfd, respSlots, sizeof(*respSlots), 0) <= 0){
        return POF_ERROR;
    }

    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        struct responseHead resp[1] = {
            lr->meterNum, "meters"
        };

        if(send(sockfd, resp, sizeof(*resp), 0) <= 0){
            return POF_ERROR;
        }

        HMAP_NODES_IN_STRUCT_TRAVERSE(p, next, idNode, lr->meterMap){
            if(send(sockfd, p, sizeof(*p), 0) <= 0){
                return POF_ERROR;
            }
        }
    }
    return POF_OK;
}

static uint32_t
listen_counters(LISTEN_ARG)
{
    struct counterInfo *p, *next;
    struct pof_local_resource *lr, *lrNext;
    struct responseHead respSlots[1] = {
        dp->slotNum, "slots"
    };

    if(send(sockfd, respSlots, sizeof(*respSlots), 0) <= 0){
        return POF_ERROR;
    }

    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        struct responseHead resp[1] = {
            lr->counterNum, "counters"
        };

        if(send(sockfd, resp, sizeof(*resp), 0) <= 0){
            return POF_ERROR;
        }

        HMAP_NODES_IN_STRUCT_TRAVERSE(p, next, idNode, lr->counterMap){
            if(send(sockfd, p, sizeof(*p), 0) <= 0){
                return POF_ERROR;
            }
        }
    }
    return POF_OK;
}

static uint32_t
listen_version(LISTEN_ARG)
{
//    char p[POF_STRING_MAX_LEN] = POFSWITCH_VERSION;
//    if(send(sockfd, p, POF_STRING_MAX_LEN, 0) <= 0){
    if(send(sockfd, POFSWITCH_VERSION, POF_STRING_MAX_LEN, 0) <= 0){
        return POF_ERROR;
    }
    return POF_OK;
}

static uint32_t
listen_state(LISTEN_ARG)
{
    struct pof_state *p = &g_states;
    if(send(sockfd, p, sizeof(*p), 0) <= 0){
        return POF_ERROR;
    }
    return POF_OK;
}

static uint32_t
listen_enable_promisc(LISTEN_ARG)
{
    dp->filter = dp->promisc;
    strncpy(g_states.promisc_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
    return POF_OK;
}

static uint32_t
listen_disable_promisc(LISTEN_ARG)
{
    dp->filter = dp->no_promisc;
    strncpy(g_states.promisc_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
    return POF_OK;
}

static uint32_t
listen_addport(LISTEN_ARG)
{
    char *arg_[2] = {
        NULL, NULL
    };
    struct pof_local_resource *lr = NULL;
    uint16_t slot;
    if(!(dp->param.portFlag & POFLRPF_FROM_CUSTOM)){
        LISTEN_PRINT_ERROR("Add port Failed: only for port custom mode.");
        return POF_ERROR;
    }
    if(!arg){
        return POF_ERROR;
    }

    pofbf_split_str(arg, ",", arg_, 2);
    slot = arg_[1] ? atoi(arg_[1]) : POF_SLOT_ID_BASE;
    if((lr = pofdp_get_local_resource(slot, dp)) == NULL){
        return POF_ERROR;
    }

    poflr_add_port(arg_[0], lr);
    return POF_OK;
}

static uint32_t
listen_delport(LISTEN_ARG)
{
    char *arg_[2] = {
        NULL, NULL
    };
    struct pof_local_resource *lr = NULL;
    uint16_t slot;
    if(!(dp->param.portFlag & POFLRPF_FROM_CUSTOM)){
        LISTEN_PRINT_ERROR("Add port Failed: only for port custom mode.");
        return;
    }
    if(!arg){
        return;
    }

    pofbf_split_str(arg, ",", arg_, 2);
    slot = arg_[1] ? atoi(arg_[1]) : POF_SLOT_ID_BASE;
    if((lr = pofdp_get_local_resource(slot, dp)) == NULL){
        return POF_ERROR;
    }

    if(poflr_del_port(arg, lr)){
        LISTEN_PRINT_ERROR("There is no port named %s\n", arg);
    }
    return POF_OK;
}

static uint32_t
listen_test(LISTEN_ARG) {return POF_OK;}

static uint32_t
listen_clear_resource(LISTEN_ARG) {return POF_OK;}

static uint32_t
listen_enable_debug(LISTEN_ARG) {return POF_OK;}

static uint32_t
listen_disable_debug(LISTEN_ARG) {return POF_OK;}

static uint32_t
listen_enable_color(LISTEN_ARG) {return POF_OK;}

static uint32_t
listen_disable_color(LISTEN_ARG) {return POF_OK;}


static uint32_t
listen_memory(LISTEN_ARG) {return POF_OK;}

#ifdef POF_SHT_VXLAN
static uint32_t
listen_blocks(LISTEN_ARG) {return POF_OK;}
#endif // POF_SHT_VXLAN

static uint32_t 
tcpCreateSock(int *sockfd)
{
    if((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        LISTEN_PRINT_ERROR("Create Socket Failed!");
        return POF_ERROR;
    }
    return POF_OK;
}

static uint32_t
tcpBindSock(int sockfd, uint16_t port)
{
    int opt = 1;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = POF_HTONL(INADDR_ANY);
    addr.sin_port = POF_HTONS(port);

    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        LISTEN_PRINT_ERROR("Bind Socket Failed!");
        return POF_ERROR;
    }
    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
        LISTEN_PRINT_ERROR("Bind Socket Failed!");
        return POF_ERROR;
    }
    return POF_OK;
}

static uint32_t
tcpListen(int sockfd)
{
    if(listen(sockfd, 5) == -1){
        LISTEN_PRINT_ERROR("Listen Socket Failed!");
        return POF_ERROR;
    }
    return POF_OK;
}

static uint32_t
tcpAccept(int sockfd, int *fd)
{
    struct sockaddr_in addr;
    socklen_t size = sizeof(struct sockaddr_in);

    if((*fd = accept(sockfd, (struct sockaddr *)&addr, &size)) == -1){
        LISTEN_PRINT_ERROR("Accept TCP Failed!");
        return POF_ERROR;
    }
    return POF_OK;
}

static uint32_t
listenRecv(int sockfd, struct pof_datapath *dp)
{
    struct command cmdRecv[1] = {0};
    uint32_t recvLen;
    LISTEN_PRINT("Waiting for pofcommand from TCP...");
    while(1){
        memset(cmdRecv, 0, sizeof(struct command));
        if((recvLen = recv(sockfd, cmdRecv, sizeof(struct command), 0)) <= 0){
            LISTEN_PRINT("TCP for pofcommand disconnect.");
            return POF_OK;
        }
        switch(cmdRecv->cmd){
#define COMMAND(STRING)                                                                 \
            case POFUC_##STRING:                                                        \
                LISTEN_PRINT("Received a pofcommand : %s", #STRING); \
                if(listen_##STRING(cmdRecv->arg, sockfd, dp) != POF_OK){                \
                    return POF_OK;                                                      \
                }                                                                       \
                break;

            COMMANDS
#undef COMMAND
            default:
                LISTEN_PRINT("Wrong pofcommand (recvLen = %u): %u", recvLen, cmdRecv->cmd);
                break;
        }
    }
}

uint32_t 
pof_switch_listen_task(void *arg_ptr)
{
    uint32_t ret;
    int sockfd, fd;
    struct pof_datapath *dp = &g_dp;
    uint16_t port = dp->listenPort;

    /* TCP. */
    ret = tcpCreateSock(&sockfd);
    POF_CHECK_RETVALUE_TERMINATE(ret);
    ret = tcpBindSock(sockfd, port);
    POF_CHECK_RETVALUE_TERMINATE(ret);
    ret = tcpListen(sockfd);
    POF_CHECK_RETVALUE_TERMINATE(ret);

    while(1){
        LISTEN_PRINT("Waiting for pofcommand connection...");
        ret = tcpAccept(sockfd, &fd);
        if(ret != POF_OK){
            LISTEN_PRINT_ERROR("TCP Accecpt Failed!");
            break;
        }
        LISTEN_PRINT("pofcommand connected succefully!");
        listenRecv(fd, dp);
        close(fd);
    }
    close(sockfd);
    return POF_OK;
}
