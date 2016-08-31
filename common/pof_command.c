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
#include "../include/pof_command.h"
#include "../include/pof_datapath.h"
#include "../include/pof_hmap.h"
#include "../include/pof_memory.h"
#include <string.h>
#include <stdio.h>

#define CMD_ARG char *arg, struct pof_datapath *dp

static void
usr_cmd_memory(CMD_ARG)
{
#ifndef MALLOC_DEBUG
    return;
#endif // MALLOC_DEBUG
    MEMORY_PRINT_ALL();
}

static void
usr_cmd_addport(CMD_ARG)
{
    struct pof_local_resource *lr = NULL;
    char *arg_[2] = {
        NULL, NULL
    };
    uint16_t slot;
    if(!(dp->param.portFlag & POFLRPF_FROM_CUSTOM)){
        POF_COMMAND_PRINT(1,RED,"Add port Failed: only for port custom mode.\n");
        return;
    }
    pofbf_split_str(arg, ",", arg_, 2);
    if(!arg_[0]){
		POF_COMMAND_PRINT_HEAD("Need an argument. Eg. addport <name> <slotID>");
        return;
    }
	POF_COMMAND_PRINT_HEAD("addport %s,%s", arg_[0], arg_[1]);
#ifdef POF_MULTIPLE_SLOTS
    slot = arg_[1] ? atoi(arg_[1]) : POF_SLOT_ID_BASE;
#else // POF_MULTIPLE_SLOTS
    slot = POF_SLOT_ID_BASE;
#endif // POF_MULTIPLE_SLOTS
    if((lr = pofdp_get_local_resource(slot, dp)) == NULL){
        POF_COMMAND_PRINT(1,RED,"The slot ID %u is INVALID.\n", slot);
        return;
    }

	POF_LOG_LOCK_OFF;
    poflr_add_port(arg_[0], lr);
	POF_LOG_LOCK_ON;
    return;
}

static void
usr_cmd_delport(CMD_ARG)
{
    struct pof_local_resource *lr;
    char *arg_[2] = {
        NULL, NULL
    };
    uint16_t slot;
    if(!(dp->param.portFlag & POFLRPF_FROM_CUSTOM)){
        POF_COMMAND_PRINT(1,RED,"Delete port Failed: only for port custom mode.\n");
        return;
    }
    pofbf_split_str(arg, ",", arg_, 2);
    if(!arg_[0]){
		POF_COMMAND_PRINT_HEAD("Need an argument. Eg. delport <name> <slotID>");
        return;
    }
	POF_COMMAND_PRINT_HEAD("delport %s,%s", arg_[0], arg_[1]);
    slot = arg_[1] ? atoi(arg_[1]) : POF_SLOT_ID_BASE;
    if((lr = pofdp_get_local_resource(slot, dp)) == NULL){
        POF_COMMAND_PRINT(1,RED,"The slot ID %u is INVALID.\n", slot);
        return;
    }

    if(poflr_del_port(arg_[0], lr) != POF_OK){
        POF_COMMAND_PRINT(1,RED,"There is no port named %s on slot %s\n", arg_[0], arg_[1]);
    }
    return;
}

static void usr_cmd_clear_resource(CMD_ARG){
    struct pof_local_resource *lr, *next;
	POF_COMMAND_PRINT_HEAD("clear_resource");
    POF_LOG_LOCK_OFF;
    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
        poflr_clear_resource(lr);
    }
    POF_LOG_LOCK_ON;
	return;
}

static uint8_t usr_cmd_gets(char * cmd, size_t cmd_len, char **arg){
    char *cmd_;
	if(fgets(cmd, cmd_len, stdin) == NULL){
		return 0xff;
	}
	cmd[strlen(cmd) - 1] = 0;

    cmd_ = strtok(cmd, " ");
    *arg = strtok(NULL, " ");

    if(!cmd_){
        return POF_CMD_EMPTY;
    }
#define COMMAND(STRING) if(strcmp(cmd_,#STRING)==0) {return POFUC_##STRING;}
	COMMANDS
#undef COMMAND

    return POF_CMD_WRONG;
}

