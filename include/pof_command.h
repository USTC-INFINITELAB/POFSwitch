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

#ifndef _POF_COMMAND_H_
#define _POF_COMMAND_H_

#include "pof_global.h"
#include "pof_log_print.h"
#include "pof_local_resource.h"
#include "pof_datapath.h"
#include "pof_common.h"

/* The user commands. */
#ifdef POF_SHT_VXLAN
#define COMMANDS \
	COMMAND(help)				\
	COMMAND(quit)				\
	COMMAND(exit)				\
	COMMAND(switch_config)		\
	COMMAND(switch_feature)		\
	COMMAND(table_resource)		\
	COMMAND(ports)				\
	COMMAND(tables)				\
	COMMAND(groups)				\
	COMMAND(meters)				\
	COMMAND(counters)			\
	COMMAND(version)			\
	COMMAND(state)			    \
	COMMAND(enable_promisc)		\
	COMMAND(disable_promisc)	\
	COMMAND(addport)	        \
	COMMAND(delport)	        \
	COMMAND(test)               \
	COMMAND(clear_resource)		\
	COMMAND(enable_debug)		\
	COMMAND(disable_debug)		\
	COMMAND(enable_color)		\
	COMMAND(disable_color)		\
	COMMAND(blocks)		        \
	COMMAND(memory)

#else // POF_SHT_VXLAN
#define COMMANDS \
	COMMAND(help)				\
	COMMAND(quit)				\
	COMMAND(exit)				\
	COMMAND(switch_config)		\
	COMMAND(switch_feature)		\
	COMMAND(table_resource)		\
	COMMAND(ports)				\
	COMMAND(tables)				\
	COMMAND(groups)				\
	COMMAND(meters)				\
	COMMAND(counters)			\
	COMMAND(version)			\
	COMMAND(state)			    \
	COMMAND(enable_promisc)		\
	COMMAND(disable_promisc)	\
	COMMAND(addport)	        \
	COMMAND(delport)	        \
	COMMAND(test)               \
	COMMAND(clear_resource)		\
	COMMAND(enable_debug)		\
	COMMAND(disable_debug)		\
	COMMAND(enable_color)		\
	COMMAND(disable_color)		\
	COMMAND(memory)
#endif // POF_SHT_VXLAN

/* The user commands. */
enum pof_usr_cmd{
#define COMMAND(STRING) POFUC_##STRING,
	COMMANDS
#undef COMMAND

	POF_COMMAND_NUM
};

#define POF_CMD_WRONG   0xFF
#define POF_CMD_EMPTY   0xFE
#define POF_CMD_STR_LEN (32)

/* Go into user command mode. */
void pof_runing_command(struct pof_datapath *dp);
/* Print all tables, entries information. */
void usr_cmd_tables();

#endif // _POF_COMMAND_H_
