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
#include "../include/pof_log_print.h"
#include "../include/pof_local_resource.h"
#include "../include/pof_byte_transfer.h"
#include "../include/pof_datapath.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

char g_versionStr[POF_STRING_MAX_LEN - 1] = "";

//struct pof_state g_states;
struct pof_state g_states = {
    {"DEVICE ID",""},
    {"VERSION",""},
#if (POF_OS_BIT == POF_OS_32)
    {"OS BIT","32"},
#elif (POF_OS_BIT == POF_OS_64)
    {"OS BIT","64"},
#endif
#if (POF_BYTE_ORDER == POF_BIG_ENDIAN)
    {"BYTE ORDER","BIG ENDIAN"},
#elif (POF_BYTE_ORDER == POF_LITTLE_ENDIAN)
    {"BYTE ORDER","LITTLE ENDIAN"},
#endif
#ifdef POF_DEBUG_ON
    {"DEBUG","ON"},
#else
    {"DEBUG","OFF"},
#endif 
#ifdef POF_DEBUG_COLOR_ON
    {"COLOR","ON"},
#else
    {"COLOR","OFF"},
#endif
#ifdef POF_DEBUG_PRINT_ECHO_ON
    {"ECHO PRINT","ON"},
#else
    {"ECHO PRINT","OFF"},
#endif
#ifdef POF_ERROR_PRINT_ON
    {"ERROR PRINT","ON"},
#else
    {"ERROR PRINT","OFF"},
#endif
    {"CONFIG FILE","NONE"},
#if (POF_PERFORM_AFTER_CTRL_DISCONN == POF_AFTER_CTRL_DISCONN_RECONN)
    {"AFTER DISCONNECTION","RE CONNECT"},
#elif (POF_PERFORM_AFTER_CTRL_DISCONN == POF_AFTER_CTRL_DISCONN_SHUT_DOWN)
    {"AFTER DISCONNECTION","SHUT DOWN"},
#endif
#if (POF_NOMATCH == POF_NOMATCH_DROP)
    {"NO MATCH","DROP"},
#elif (POF_NOMATCH == POF_NOMATCH_PACKET_IN)
    {"NO MATCH","PACKET IN"},
#endif
#ifdef POF_PROMISC_ON
    {"PROMISC","ON"},
#else
    {"PROMISC","OFF"},
#endif
    {"CONTROLLER IP",POF_CONTROLLER_IP_ADDR},
    {"CONNECTION PORT","6633"},
#ifdef POF_AUTOCLEAR
    {"AUTO CLEAR AFTER DISCONNECTION","ON"},
#else
    {"AUTO CLEAR AFTER DISCONNECTION","OFF"},
#endif
    {"LOG FILE","OFF"},
#ifdef POF_DEBUG_ON
    {"VERBOSITY MODE","DEBUG"},
#else
    {"VERBOSITY MODE","ERROR"},
#endif
    {"PORT MODE","SYSTEM"},
    {"SLOT NUMBER","1"},
    {"LISTEN PORT","6634"},
#ifdef POF_SD2N
    {"SD2N","ON"},
#else
    {"SD2N","OFF"},
#endif
#ifdef POF_SD2N_AFTER1015
    {"SD2N_AFTER1015","ON"},
#else
    {"SD2N_AFTER1015","OFF"},
#endif
#ifdef POF_MULTIPLE_SLOTS
    {"MULTIPLE SLOTS","ON"},
#else
    {"MULTIPLE SLOTS","OFF"},
#endif
#ifdef POF_SHT_VXLAN
    {"SHT VXLAN", "ON"},
#else
    {"SHT VXLAN", "OFF"},
#endif
};

static uint32_t readConfigFile(FILE *fp, struct pof_datapath *dp);

uint32_t pofc_cmd_auto_clear = TRUE;


#define COMMAND_STR_LEN (30)

