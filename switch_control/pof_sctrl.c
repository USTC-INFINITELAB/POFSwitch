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
#include "pof_command.h"
#include "pof_type.h"
#include "pof_global.h"
#include "pof_conn.h"
#include "pof_log_print.h"
#include "pof_local_resource.h"
#include "pof_datapath.h"
#include "pof_byte_transfer.h"
#include "pof_switch_listen.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <signal.h>
#include <getopt.h>

struct pofsctrl {
    /* TCP connection. */
    char ip[POF_IP_ADDRESS_STRING_LEN];
    uint16_t port;
};

struct pofsctrl pofsctrl = {
    POFDP_TCP_LISTEN_IP,
    POFDP_TCP_LISTEN_PORT,
};

#define SCTRL_OK        (0)
#define SCTRL_QUIT      (1)
#define SCTRL_RECONN    (2)
#define SCTRL_ERROR     (3)
#define CMD_ARG  const char *cmdStr, const char *arg, int sockfd

static uint32_t
cmd_help(CMD_ARG)
{
#define COMMAND(STRING)                             \
            if(POFUC_##STRING > POFUC_test){        \
                return SCTRL_OK;                    \
            }                                       \
            POF_COMMAND_PRINT(1,PINK,#STRING"\n");
	COMMANDS
#undef COMMAND
    return SCTRL_OK;
}

static uint32_t
cmd_quit(CMD_ARG) {return SCTRL_QUIT;}

static uint32_t
cmd_exit(CMD_ARG) {return SCTRL_QUIT;}

static uint32_t
cmdRecv(int sockfd, void *p, uint32_t structLen)
{
    uint32_t recvLen;
    if((recvLen = recv(sockfd, p, structLen, 0)) <= 0){
        POF_ERROR_CPRINT_FL("Recv Response ERROR.");
        return SCTRL_RECONN;
    }
    if(recvLen != structLen){
        POF_ERROR_CPRINT_FL("Recv Length ERROR: structLen = %u, recvLen = %u", structLen, recvLen);
        return SCTRL_ERROR;
    }
    return SCTRL_OK;
}

static uint32_t
cmdSend(int sockfd, const struct command *cmd, const char *cmdStr)
{
    if(send(sockfd, cmd, sizeof(struct command), 0) <= 0){
        POF_ERROR_CPRINT_FL("Send Command %s ERROR.", cmdStr);
        return SCTRL_RECONN;
    }
    return SCTRL_OK;
}

static uint32_t
cmd_switch_config(CMD_ARG)
{
    struct command cmd[1] = {
        POFUC_switch_config, 0
    };
    struct pof_switch_config p[1] = {0};
    uint32_t ret;

    if( (ret = cmdSend(sockfd, cmd, cmdStr))   != SCTRL_OK || \
        (ret = cmdRecv(sockfd, p, sizeof(*p))) != SCTRL_OK ){
        return ret;
    }

    cmdPrintSwitchConfig(p);
    
    return SCTRL_OK;
}

static uint32_t
cmd_switch_feature(CMD_ARG)
{
    struct command cmd[] = {
        POFUC_switch_feature, 0
    };
    struct pof_switch_features p[] = {0};
    uint32_t ret;

    if( (ret = cmdSend(sockfd, cmd, cmdStr))   != SCTRL_OK || \
        (ret = cmdRecv(sockfd, p, sizeof(*p))) != SCTRL_OK ){
        return ret;
    }

    cmdPrintFeature(p);
    return SCTRL_OK;
}

static uint32_t
cmd_table_resource(CMD_ARG)
{
    struct command cmd[] = {
        POFUC_table_resource, 0
    };
    struct pof_flow_table_resource p[] = {0};
    uint32_t ret;

    if( (ret = cmdSend(sockfd, cmd, cmdStr))   != SCTRL_OK || \
        (ret = cmdRecv(sockfd, p, sizeof(*p))) != SCTRL_OK ){
        return ret;
    }

    cmdPrintTableResource(p);
    return SCTRL_OK;
}

static uint32_t
cmd_ports(CMD_ARG)
{
    struct command cmd[] = {
        POFUC_ports, 0
    };
    struct portInfo p[] = {0};
    struct responseHead respSlots[] = {0};
    struct responseHead respPorts[] = {0};
    uint32_t ret, i, j;

    if( (ret = cmdSend(sockfd, cmd, cmdStr)) != SCTRL_OK || \
        (ret = cmdRecv(sockfd, respSlots, sizeof(*respSlots))) != SCTRL_OK ){
        return ret;
    }

    for(i=0; i<respSlots->count; i++){
        if((ret = cmdRecv(sockfd, respPorts, sizeof(*respPorts))) != SCTRL_OK){
            return ret;
        }
        for(j=0; j<respPorts->count; j++){
            if((ret = cmdRecv(sockfd, p, sizeof(*p))) != SCTRL_OK){
                return ret;
            }
            cmdPrintPort(p);
        }
    }
    return SCTRL_OK;
}

static uint32_t
cmd_tables(CMD_ARG)
{
    struct command cmd[] = {
        POFUC_tables, 0
    };
    struct tableInfo table[] = {0};
    struct entryInfo entry[] = {0};
    struct responseHead respSlots[] = {0};
    struct responseHead respTable[] = {0};
    struct responseHead respEntry[] = {0};
    uint32_t ret, i, j, k;

    if( (ret = cmdSend(sockfd, cmd, cmdStr)) != SCTRL_OK || \
        (ret = cmdRecv(sockfd, respSlots, sizeof(*respSlots))) != SCTRL_OK ){
        return ret;
    }

    for(k=0; k<respSlots->count; k++){
        if((ret = cmdRecv(sockfd, respTable, sizeof(*respTable))) != SCTRL_OK){
            return ret;
        }
        /* Get all tables. */
        for(i=0; i<respTable->count; i++){
            /* Get one table. */
            if((ret = cmdRecv(sockfd, table, sizeof(*table))) != SCTRL_OK){
                return ret;
            }
            /* Print table. */
            cmdPrintFlowTableBaseinfo(table);
            /* Get entry number. */
            if((ret = cmdRecv(sockfd, respEntry, sizeof(*respEntry))) != SCTRL_OK){
                return ret;
            }
            /* Get all entries. */
            for(j=0; j<respEntry->count; j++){
                /* Get one entry. */
                if((ret = cmdRecv(sockfd, entry, sizeof(*entry))) != SCTRL_OK){
                    return ret;
                }
                /* Print entry. */
                cmdPrintFlowEntryBaseinfo(entry);
            }
        }
    }

    return SCTRL_OK;
}

static uint32_t
cmd_groups(CMD_ARG)
{
    struct command cmd[] = {
        POFUC_groups, 0
    };
    struct groupInfo p[] = {0};
    struct responseHead resp[] = {0};
    struct responseHead respSlots[] = {0};
    uint32_t ret, i, j;

    if( (ret = cmdSend(sockfd, cmd, cmdStr)) != SCTRL_OK || \
        (ret = cmdRecv(sockfd, respSlots, sizeof(*respSlots))) != SCTRL_OK ){
        return ret;
    }

    for(j=0; j<respSlots->count; j++){
        if((ret = cmdRecv(sockfd, resp, sizeof(*resp))) != SCTRL_OK){
            return ret;
        }
        for(i=0; i<resp->count; i++){
            if((ret = cmdRecv(sockfd, p, sizeof(*p))) != SCTRL_OK){
                return ret;
            }
            cmdPrintGroup(p);
        }
    }
    return SCTRL_OK;
}

static uint32_t
cmd_meters(CMD_ARG)
{
    struct command cmd[] = {
        POFUC_meters, 0
    };
    struct meterInfo p[] = {0};
    struct responseHead resp[] = {0};
    struct responseHead respSlots[] = {0};
    uint32_t ret, i, j;

    if( (ret = cmdSend(sockfd, cmd, cmdStr)) != SCTRL_OK || \
        (ret = cmdRecv(sockfd, respSlots, sizeof(*respSlots))) != SCTRL_OK ){
        return ret;
    }

    for(j=0; j<respSlots->count; j++){
        if((ret = cmdRecv(sockfd, resp, sizeof(*resp))) != SCTRL_OK){
            return ret;
        }
        for(i=0; i<resp->count; i++){
            if((ret = cmdRecv(sockfd, p, sizeof(*p))) != SCTRL_OK){
                return ret;
            }
            cmdPrintMeter(p);
        }
    }
    return SCTRL_OK;
}

static uint32_t
cmd_counters(CMD_ARG)
{
    struct command cmd[] = {
        POFUC_counters, 0
    };
    struct counterInfo p[] = {0};
    struct responseHead resp[] = {0};
    struct responseHead respSlots[] = {0};
    uint32_t ret, i, j;

    if( (ret = cmdSend(sockfd, cmd, cmdStr)) != SCTRL_OK || \
        (ret = cmdRecv(sockfd, respSlots, sizeof(*respSlots))) != SCTRL_OK ){
        return ret;
    }

    for(j=0; j<respSlots->count; j++){
        if((ret = cmdRecv(sockfd, resp, sizeof(*resp))) != SCTRL_OK){
            return ret;
        }
        for(i=0; i<resp->count; i++){
            if((ret = cmdRecv(sockfd, p, sizeof(*p))) != SCTRL_OK){
                return ret;
            }
            cmdPrintCounter(p);
        }
    }
    return SCTRL_OK;
}

static uint32_t
cmd_version(CMD_ARG)
{
    struct command cmd[] = {
        POFUC_version, 0
    };
    char p[POF_STRING_MAX_LEN];
    uint32_t ret;

    if( (ret = cmdSend(sockfd, cmd, cmdStr))   != SCTRL_OK || \
        (ret = cmdRecv(sockfd, p, POF_STRING_MAX_LEN)) != SCTRL_OK ){
        return ret;
    }

    cmdPrintVersion(p);
    return SCTRL_OK;
}

static uint32_t
cmd_state(CMD_ARG)
{
    struct command cmd[] = {
        POFUC_state, 0
    };
    struct pof_state p[] = {0};
    uint32_t ret;

    if( (ret = cmdSend(sockfd, cmd, cmdStr))   != SCTRL_OK || \
        (ret = cmdRecv(sockfd, p, sizeof(*p))) != SCTRL_OK ){
        return ret;
    }

    poflp_states_print(p);
    return SCTRL_OK;
}

static uint32_t
cmd_enable_promisc(CMD_ARG)
{
    struct command cmd[] = {
        POFUC_enable_promisc, 0
    };
    uint32_t ret;

    return cmdSend(sockfd, cmd, cmdStr);
}

static uint32_t
cmd_disable_promisc(CMD_ARG)
{
    struct command cmd[] = {
        POFUC_disable_promisc, 0
    };
    uint32_t ret;

    return cmdSend(sockfd, cmd, cmdStr);
}

static uint32_t
cmd_addport(CMD_ARG)
{
    struct command cmd[] = {
        POFUC_addport, {0}
    };
    uint32_t ret;

    if(!arg){
		POF_COMMAND_PRINT_HEAD("Need an argument. Eg. addport eth0");
        return SCTRL_OK;
    }
    strncpy(cmd->arg, arg, ARG_LEN);
    return cmdSend(sockfd, cmd, cmdStr);
}

static uint32_t
cmd_delport(CMD_ARG)
{
    struct command cmd[] = {
        POFUC_delport, {0}
    };
    uint32_t ret;

    if(!arg){
		POF_COMMAND_PRINT_HEAD("Need an argument. Eg. addport eth0");
        return SCTRL_OK;
    }
    strncpy(cmd->arg, arg, ARG_LEN);
    return cmdSend(sockfd, cmd, cmdStr);
}

static uint32_t
cmd_test(CMD_ARG)
{
    return SCTRL_OK;
}

static uint32_t
cmd_clear_resource(CMD_ARG) {return SCTRL_OK;}

static uint32_t
cmd_enable_debug(CMD_ARG) {return SCTRL_OK;}

static uint32_t
cmd_disable_debug(CMD_ARG) {return SCTRL_OK;}

static uint32_t
cmd_enable_color(CMD_ARG) {return SCTRL_OK;}

static uint32_t
cmd_disable_color(CMD_ARG) {return SCTRL_OK;}

static uint32_t
cmd_memory(CMD_ARG) {return SCTRL_OK;}

#ifdef POF_SHT_VXLAN
static uint32_t
cmd_blocks(CMD_ARG) {return SCTRL_OK;}
#endif // POF_SHT_VXLAN

static uint32_t 
tcpCreateSock(int *sockfd)
{
    if((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        POF_ERROR_CPRINT_FL("Create Socket Failed!");
        return POF_ERROR;
    }
    return POF_OK;
}

static uint32_t
tcpRecv(int fd, void *data, uint32_t lenMax, uint32_t *len)
{
    if((*len = recv(fd, data, lenMax, 0)) == -1){
        POF_ERROR_CPRINT_FL("Recv TCP Failed!");
        return POF_ERROR;
    }
    return POF_OK;
}

static uint32_t
tcpSend(int fd, void *data, uint32_t len)
{
    if(send(fd, data, len, 0) != len){
        POF_ERROR_CPRINT_FL("Send TCP Failed!");
        return POF_ERROR;
    }
}

static uint32_t
tcpConnect(int sockfd, uint16_t port, char *ip)
{
    struct sockaddr_in addr;
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = POF_HTONS(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if(connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
        return POF_ERROR;
    }
    return POF_OK;
}

static void
terminateHandler()
{
    POF_DEBUG_CPRINT_FL(1,RED,"Call terminate_handler!");
    exit(0);
}

static uint8_t 
cmdGet(char *cmd, char **arg)
{
    char *cmd_;
    uint8_t i;
	if(fgets(cmd, POF_CMD_STR_LEN, stdin) == NULL){
		return POF_CMD_WRONG;
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

    POF_COMMAND_PRINT_HEAD("wrong command: %s", cmd_);
    POF_COMMAND_PRINT(1,RED,"Input 'help' to find commands!\n");
    return POF_CMD_WRONG;
}

static uint32_t
cmd(int sockfd)
{
    uint32_t ret;
    uint8_t cmdType;
    char cmd[POF_CMD_STR_LEN] = {0}, *arg = NULL;
    while(1){
        POF_COMMAND_PRINT(1,BLUE,"pof_command >>");
        cmdType = cmdGet(cmd, &arg);
	    POF_COMMAND_PRINT_HEAD("%s", cmd);
        if(cmdType == POFUC_quit || cmdType == POFUC_exit){
            return SCTRL_QUIT;
        }

        switch(cmdType){
#define COMMAND(STRING)                                 \
            case POFUC_##STRING:                        \
                ret = cmd_##STRING(cmd, arg, sockfd);   \
                if(ret != SCTRL_OK){                    \
                    return ret;                         \
                }                                       \
                break;

            COMMANDS
#undef COMMAND
            default:
                break;
        }
    }
}


//  OPTIONS(OPT,OPTSTR,LONG,FUNC,HELP)
#define OPTIONS \
    OPTION('i',"i:","ip-addr",ip_addr,"(I)P address of the pofswitch. Default is 127.0.0.1") \
    OPTION('L',"L:","listen-port",listen_port,"Switch (l)istens to this port for pofsctrl. Default is 6634.") \
    OPTION('t',"t","test",test,"(T)est.")                                                       \
    OPTION('h',"h","help",help,"Print (h)elp message.")                                         \
    OPTION('v',"v","version",version,"Print the (v)ersion of POFSwitch.")

static uint32_t
opt_version(char *optarg)
{
    printf("Version: %s.%s\n", POFSwitch_VERSION_STR, POFSWITCH_VERSION_SUB);
    exit(0);
}

static uint32_t
opt_test(char *optarg)
{
    printf("Test is running!\n");
    if(optarg != NULL){
        printf("Your arg is %s\n", optarg);
    }else{
        printf("There is no arg.\n");
    }
    exit(0);
}

static uint32_t
opt_listen_port(char *optarg)
{
    if(optarg == NULL){
        return POF_ERROR;
    }
    uint16_t port = atoi(optarg);
    pofsctrl.port = port;
    return POF_OK;
}

static uint32_t
opt_ip_addr(char *optarg)
{
    if(optarg == NULL){
        return POF_ERROR;
    }
    strncpy(pofsctrl.ip, optarg, POF_IP_ADDRESS_STRING_LEN);
    return POF_OK;
}

static uint32_t
opt_help(char *optarg)
{
	printf("Usage: pofsctrl [options] [target] ...\n");
	printf("Options:\n");

#define OPTION(OPT,OPTSTR,LONG,FUNC,HELP) \
            printf("  -%c, --%-24s%s\n",OPT,LONG,HELP);
    OPTIONS
#undef OPTION

	printf("\nReport bugs to <yujingzhou@huawei.com>\n");
    exit(0);
}

static uint32_t
runOption(int argc, char *argv[])
{
	uint32_t ret = POF_OK;
#define OPTSTRING_LEN (100)
	char optstring[OPTSTRING_LEN] = {0};
 	struct option long_options[] = {
#define OPTION(OPT,OPTSTR,LONG,FUNC,HELP) {LONG,2,NULL,OPT},
        OPTIONS
#undef OPTION
		{NULL,0,NULL,0}
	};
	int ch;

#define OPTION(OPT,OPTSTR,LONG,FUNC,HELP) strncat(optstring,OPTSTR,OPTSTRING_LEN);
    OPTIONS
#undef OPTION

	while((ch=getopt_long(argc, argv, optstring, long_options, NULL)) != -1){
		switch(ch){
			case '?':
				exit(0);
				break;
#define OPTION(OPT,OPTSTR,LONG,FUNC,HELP)               \
            case OPT:                                   \
                if(opt_##FUNC(optarg) != POF_OK){       \
                    printf("Wrong Option: %c\n", ch);   \
                    opt_help(NULL);                     \
                }                                       \
                break;

            OPTIONS
#undef OPTION
			default:
				break;
		}
	}
#undef OPTSTRING_LEN
}

int
main(int argc, char *argv[])
{
    uint32_t ret;
    int sockfd;
    uint16_t port;
    char *ip;

    signal(SIGINT, terminateHandler);
    signal(SIGTERM, terminateHandler);

    ret = runOption(argc, argv);
    POF_CHECK_VALUES_ERROR_TERMINATE(ret, POF_ERROR, terminateHandler);

    port = pofsctrl.port;
    ip = pofsctrl.ip;

    /* TCP. */
    ret = tcpCreateSock(&sockfd);
    POF_CHECK_VALUES_ERROR_TERMINATE(ret, POF_ERROR, terminateHandler);

    POF_DEBUG_CPRINT_FL(1,BLUE,"Connecting to POFSwitch (ip=%s, port=%u)...", ip, port);

    while(1){
        ret = tcpConnect(sockfd, port, ip);
        if(ret != POF_OK){
            sleep(1);
            continue;
        }
        POF_DEBUG_CPRINT_FL(1,BLUE,"Connected!");

        ret = cmd(sockfd);
        if(ret == SCTRL_QUIT){
            break;
        }else if(ret == SCTRL_RECONN){
            continue;
        }else if(ret == SCTRL_ERROR){
            POF_ERROR_CPRINT_FL("ERROR");
            break;
        }
    }
    POF_DEBUG_CPRINT_FL(1,BLUE,"EXIT");
    close(sockfd);
    return POF_OK;
}