static void
usr_cmd_enable_debug(CMD_ARG)
{
	POF_COMMAND_PRINT_HEAD("enable_debug");
	SET_DBG_ENABLED();
    strncpy(g_states.debug_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
}

static void
usr_cmd_disable_debug(CMD_ARG)
{
	POF_COMMAND_PRINT_HEAD("disable_debug");
	SET_DBG_DISABLED();
    strncpy(g_states.debug_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
}

static void
usr_cmd_enable_color(CMD_ARG)
{
	POF_COMMAND_PRINT_HEAD("enable_color");
	SET_COLOR_ENABLED();
    strncpy(g_states.debug_color_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
}

static void
usr_cmd_disable_color(CMD_ARG)
{
	POF_COMMAND_PRINT_HEAD("disable_color");
	SET_COLOR_DISABLED();
    strncpy(g_states.debug_color_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
}

static void
usr_cmd_state(CMD_ARG)
{
	POF_COMMAND_PRINT_HEAD("state");
    poflp_states_print(&g_states);
}

static void
usr_cmd_test(CMD_ARG)
{
#define ARG_NUM_MAX    (4)
    uint32_t i;
    char *arg_[ARG_NUM_MAX] = {0};
    pofbf_split_str(arg, ",", arg_, ARG_NUM_MAX);
	POF_COMMAND_PRINT_HEAD("test");
    for(i=0; i<ARG_NUM_MAX; i++){
        if(arg_[i]){
            POF_COMMAND_PRINT_HEAD("arg[i] = %s", arg_[i]);
        }
    }
#undef ARG_NUM_MAX
    return;
}

static void usr_cmd_version(CMD_ARG){
    cmdPrintVersion(POFSWITCH_VERSION);
}

static void
cmdPrintHelp()
{
	POF_COMMAND_PRINT_HEAD("help");
#define COMMAND(STRING) POF_COMMAND_PRINT(1,PINK,#STRING"\n");
	COMMANDS
#undef COMMAND
}

static void usr_cmd_help(CMD_ARG){
    cmdPrintHelp();
}

static void usr_cmd_switch_config(CMD_ARG){
    pof_switch_config *pofsc_ptr = NULL;
    poflr_get_switch_config(&pofsc_ptr);
    cmdPrintSwitchConfig(pofsc_ptr);

    return;
}


static void usr_cmd_switch_feature(CMD_ARG){
    pof_switch_features *p = NULL;
    poflr_get_switch_feature(&p);
    cmdPrintFeature(p);
    return;
}

static void usr_cmd_table_resource(CMD_ARG){
    pof_flow_table_resource *p = NULL;
    poflr_get_flow_table_resource(&p);
    cmdPrintTableResource(p);
    return;
}

static void 
cmdPrintFlowEntrys(const struct tableInfo *table)
{
    struct entryInfo *entry, *next;
    HMAP_NODES_IN_STRUCT_TRAVERSE(entry, next, node, table->entryMap){
        cmdPrintFlowEntryBaseinfo(entry);
    }
}

static void 
cmdPrintFlowTable(const struct tableInfo *table)
{
    cmdPrintFlowTableBaseinfo(table);
    cmdPrintFlowEntrys(table);
}

static void 
cmdPrintFlowTables(CMD_ARG)
{
    struct tableInfo *table, *next;
    struct pof_local_resource *lr, *lrNext;
    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        POF_COMMAND_PRINT(1,PINK,"\n[Slot %d]\n", lr->slotID);
        HMAP_NODES_IN_STRUCT_TRAVERSE(table, next, idNode, lr->tableIdMap){
            cmdPrintFlowTable(table);
        }
    }
}

#ifdef POF_SHT_VXLAN
static void 
cmdPrintInsBlocks(CMD_ARG)
{
    struct insBlockInfo *insBlock, *next;
    struct pof_local_resource *lr, *lrNext;
    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        POF_COMMAND_PRINT(1,PINK,"\n[Slot %d]\n", lr->slotID);
        HMAP_NODES_IN_STRUCT_TRAVERSE(insBlock, next, idNode, lr->insBlockMap){
            cmdPrintInsBlock(insBlock);
        }
    }
}

static void
usr_cmd_blocks(CMD_ARG)
{
    POF_COMMAND_PRINT_HEAD("blocks");
    cmdPrintInsBlocks(arg, dp);
}
#endif // POF_SHT_VXLAN

static void usr_cmd_groups(CMD_ARG){
    struct groupInfo *group, *next;
    struct pof_local_resource *lr, *lrNext;

    POF_COMMAND_PRINT_HEAD("groups");
    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        POF_COMMAND_PRINT(1,PINK,"\n[Slot %d]\n", lr->slotID);
        HMAP_NODES_IN_STRUCT_TRAVERSE(group, next, idNode, lr->groupMap){
            cmdPrintGroup(group);
        }
    }
}

static void usr_cmd_meters(CMD_ARG){
    struct meterInfo *meter, *next;
    struct pof_local_resource *lr, *lrNext;

    POF_COMMAND_PRINT_HEAD("meters");
    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        POF_COMMAND_PRINT(1,PINK,"\n[Slot %d]\n", lr->slotID);
        HMAP_NODES_IN_STRUCT_TRAVERSE(meter, next, idNode, lr->meterMap){
            cmdPrintMeter(meter);
        }
    }
}