//  CONFIG_CMD(OPT,OPTSTR,LONG,FUNC,HELP)
#define CONFIG_CMDS \
    CONFIG_CMD('i',"i:","ip-addr",ip_addr,"(I)P address of the Controller.")                        \
    CONFIG_CMD('p',"p:","port",port,"Connection (p)ort number. Default is 6633.")                   \
    CONFIG_CMD('a',"a:","add-port",add_port,"(A)dd a port to pofswitch to enable the port custom mode. Eg. -a eth0 -a eth1") \
    CONFIG_CMD('e',"e","empty",empty,"Start an (e)mpty switch without any port to enable the port custom mode.") \
    CONFIG_CMD('f',"f:","file",file,"Set config (f)ile.")                                           \
    CONFIG_CMD('d',"d:","device-id",device_id,"Set (d)evice id.")                                   \
    CONFIG_CMD('l',"l:","log-file",log_file,"Create (l)og file: /usr/local/var/log/pofswitch.log.") \
    CONFIG_CMD('s',"s","state",state,"Print software (s)tate information.")                         \
    CONFIG_CMD('P',"P","promisc",promisc,"Enable the (P)ROMISC mode. Default is diabled.")          \
    CONFIG_CMD('V',"V:","verbosity",verbosity,"(V)erbosity mode: debug|error|mute. Mute mode has no pof_command.") \
    CONFIG_CMD('L',"L:","listen-port",listen_port,"Switch (l)istens to this port for pofsctrl. Default is 6634.") \
    CONFIG_CMD('h',"h","help",help,"Print (h)elp message.")                                         \
    CONFIG_CMD('v',"v","version",version,"Print the (v)ersion of POFSwitch.")                       \
    CONFIG_CMD('S',"S:","slot-num",slot_num,"Set the number of (s)lots. Default is 1.")             \
    CONFIG_CMD('m',"m","man-clear",man_clear,"(M)anually clear the resource when disconnect.")      \
    CONFIG_CMD('t',"t","test",test,"(T)est.")

#define OPT_ARG char *optarg, struct pof_datapath *dp

static uint32_t
start_cmd_slot_num(OPT_ARG)
{
#ifdef POF_MULTIPLE_SLOTS
    uint16_t slotNum = atoi(optarg);
    dp->slotNum = slotNum;
    sprintf(g_states.slotNum.cont, "%u", slotNum);
    return POF_OK;
#else // POF_MULTIPLE_SLOTS
    printf("Multiple Slots is disabled.");
    return POF_ERROR;
#endif // POF_MULTIPLE_SLOTS
}

static uint32_t
start_cmd_listen_port(OPT_ARG)
{
    if(optarg == NULL){
        return POF_ERROR;
    }
    uint16_t port = atoi(optarg);
    dp->listenPort = port;
    sprintf(g_states.listenPort.cont, "%u", port);
    return POF_OK;
}

static uint32_t
start_cmd_device_id(OPT_ARG)
{
    uint32_t devID = strtoul(optarg, NULL, 16);
    g_poflr_dev_id = devID;
    sprintf(g_states.devID.cont, "0x%.8x", devID);
    return POF_OK;
}

static uint32_t
start_cmd_add_port(OPT_ARG)
{
    char *arg[2] = {
        NULL, NULL
    };
    uint16_t slot;

    dp->param.portFlag |= POFLRPF_FROM_CUSTOM;
    strncpy(g_states.port_mode.cont, "CUSTOM", POF_STRING_PAIR_MAX_LEN-1);
    pofbf_split_str(optarg, ",", arg, 2);
#ifdef POF_MULTIPLE_SLOTS
    slot = arg[1] ? atoi(arg[1]) : POF_SLOT_ID_BASE;
#else // POF_MULTIPLE_SLOTS
    slot = POF_SLOT_ID_BASE;
#endif // POF_MULTIPLE_SLOTS
    if(slot < POF_SLOT_ID_BASE || slot > (POF_SLOT_ID_BASE + dp->slotNum - 1)){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_INVALID_SLOT_ID);
    }

    return poflr_add_port_only_name(arg[0], slot);
}

static uint32_t
start_cmd_empty(OPT_ARG)
{
    dp->param.portFlag |= POFLRPF_FROM_CUSTOM;
    strncpy(g_states.port_mode.cont, "CUSTOM", POF_STRING_PAIR_MAX_LEN-1);
    return POF_OK;
}

