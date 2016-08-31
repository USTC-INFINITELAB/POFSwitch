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

#ifndef _POF_LOG_PRINT_H_
#define _POF_LOG_PRINT_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "pof_common.h"
#include "pof_global.h"
#include "pof_local_resource.h"

#define BLACK   "30"
#define RED     "31"
#define GREEN   "32"
#define YELLOW  "33"
#define BLUE    "34"
#define PINK    "35"
#define CYAN    "36"
#define WHITE	""

#define POF_LOG_STRING_MAX_LEN (512)
#define LOGOPT (1)

struct log_util{
	FILE *log_fp;
	pthread_mutex_t mutex;
	uint32_t counter;

	uint8_t colorEnable;
	uint8_t dbgEnable;
	uint8_t cmdEnable;
	uint8_t errEnable;
};

extern struct log_util g_log;

#define IS_COLOR_ENABLED()	(g_log.colorEnable == POFE_ENABLE)
#define IS_DBG_ENABLED()	(g_log.dbgEnable == POFE_ENABLE)
#define IS_CMD_ENABLED()	(g_log.cmdEnable == POFE_ENABLE)
#define IS_ERR_ENABLED()	(g_log.errEnable == POFE_ENABLE)

#define SET_COLOR_ENABLED()		(g_log.colorEnable = POFE_ENABLE)
#define SET_COLOR_DISABLED()	(g_log.colorEnable = POFE_DISABLE)
#define SET_DBG_ENABLED()		(g_log.dbgEnable = POFE_ENABLE)
#define SET_DBG_DISABLED()		(g_log.dbgEnable = POFE_DISABLE)
#define SET_CMD_ENABLED()		(g_log.cmdEnable = POFE_ENABLE)
#define SET_CMD_DISABLED()		(g_log.cmdEnable = POFE_DISABLE)
#define SET_ERR_ENABLED()		(g_log.errEnable = POFE_ENABLE)
#define SET_ERR_DISABLED()		(g_log.errEnable = POFE_DISABLE)

/* flag = 0: receive, flag = 1: send */
extern void pof_debug_cprint_packet(const void *ph, uint32_t flag, int len);
extern void poflp_flow_entry(const void *ph);
extern void pof_open_log_file(char *filename);
extern void pof_close_log_file();

extern void poflp_states_print(const struct pof_state *);
extern void poflp_states_print_(const struct pof_state *);

extern void cmdPrintVersion(const char *version);
extern void cmdPrintTableResource(const struct pof_flow_table_resource *p);
extern void cmdPrintSwitchConfig(const struct pof_switch_config *pofsc_ptr);
extern void cmdPrintPort(const struct portInfo *p);
extern void cmdPrintMeter(const struct meterInfo *meter);
extern void cmdPrintGroup(const struct groupInfo *group);
extern void cmdPrintFlowTableBaseinfo(const struct tableInfo *table);
extern void cmdPrintFlowEntryBaseinfo(const struct entryInfo *entry);
extern void cmdPrintFeature(const struct pof_switch_features *p);
extern void cmdPrintCounter(const struct counterInfo *counter);
#ifdef POF_SHT_VXLAN
extern void cmdPrintInsBlock(const struct insBlockInfo *p);
#endif // POF_SHT_VXLAN

#if (POF_OS_BIT == POF_OS_32)
#define POF_PRINT_FORMAT_U64    "llu"
#elif (POF_OS_BIT == POF_OS_64)
#define POF_PRINT_FORMAT_U64    "lu"
#endif // POF_OS_BIT

#define POF_LOG_LOCK_ON			pthread_mutex_lock(&g_log.mutex)
#define POF_LOG_LOCK_OFF		pthread_mutex_unlock(&g_log.mutex)

#define POF_TEXTCOLOR(i,col)					\
			if(IS_COLOR_ENABLED()){				\
				printf("\033[%d;%sm", i, col);	\
			}
#define POF_OVERCOLOR()				\
			if(IS_COLOR_ENABLED()){	\
				printf("\033[0m");	\
			}