static void usr_cmd_counters(CMD_ARG){
    struct counterInfo *counter, *next;
    struct pof_local_resource *lr, *lrNext;

    POF_COMMAND_PRINT_HEAD("counter");
    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        POF_COMMAND_PRINT(1,PINK,"\n[Slot %d]\n", lr->slotID);
        HMAP_NODES_IN_STRUCT_TRAVERSE(counter, next, idNode, lr->counterMap){
            cmdPrintCounter(counter);
        }
    }
}

void usr_cmd_tables(CMD_ARG){
	POF_COMMAND_PRINT_HEAD("tables");
    cmdPrintFlowTables(arg, dp);
}

static void usr_cmd_ports(CMD_ARG){
    struct portInfo *p, *next;
    struct pof_local_resource *lr, *lrNext;

    POF_COMMAND_PRINT_HEAD("ports");
    HMAP_NODES_IN_STRUCT_TRAVERSE(lr, lrNext, slotNode, dp->slotMap){
        POF_COMMAND_PRINT(1,PINK,"\n[Slot %d]\n", lr->slotID);
        HMAP_NODES_IN_STRUCT_TRAVERSE(p, next, pofIndexNode, lr->portPofIndexMap){
            cmdPrintPort(p);
        }
    }
    return;
}

static void
usr_cmd_quit()
{
	POF_COMMAND_PRINT_HEAD("quit");
	return;
}

static void
usr_cmd_exit()
{
	POF_COMMAND_PRINT_HEAD("exit");
	return;
}

static void
usr_cmd_enable_promisc(CMD_ARG)
{
    dp->filter = dp->promisc;
	POF_COMMAND_PRINT_HEAD("Enable PROMISC.");
    strncpy(g_states.promisc_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
	return;
}

static void
usr_cmd_disable_promisc(CMD_ARG)
{
    dp->filter = dp->no_promisc;
	POF_COMMAND_PRINT_HEAD("Disable PROMISC.");
    strncpy(g_states.promisc_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
	return;
}

void pof_runing_command(struct pof_datapath *dp){
    char cmd[POF_CMD_STR_LEN] = "";
	uint8_t usr_cmd_type;
    char *arg = NULL;
    sleep(1);
    while(1){
#ifdef MALLOC_DEBUG
        MEMORY_PRINT();
#endif // MALLOC_DEBUG
        POF_COMMAND_PRINT_NO_LOGFILE(1,BLUE,"pof_command >>");

		usr_cmd_type = usr_cmd_gets(cmd, POF_CMD_STR_LEN, &arg);
		if(usr_cmd_type == POFUC_quit || usr_cmd_type == POFUC_exit){
			return;
		}

	    POF_LOG_LOCK_ON;
        switch(usr_cmd_type){
#define COMMAND(STRING) \
			case POFUC_##STRING:	\
				usr_cmd_##STRING(arg, dp); \
				break;

			COMMANDS
#undef COMMAND
            case POF_CMD_EMPTY:
                break;
            default:
				POF_COMMAND_PRINT_HEAD("wrong command: %s", cmd);
                POF_COMMAND_PRINT(1,RED,"Input 'help' to find commands!\n");
                break;
        }
	    POF_LOG_LOCK_OFF;
    }
}