static uint32_t
start_cmd_promisc(OPT_ARG)
{
    dp->filter = dp->promisc;
    strncpy(g_states.promisc_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
    return POF_OK;
}

static uint32_t
start_cmd_verbosity(OPT_ARG)
{
    if(optarg == NULL){
        return POF_ERROR;
    }
    if(strcmp(optarg, "debug") == POF_OK){
		SET_DBG_ENABLED();
		SET_ERR_ENABLED();
		SET_CMD_ENABLED();
        strncpy(g_states.debug_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
        strncpy(g_states.verbosity.cont, "DEBUG", POF_STRING_PAIR_MAX_LEN-1);
    }else if(strcmp(optarg, "error") == POF_OK){
		SET_DBG_DISABLED();
		SET_ERR_ENABLED();
		SET_CMD_ENABLED();
        strncpy(g_states.debug_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
        strncpy(g_states.verbosity.cont, "ERROR", POF_STRING_PAIR_MAX_LEN-1);
    }else if(strcmp(optarg, "mute") == POF_OK){
		SET_DBG_DISABLED();
		SET_ERR_DISABLED();
		SET_CMD_DISABLED();
        strncpy(g_states.debug_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
        strncpy(g_states.verbosity.cont, "MUTE", POF_STRING_PAIR_MAX_LEN-1);
    }else{
        return POF_ERROR;
    }
    return POF_OK;
}

static uint32_t
start_cmd_ip_addr(OPT_ARG)
{
    if(optarg == NULL){
        return POF_ERROR;
    }
    pofsc_set_controller_ip(optarg);
    return POF_OK;
}

static uint32_t
start_cmd_port(OPT_ARG)
{
    if(optarg == NULL){
        return POF_ERROR;
    }
    pofsc_set_controller_port(atoi(optarg));
    return POF_OK;
}

static uint32_t
start_cmd_file(OPT_ARG)
{
    FILE *fp = NULL;
    if(optarg == NULL){
        return POF_ERROR;
    }

    if((fp = fopen(optarg, "r")) == NULL){
        printf("Can not read the file %s\n", optarg);
        exit(0);
    }
    readConfigFile(fp, dp);
    strncpy(g_states.config_file.cont, optarg, POF_STRING_PAIR_MAX_LEN-1);
    return POF_OK;
}

static uint32_t
start_cmd_man_clear(OPT_ARG)
{
    pofc_cmd_auto_clear = FALSE;
    strncpy(g_states.autoclear_after_disconn.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
    return POF_OK;
}

static uint32_t
start_cmd_log_file(OPT_ARG)
{
    char *filename = optarg ? optarg : \
                     "/usr/local/var/log/pofswitch.log";
    if(POF_OK != pofsc_check_root()){
        exit(0);
    }
    pof_open_log_file(filename);
    strncpy(g_states.log_file.cont, filename, POF_STRING_PAIR_MAX_LEN-1);
    return POF_OK;
}

static uint32_t
start_cmd_state(OPT_ARG)
{
    poflp_states_print_(&g_states);
    exit(0);
}

static uint32_t
start_cmd_test(OPT_ARG)
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
start_cmd_help(OPT_ARG)
{
	printf("Usage: pofswitch [options] [target] ...\n");
	printf("Options:\n");

    while(1){
#define CONFIG_CMD(OPT,OPTSTR,LONG,FUNC,HELP)   \
            if(OPT == 't') {break;}             \
            printf("  -%c, --%-24s%s\n",OPT,LONG,HELP);
        CONFIG_CMDS
#undef CONFIG_CMD
        break;
    }

	printf("\nReport bugs to <yujingzhou@huawei.com>\n");
    exit(0);
}

static uint32_t
start_cmd_version(OPT_ARG)
{
    printf("Version: %s\n", POFSWITCH_VERSION);
    exit(0);
}


/***********************************************************************
 * Initialize the config by command.
 * Form:     static uint32_t pof_set_init_config_by_command(int argc, char *argv[])
 * Input:    command arg numbers, command args.
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function initialize the config of Soft Switch by user's 
 *           command arguments. 
 *           -i: set the IP address of the Controller.
 *           -p: set the port to connect to the Controller.
 ***********************************************************************/
static uint32_t pof_set_init_config_by_command(int argc, char *argv[], struct pof_datapath *dp){
	uint32_t ret = POF_OK;
#define OPTSTRING_LEN (100)
	char optstring[OPTSTRING_LEN] = {0};
#undef OPTSTRING_LEN
 	struct option long_options[] = {
#define CONFIG_CMD(OPT,OPTSTR,LONG,FUNC,HELP) {LONG,2,NULL,OPT},
        CONFIG_CMDS
#undef CONFIG_CMD
		{NULL,0,NULL,0}
	};
	int ch;

#define CONFIG_CMD(OPT,OPTSTR,LONG,FUNC,HELP) strncat(optstring,OPTSTR,COMMAND_STR_LEN);
    CONFIG_CMDS
#undef CONFIG_CMD

	while((ch=getopt_long(argc, argv, optstring, long_options, NULL)) != -1){
		switch(ch){
			case '?':
				exit(0);
				break;
#define CONFIG_CMD(OPT,OPTSTR,LONG,FUNC,HELP)               \
            case OPT:                                       \
                if(start_cmd_##FUNC(optarg, dp) != POF_OK){ \
                    printf("Wrong Option: %c\n", ch);       \
                    start_cmd_help(NULL, dp);               \
                    exit(0);                                \
                }                                           \
                break;

            CONFIG_CMDS
#undef CONFIG_CMD
			default:
				break;
		}
	}
    return POF_OK;
}

uint32_t pof_auto_clear(){
	return pofc_cmd_auto_clear;
}

enum pof_init_config_type{
	POFICT_CONTROLLER_IP	= 0,
	POFICT_CONNECTION_PORT  = 1,
	POFICT_MM_TABLE_NUMBER = 2,
	POFICT_LPM_TABLE_NUMBER = 3,
	POFICT_EM_TABLE_NUMBER  = 4,
	POFICT_DT_TABLE_NUMBER  = 5,
	POFICT_FLOW_TABLE_SIZE  = 6,
	POFICT_FLOW_TABLE_KEY_LENGTH = 7,
	POFICT_METER_NUMBER     = 8,
	POFICT_COUNTER_NUMBER   = 9,
	POFICT_GROUP_NUMBER     = 10,
	POFICT_DEVICE_PORT_NUMBER_MAX = 11,

	POFICT_CONFIG_TYPE_MAX,
};

char *config_type_str[POFICT_CONFIG_TYPE_MAX] = {
	"Controller_IP", "Connection_port", 
	"MM_table_number", "LPM_table_number", "EM_table_number", "DT_table_number",
	"Flow_table_size", "Flow_table_key_length", 
	"Meter_number", "Counter_number", "Group_number", 
	"Device_port_number_max"
};

static uint8_t pofsic_get_config_type(char *str){
	uint8_t i;

	for(i=0; i<POFICT_CONFIG_TYPE_MAX; i++){
		if(strcmp(str, config_type_str[i]) == 0){
			return i;
		}
	}

	return 0xff;
}

static uint32_t pofsic_get_config_data(FILE *fp, uint32_t *ret_p){
	char str[POF_STRING_MAX_LEN];

	if(fscanf(fp, "%s", str) != 1){
		*ret_p = POF_ERROR;
		return POF_ERROR;
	}else{
		return atoi(str);
	}
}

static uint32_t
readConfigFile(FILE *fp, struct pof_datapath *dp)
{
	uint32_t ret = POF_OK, data = 0;
    struct pof_param *param = &dp->param;
	char     str[POF_STRING_MAX_LEN] = "\0";
	char     ip_str[POF_STRING_MAX_LEN] = "\0";
	uint8_t  config_type = 0;
	while(fscanf(fp, "%s", str) == 1){
		config_type = pofsic_get_config_type(str);
		if(config_type == POFICT_CONTROLLER_IP){
			if(fscanf(fp, "%s", ip_str) != 1){
				ret = POF_ERROR;
			}else{
				pofsc_set_controller_ip(ip_str);
			}
		}else{
			data = pofsic_get_config_data(fp, &ret);
			switch(config_type){
				case POFICT_CONNECTION_PORT:
	                pofsc_set_controller_port(data);
					break;
				case POFICT_MM_TABLE_NUMBER:
                    param->tableNumMaxEachType[POF_MM_TABLE] = data;
					break;
				case POFICT_LPM_TABLE_NUMBER:
                    param->tableNumMaxEachType[POF_LPM_TABLE] = data;
					break;
				case POFICT_EM_TABLE_NUMBER:
                    param->tableNumMaxEachType[POF_EM_TABLE] = data;
					break;
				case POFICT_DT_TABLE_NUMBER:
                    param->tableNumMaxEachType[POF_LINEAR_TABLE] = data;
					break;
				case POFICT_FLOW_TABLE_SIZE:
                    param->tableSizeMax = data;
					break;
				case POFICT_FLOW_TABLE_KEY_LENGTH:
					poflr_set_key_len(data);
					break;
				case POFICT_METER_NUMBER:
                    param->meterNumMax = data;
					break;
				case POFICT_COUNTER_NUMBER:
                    param->counterNumMax = data;
					break;
				case POFICT_GROUP_NUMBER:
                    param->groupNumMax = data;
					break;
				case POFICT_DEVICE_PORT_NUMBER_MAX:
                    param->portNumMax = data;
					break;
				default:
					ret = POF_ERROR;
					break;
			}
		}

		if(ret != POF_OK){
			POF_ERROR_CPRINT_FL("Can't load config file: Wrong config format.");
			return ret;
		}
	}
    return POF_OK;
}

/***********************************************************************
 * Initialize the config by file.
 * Form:     static uint32_t pof_set_init_config_by_file()
 * Input:    NONE
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function initialize the config of Soft Switch by "config"
 *           file. The config's content:
 *			 "Controller_IP", "Connection_port", 
 *			 "MM_table_number", "LPM_table_number", "EM_table_number", "DT_table_number",
 *			 "Flow_table_size", "Flow_table_key_length", 
 *			 "Meter_number", "Counter_number", "Group_number", 
 *			 "Device_port_number_max"
 ***********************************************************************/
static uint32_t pof_set_init_config_by_file(struct pof_datapath *dp){
	char     filename_relative[] = "./pofswitch_config.conf";
	char     filename_absolute[] = "/etc/pofswitch/pofswitch_config.conf";
	char     filename_final[POF_STRING_MAX_LEN] = "\0";
	FILE     *fp = NULL;

    if((fp = fopen(filename_relative, "r")) == NULL){
        if((fp = fopen(filename_absolute, "r")) == NULL){
            return POF_ERROR;
        }else{
            strncpy(filename_final, filename_absolute, POF_STRING_MAX_LEN-1);
        }
    }else{
        strncpy(filename_final, filename_relative, POF_STRING_MAX_LEN-1);
    }

    strncpy(g_states.config_file.cont, filename_final, POF_STRING_PAIR_MAX_LEN-1);
    readConfigFile(fp, dp);

	fclose(fp);
    return POF_OK;
}

static void
versionInit(const char *version1, const char *version2)
{
    snprintf(g_states.version.cont, POF_STRING_PAIR_MAX_LEN-1, "%s.%s", version1, version2);
    snprintf(g_versionStr, POF_STRING_PAIR_MAX_LEN-1, "%s.%s", version1, version2);
    return;
}

/***********************************************************************
 * Initialize the config by file or command.
 * Form:     uint32_t pof_set_init_config(int argc, char *argv[])
 * Input:    number of arg, args
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function initialize the config of Soft Switch by "config"
 *           file or command arguments.
 ***********************************************************************/
uint32_t pof_set_init_config(int argc, char *argv[], struct pof_datapath *dp){
	uint32_t ret;

    versionInit(POFSwitch_VERSION_STR, POFSWITCH_VERSION_SUB);
	ret = pof_set_init_config_by_file(dp);
	ret = pof_set_init_config_by_command(argc, argv, dp);

	return POF_OK;
}

#undef POF_STRING_MAX_LEN