#define POF_CPRINT(i,col,enable,cont,...)					\
			if(g_log.log_fp != NULL){						\
				fprintf(g_log.log_fp,cont,##__VA_ARGS__);	\
			}												\
			if(enable == POFE_ENABLE){						\
				POF_TEXTCOLOR(i,col);						\
				printf(cont,##__VA_ARGS__);					\
				POF_OVERCOLOR();							\
			}
#define POF_CPRINT_NO_LOGFILE(i,col,enable,cont,...)	\
			if(enable == POFE_ENABLE){					\
				POF_TEXTCOLOR(i,col);					\
				printf(cont,##__VA_ARGS__);				\
				POF_OVERCOLOR();						\
			}

#define POF_DEBUG_CPRINT(i,col,cont,...)	POF_CPRINT(i,col,g_log.dbgEnable,cont,##__VA_ARGS__)

#define POF_DEBUG_CPRINT_HEAD(i,col)            POF_DEBUG_CPRINT(i,col,"%05u|INFO|%s|%d: ", g_log.counter++, __FILE__, __LINE__)
#define POF_DEBUG_CPRINT_FL(i,col,cont,...) \
			POF_LOG_LOCK_ON;								\
            POF_DEBUG_CPRINT_HEAD(i,col);					\
            POF_DEBUG_CPRINT(i,col,cont"\n",##__VA_ARGS__); \
			POF_LOG_LOCK_OFF
#define POF_DEBUG_CPRINT_FL_NO_LOCK(i,col,cont,...) \
            POF_DEBUG_CPRINT_HEAD(i,col);					\
            POF_DEBUG_CPRINT(i,col,cont"\n",##__VA_ARGS__); \

#define POF_DEBUG_CPRINT_FL_NO_ENTER(i,col,cont,...) \
            POF_DEBUG_CPRINT_HEAD(i,col); \
            POF_DEBUG_CPRINT(i,col,cont,##__VA_ARGS__); \

/* flag = 0: receive, flag = 1: send */
#define POF_DEBUG_CPRINT_PACKET(pheader,flag,len) \
			POF_LOG_LOCK_ON;							\
            POF_DEBUG_CPRINT_HEAD(1,GREEN);				\
            pof_debug_cprint_packet(pheader,flag,len);	\
			POF_LOG_LOCK_OFF

#define POF_DEBUG_CPRINT_0X_NO_ENTER(x, len) \
            { \
                POF_DEBUG_CPRINT(1,WHITE,"0x"); \
                uint32_t i; \
                for(i=0; i<len; i++){   \
                    POF_DEBUG_CPRINT(1,WHITE,"%.2x ", *((uint8_t *)x + i)); \
                } \
            }

#define POF_DEBUG_CPRINT_0X(x, len) \
            POF_DEBUG_CPRINT_0X_NO_ENTER(x, len); \
            POF_DEBUG_CPRINT(1,WHITE,"\n")

#define POF_DEBUG_CPRINT_FL_0X(i,col,x,len,cont,...) \
			POF_LOG_LOCK_ON;										\
            POF_DEBUG_CPRINT_FL_NO_ENTER(i,col,cont,##__VA_ARGS__); \
            POF_DEBUG_CPRINT_0X(x,len);								\
			POF_LOG_LOCK_OFF

#define POF_DEBUG_SLEEP(i) \
            POF_DEBUG_CPRINT_FL(1,WHITE,"sleep start! [%ds]\n", i); \
            sleep(i); \
            POF_DEBUG_CPRINT_FL(1,WHITE,"sleep over! [%ds]\n", i)

#define POF_DEBUG(cont,...)         POF_DEBUG_CPRINT_FL(1,YELLOW,"[DEBUG] "cont,##__VA_ARGS__)
#define POF_DEBUG_VAR(var)          POF_DEBUG(#var" = 0x%.8x", (uint32_t)(var));

#define POF_COMMAND_PRINT_NO_LOGFILE(i,col,cont,...)	POF_CPRINT_NO_LOGFILE(i,col,g_log.cmdEnable,cont,##__VA_ARGS__)
#define POF_COMMAND_PRINT(i,col,cont,...)   POF_CPRINT(i,col,g_log.cmdEnable,cont,##__VA_ARGS__)
#define POF_COMMAND_PRINT_HEAD(cont,...)	POF_COMMAND_PRINT(1,PINK,"POF_COMMAND|%s|%d: "cont"\n",__FILE__,__LINE__,##__VA_ARGS__)

/* Print data x in byte unit with hexadeimal format in user command mode with no enter at the end. */
#define POF_COMMAND_PRINT_0X_NO_ENTER(x, len) \
    {int i;																\
        POF_COMMAND_PRINT(1,WHITE,"0x");                                \
        for(i=0; i<len; i++){											\
            POF_COMMAND_PRINT(1,WHITE,"%.2x ", *((uint8_t *)x + i));	\
        }                                                               \
    }

#define COMMAND_PRINT_U64(var) \
            POF_COMMAND_PRINT(1,WHITE,"%"POF_PRINT_FORMAT_U64" ",var);

#ifdef POF_ERROR_PRINT_ON
#define POF_ERROR_CPRINT(cont,...)		POF_CPRINT(1,RED,g_log.errEnable,cont,##__VA_ARGS__);
#else // POF_ERROR_PRINT_ON
#define POF_ERROR_CPRINT(cont,...)
#endif // POF_ERROR_PRINT_ON

#define POF_ERROR_CPRINT_HEAD() POF_ERROR_CPRINT("ERROR|%s|%d: ", __FILE__, __LINE__)
#define POF_ERROR_CPRINT_FL(cont,...)                                           \
			POF_LOG_LOCK_ON;							                        \
            POF_ERROR_CPRINT_HEAD();                                            \
            POF_ERROR_CPRINT(cont"\n", ##__VA_ARGS__);                          \
            POF_ERROR_CPRINT("System ERROR information: %s\n",strerror(errno)); \
			POF_LOG_LOCK_OFF;
#define POF_ERROR_CPRINT_FL_NO_LOCK(cont,...)           \
            POF_ERROR_CPRINT_HEAD();                    \
            POF_ERROR_CPRINT(cont"\n", ##__VA_ARGS__);

#define POF_PRINT(i,col,cont,...)	POF_CPRINT(i,col,POFE_ENABLE,cont,##__VA_ARGS__)
#define POF_PRINT_HEAD(i,col)       POF_PRINT(i,col,"%05u|INFO|%s|%d: ", g_log.counter++, __FILE__, __LINE__)
#define POF_PRINT_FL(i,col,cont,...) \
            POF_PRINT_HEAD(i,col);					\
            POF_PRINT(i,col,cont"\n",##__VA_ARGS__); \


#endif // _POF_LOG_PRINT_H_
