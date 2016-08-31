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
#include "../include/pof_log_print.h"
#include "../include/pof_byte_transfer.h"
#include "../include/pof_local_resource.h"

static void action(const void *ph);
static void poflp_flow_entry_simple(const void *ph);
static void instruction(const void *ph);

#define LOG_PRINT_U64(var) POF_DEBUG_CPRINT(1,WHITE,"%"POF_PRINT_FORMAT_U64" ",var)
#define DEBUG_VAR(STRUCTURE,MEMBER,STR)                         \
            POF_DEBUG_CPRINT(1,CYAN,#MEMBER"=");                \
            POF_DEBUG_CPRINT(1,WHITE,STR" ",(STRUCTURE).MEMBER);

void poflp_states_print_(const struct pof_state *state){
    struct pof_str_pair *str_pair = NULL;
    int num = 0, i;
    
    num = sizeof(struct pof_state) / sizeof(struct pof_str_pair);
    for(i=0;i<num;i++){
        str_pair = (struct pof_str_pair *)state + i;
        printf("  %-30s| %s\n",str_pair->item,str_pair->cont);
    }

    return;
}

void poflp_states_print(const struct pof_state *state){
    struct pof_str_pair *str_pair = NULL;
    int num = 0, i;
    
    num = sizeof(struct pof_state) / sizeof(struct pof_str_pair);
    for(i=0;i<num;i++){
        str_pair = (struct pof_str_pair *)state + i;
        POF_DEBUG_CPRINT(1,CYAN,"  %-30s| %s\n",str_pair->item,str_pair->cont);
    }

    return;
}

void 
cmdPrintVersion(const char *version)
{
	POF_COMMAND_PRINT_HEAD("version");
	POF_COMMAND_PRINT(1,PINK,"%s\n", version);
}

static void 
cmdPrintTableResourceDesc(const pof_table_resource_desc *p)
{
    POF_COMMAND_PRINT(1,CYAN,"device_id=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.4x ",p->device_id);
    POF_COMMAND_PRINT(1,CYAN,"type=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",p->type);
    POF_COMMAND_PRINT(1,CYAN,"tbl_num=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",p->tbl_num);
    POF_COMMAND_PRINT(1,CYAN,"key_len=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->key_len);
    POF_COMMAND_PRINT(1,CYAN,"total_size=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->total_size);
	POF_COMMAND_PRINT(1,WHITE,"\n");
    return;
}

void
cmdPrintTableResource(const struct pof_flow_table_resource *p)
{
    int i;
    POF_COMMAND_PRINT_HEAD("table_resource");

    POF_COMMAND_PRINT(1,CYAN,"resourceType=");
    POF_COMMAND_PRINT(1,WHITE,"%.2x ",p->resourceType);
#ifdef POF_MULTIPLE_SLOTS
    POF_COMMAND_PRINT(1,CYAN,"slotID=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.4x ",p->slotID);
#endif // POF_MULTIPLE_SLOTS
    POF_COMMAND_PRINT(1,CYAN,"counter_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->counter_num);
    POF_COMMAND_PRINT(1,CYAN,"meter_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->meter_num);
    POF_COMMAND_PRINT(1,CYAN,"group_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->group_num);
	POF_COMMAND_PRINT(1,WHITE,"\n");

    for(i=0;i<POF_MAX_TABLE_TYPE;i++){
        POF_COMMAND_PRINT(1,PINK,"[pof_table_resource_desc %d] ",i);
        cmdPrintTableResourceDesc(&p->tbl_rsc_desc[i]);
		POF_COMMAND_PRINT(1,WHITE,"\n");
    }
}

void
cmdPrintSwitchConfig(const struct pof_switch_config *pofsc_ptr)
{
    POF_COMMAND_PRINT_HEAD("switch_config");

#ifdef POF_MULTIPLE_SLOTS
    POF_COMMAND_PRINT(1,CYAN,"dev_id=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.8x ",pofsc_ptr->dev_id);
#endif // POF_MULTIPLE_SLOTS
    POF_COMMAND_PRINT(1,CYAN,"flags=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",pofsc_ptr->flags);
    POF_COMMAND_PRINT(1,CYAN,"miss_send_len=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",pofsc_ptr->miss_send_len);
	POF_COMMAND_PRINT(1,WHITE,"\n");
}

void
cmdPrintPort(const struct portInfo *p)
{
    POF_COMMAND_PRINT(1,PINK,"[Port %d] ", p->pofIndex);

    POF_COMMAND_PRINT(1,CYAN,"name=");
    POF_COMMAND_PRINT(1,WHITE,"%s ",p->name);
    POF_COMMAND_PRINT(1,CYAN,"port_id=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->pofIndex);
    POF_COMMAND_PRINT(1,CYAN,"slotID=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->slotID);
    POF_COMMAND_PRINT(1,CYAN,"hw_addr=");
    POF_COMMAND_PRINT_0X_NO_ENTER(p->hwaddr, POF_ETH_ALEN);
    POF_COMMAND_PRINT(1,CYAN,"IP=");
    POF_COMMAND_PRINT(1,WHITE,"%-15s ",p->ip);
    POF_COMMAND_PRINT(1,CYAN,"state=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",p->pofState);
    POF_COMMAND_PRINT(1,CYAN,"of_enable=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",p->of_enable);
    POF_COMMAND_PRINT(1,WHITE,"\n");
}

void
cmdPrintMeter(const struct meterInfo *meter)
{
    POF_COMMAND_PRINT(1,PINK,"[meter %d] ", meter->id);
    POF_COMMAND_PRINT(1,CYAN,"rate=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", meter->rate);
    POF_COMMAND_PRINT(1,CYAN,"meter_id=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", meter->id);
    POF_COMMAND_PRINT(1,CYAN,"\n");
}

static void 
cmdPrintMatch(const pof_match *p)
{
    POF_COMMAND_PRINT(1,CYAN,"field_id=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", p->field_id);
    POF_COMMAND_PRINT(1,CYAN,"offset=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", p->offset);
    POF_COMMAND_PRINT(1,CYAN,"len=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", p->len);
}

void
cmdPrintGroup(const struct groupInfo *group)
{
    uint32_t j;
    POF_COMMAND_PRINT(1,PINK,"[group %d] ", group->id);

    POF_COMMAND_PRINT(1,CYAN,"type=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", group->type);
    POF_COMMAND_PRINT(1,CYAN,"action_number=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", group->action_number);
    POF_COMMAND_PRINT(1,CYAN,"group_id=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", group->id);
    POF_COMMAND_PRINT(1,CYAN,"counter_id=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", group->counter_id);

	uint8_t enableTmp = g_log.dbgEnable;
	g_log.dbgEnable = g_log.cmdEnable;

    for(j=0; j<group->action_number; j++){
        action(&group->action[j]);
    }
    g_log.dbgEnable = enableTmp;

    POF_COMMAND_PRINT(1,CYAN,"\n");
}

void 
cmdPrintFlowTableBaseinfo(const struct tableInfo *table)
{
    char ctype[POF_MAX_TABLE_TYPE][5] = {
                            "MM","LPM","EM","DT",};
    uint32_t type = table->type;
    uint32_t table_id = table->id;
    uint32_t i;

    POF_COMMAND_PRINT(1,PINK,"\n[Flow table] [%s %d] ", ctype[type], table_id);

    POF_COMMAND_PRINT(1,CYAN,"entry_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", table->entryNum);
    POF_COMMAND_PRINT(1,CYAN,"table_id=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", table->id);
    POF_COMMAND_PRINT(1,CYAN,"type=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", table->type);
    POF_COMMAND_PRINT(1,CYAN,"match_field_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",table->match_field_num);
    POF_COMMAND_PRINT(1,CYAN,"size=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", table->size);
//    POF_COMMAND_PRINT(1,CYAN,"key_len=");
//    POF_COMMAND_PRINT(1,WHITE,"%u ", table->key_len);
    POF_COMMAND_PRINT(1,CYAN,"table_name=");
    POF_COMMAND_PRINT(1,WHITE,"%s ", table->name);
    for(i=0;i<table->match_field_num;i++){
        POF_COMMAND_PRINT(1,PINK,"<match %d> ", i);
        cmdPrintMatch(&table->match[i]);
    }
	POF_COMMAND_PRINT(1,WHITE,"\n");
    return;
}

void 
cmdPrintFlowEntryBaseinfo(const struct entryInfo *entry)
{
	uint8_t enableTmp = g_log.dbgEnable;
	g_log.dbgEnable = g_log.cmdEnable;

	poflp_flow_entry_simple(entry);

    g_log.dbgEnable = enableTmp;

    POF_COMMAND_PRINT(1,WHITE,"\n");
    return;
}

#ifdef POF_SHT_VXLAN
void
cmdPrintInsBlock(const struct insBlockInfo *p)
{
	void (*tmp)(char *,char *,char *) = g_log._pDbg;
    struct pof_instruction *ins = (struct pof_instruction *)p->insData;
    uint8_t i, insNum = p->insNum;

    g_log._pDbg = g_log._pCmd;

    POF_COMMAND_PRINT(1,PINK,"<block[%.2u]> ", p->blockID);
    DEBUG_VAR(*p, insNum,    "%u");
    DEBUG_VAR(*p, tableID,   "%u");
    DEBUG_VAR(*p, blockID,   "%u");

    for(i=0; i<insNum; i++){
        instruction(ins);
        ins = (struct pof_instruction *)( \
                (uint8_t *)ins + ins->len );
    }

	g_log._pDbg = tmp;

    POF_COMMAND_PRINT(1,WHITE,"\n");
    return;
}
#endif // POF_SHT_VXLAN

void
cmdPrintFeature(const struct pof_switch_features *p)
{
    POF_COMMAND_PRINT_HEAD("switch_feature");

    POF_COMMAND_PRINT(1,CYAN,"dev_id=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.8x ",p->dev_id);
#ifdef POF_MULTIPLE_SLOTS
    POF_COMMAND_PRINT(1,CYAN,"slotID=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.4x ",p->slotID);
#endif // POF_MULTIPLE_SLOTS
    POF_COMMAND_PRINT(1,CYAN,"port_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->port_num);
    POF_COMMAND_PRINT(1,CYAN,"table_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->table_num);
    POF_COMMAND_PRINT(1,CYAN,"capabilities=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->capabilities);
    POF_COMMAND_PRINT(1,CYAN,"vendor_id=");
    POF_COMMAND_PRINT(1,WHITE,"%s ",p->vendor_id);
    POF_COMMAND_PRINT(1,CYAN,"dev_fw_id=");
    POF_COMMAND_PRINT(1,WHITE,"%s ",p->dev_fw_id);
    POF_COMMAND_PRINT(1,CYAN,"dev_lkup_id=");
    POF_COMMAND_PRINT(1,WHITE,"%s ",p->dev_lkup_id);
	POF_COMMAND_PRINT(1,WHITE,"\n");
}

void
cmdPrintCounter(const struct counterInfo *counter)
{
    POF_COMMAND_PRINT(1,PINK,"[counter %d] ", counter->id);
    POF_COMMAND_PRINT(1,CYAN,"counter_id=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", counter->id);
    POF_COMMAND_PRINT(1,CYAN,"value=");
    COMMAND_PRINT_U64(counter->value);
#ifdef POF_SD2N
    POF_COMMAND_PRINT(1,CYAN,"byte_value=");
    COMMAND_PRINT_U64(counter->byte_value);
#endif // POF_SD2N
    POF_COMMAND_PRINT(1,CYAN,"\n");
}

void pof_open_log_file(char *filename){
	g_log.log_fp = fopen(filename, "w");
	if(!g_log.log_fp){
		POF_ERROR_CPRINT_FL("Open %s failed!", filename);
		exit(0);
	}
	return;
}

void pof_close_log_file(){
	if(!g_log.log_fp){
		return;
	}
	fclose(g_log.log_fp);
	g_log.log_fp = NULL;
	return;
}

static void port(const unsigned char * ph){
    uint32_t i;
    pof_port p = *((pof_port *)ph);

    pof_NtoH_transfer_port(&p);

#ifdef POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"slotID=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.4x ",p.slotID);
    POF_DEBUG_CPRINT(1,CYAN,"port_id=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.4x ",p.port_id);
#else // POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"port_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.port_id);
#endif // POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"device_id=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.8x ",p.device_id);
    POF_DEBUG_CPRINT(1,CYAN,"hw_addr=0x");
    for(i=0; i<POF_ETH_ALEN; i++){
        POF_DEBUG_CPRINT(1,WHITE,"%.2x", *(p.hw_addr+i));
    }
    POF_DEBUG_CPRINT(1,CYAN," name=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p.name);
    POF_DEBUG_CPRINT(1,CYAN,"config=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p.config);
    POF_DEBUG_CPRINT(1,CYAN,"state=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p.state);
    POF_DEBUG_CPRINT(1,CYAN,"curr=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p.curr);
    POF_DEBUG_CPRINT(1,CYAN,"advertised=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p.advertised);
    POF_DEBUG_CPRINT(1,CYAN,"supported=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p.supported);
    POF_DEBUG_CPRINT(1,CYAN,"peer=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p.peer);
    POF_DEBUG_CPRINT(1,CYAN,"curr_speed=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.curr_speed);
    POF_DEBUG_CPRINT(1,CYAN,"max_speed=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.max_speed);
    POF_DEBUG_CPRINT(1,CYAN,"of_enable=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p.of_enable);
}

static void port_status(const unsigned char *ph){
    pof_port_status p = *((pof_port_status *)ph);

    pof_HtoN_transfer_port_status(&p);

    POF_DEBUG_CPRINT(1,CYAN,"reason=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p.reason);

    POF_DEBUG_CPRINT(1,PINK,"[pof_port] ");

    port((unsigned char *)ph+sizeof(pof_port_status)- \
                sizeof(pof_port));
}

static void table_resource_desc(const unsigned char *ph){
    pof_table_resource_desc *p = \
			(pof_table_resource_desc *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"device_id=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.8x ",p->device_id);
    POF_DEBUG_CPRINT(1,CYAN,"type=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->type);
    POF_DEBUG_CPRINT(1,CYAN,"tbl_num=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->tbl_num);
    POF_DEBUG_CPRINT(1,CYAN,"key_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->key_len);
    POF_DEBUG_CPRINT(1,CYAN,"total_size=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->total_size);
}

static void flow_table_resource(const unsigned char *ph){
    pof_flow_table_resource p = *((pof_flow_table_resource *)ph);
    int i;

    pof_HtoN_transfer_flow_table_resource(&p);

    POF_DEBUG_CPRINT(1,CYAN,"resourceType=");
    POF_DEBUG_CPRINT(1,WHITE,"%.2x ",p.resourceType);
#ifdef POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"slotID=");
    POF_DEBUG_CPRINT(1,WHITE,"%.4x ",p.slotID);
#endif // POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"counter_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.counter_num);
    POF_DEBUG_CPRINT(1,CYAN,"meter_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.meter_num);
    POF_DEBUG_CPRINT(1,CYAN,"group_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.group_num);

    for(i=0;i<POF_MAX_TABLE_TYPE;i++){
        POF_DEBUG_CPRINT(1,PINK,"[pof_table_resource_desc%d] ",i);
        table_resource_desc((uint8_t *)&p.tbl_rsc_desc[i]);
    }
}

static void switch_config(const unsigned char *ph){
    pof_switch_config p = *((pof_switch_config *)ph);

    pof_HtoN_transfer_switch_config(&p);

#ifdef POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"dev_id=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.8x ",p.dev_id);
#endif // POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"flags=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.4x ",p.flags);
    POF_DEBUG_CPRINT(1,CYAN,"miss_send_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%.4x ",p.miss_send_len);
}

static void switch_features(const unsigned char *ph){
    pof_switch_features p = *((pof_switch_features *)ph);

    pof_HtoN_transfer_switch_features(&p);

    POF_DEBUG_CPRINT(1,CYAN,"dev_id=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.8x ",p.dev_id);
#ifdef POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"slotID=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.4x ",p.slotID);
#endif // POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"port_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.port_num);
    POF_DEBUG_CPRINT(1,CYAN,"table_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.table_num);
    POF_DEBUG_CPRINT(1,CYAN,"capabilities=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.capabilities);
    POF_DEBUG_CPRINT(1,CYAN,"vendor_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p.vendor_id);
    POF_DEBUG_CPRINT(1,CYAN,"dev_fw_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p.dev_fw_id);
    POF_DEBUG_CPRINT(1,CYAN,"dev_lkup_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p.dev_lkup_id);

}

static void set_config(const unsigned char *ph){
    switch_config(ph);
}

static void match(const void *ph){
    pof_match *p = (pof_match *)ph;

    POF_DEBUG_CPRINT(1,PINK,"[match] ");
    POF_DEBUG_CPRINT(1,CYAN,"field_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->field_id);
    POF_DEBUG_CPRINT(1,CYAN,"offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->offset);
    POF_DEBUG_CPRINT(1,CYAN,"len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->len);
}

static void match_x(const void *ph){
    pof_match_x *p = (pof_match_x *)ph;

    POF_DEBUG_CPRINT(1,PINK,"[match_x] ");
    POF_DEBUG_CPRINT(1,CYAN,"field_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->field_id);
    POF_DEBUG_CPRINT(1,CYAN,"offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->offset);
    POF_DEBUG_CPRINT(1,CYAN,"len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->len);

    POF_DEBUG_CPRINT(1,CYAN,"value=");
    POF_DEBUG_CPRINT_0X_NO_ENTER(p->value, POF_MAX_FIELD_LENGTH_IN_BYTE);
    POF_DEBUG_CPRINT(1,CYAN,"mask=");
    POF_DEBUG_CPRINT_0X_NO_ENTER(p->mask, POF_MAX_FIELD_LENGTH_IN_BYTE);
}

static void
value_byte(void *ph, uint8_t type, const char *union_name, const char *type_name, uint8_t byteLen)
{
	union {
		uint8_t value;
		struct pof_match field;
	} *p = ph;

	POF_DEBUG_CPRINT(1,CYAN,"%s=",type_name);
	if(type == POFVT_IMMEDIATE_NUM){
		POF_DEBUG_CPRINT(1,WHITE,"POFVT_IMMEDIATE_NUM ");
		POF_DEBUG_CPRINT(1,CYAN,"%s=",union_name);
		POF_DEBUG_CPRINT(1,CYAN,"[value]");
		POF_DEBUG_CPRINT_0X_NO_ENTER(p, byteLen);
	}else{
		POF_DEBUG_CPRINT(1,WHITE,"POFVT_FIELD ");
		POF_DEBUG_CPRINT(1,CYAN,"%s=",union_name);
		match(&p->field);
	}
}

static void
value_16(void *ph, uint8_t type, const char *union_name, const char *type_name)
{
	union {
		uint16_t value;
		struct pof_match field;
	} *p = ph;

	POF_DEBUG_CPRINT(1,CYAN,"%s=",type_name);
	if(type == POFVT_IMMEDIATE_NUM){
		POF_DEBUG_CPRINT(1,WHITE,"POFVT_IMMEDIATE_NUM ");
		POF_DEBUG_CPRINT(1,CYAN,"%s=",union_name);
		POF_DEBUG_CPRINT(1,CYAN,"[value]");
		POF_DEBUG_CPRINT(1,WHITE,"%u ",p->value);
	}else{
		POF_DEBUG_CPRINT(1,WHITE,"POFVT_FIELD ");
		POF_DEBUG_CPRINT(1,CYAN,"%s=",union_name);
		match(&p->field);
	}
}

static void
value(void *ph, uint8_t type, const char *union_name, const char *type_name)
{
	union {
		uint32_t value;
		struct pof_match field;
	} *p = ph;

	POF_DEBUG_CPRINT(1,CYAN,"%s=",type_name);
	if(type == POFVT_IMMEDIATE_NUM){
		POF_DEBUG_CPRINT(1,WHITE,"POFVT_IMMEDIATE_NUM ");
		POF_DEBUG_CPRINT(1,CYAN,"%s=",union_name);
		POF_DEBUG_CPRINT(1,CYAN,"[value]");
		POF_DEBUG_CPRINT(1,WHITE,"%u ",p->value);
	}else{
		POF_DEBUG_CPRINT(1,WHITE,"POFVT_FIELD ");
		POF_DEBUG_CPRINT(1,CYAN,"%s=",union_name);
		match(&p->field);
	}
}

static void flow_table(const unsigned char *ph){
    pof_flow_table p = *((pof_flow_table *)ph);
    uint32_t i;

    pof_NtoH_transfer_flow_table(&p);

    POF_DEBUG_CPRINT(1,CYAN,"command=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.command);
    POF_DEBUG_CPRINT(1,CYAN,"slotID=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.slotID);
    POF_DEBUG_CPRINT(1,CYAN,"tid=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.tid);
    POF_DEBUG_CPRINT(1,CYAN,"type=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.type);
    POF_DEBUG_CPRINT(1,CYAN,"match_field_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.match_field_num);
    POF_DEBUG_CPRINT(1,CYAN,"size=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.size);
    POF_DEBUG_CPRINT(1,CYAN,"key_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.key_len);
    POF_DEBUG_CPRINT(1,CYAN,"table_name=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p.table_name);

    for(i=0; i<p.match_field_num; i++){
        match(&p.match[i]);
    }
}

static void action_DROP(const void *ph){
    pof_action_drop *p = (pof_action_drop *)ph;

#ifdef POF_SHT_VXLAN
    value(&p->reason_code, p->code_type, "reason_code", "code_type");
#else // POF_SHT_VXLAN
    DEBUG_VAR(*p, reason_code, "0x%.8x");
#endif // POF_SHT_VXLAN
}

static void action_PACKET_IN(const void *ph){
    pof_action_packet_in *p = (pof_action_packet_in *)ph;

#ifdef POF_SHT_VXLAN
    value(&p->reason_code, p->code_type, "reason_code", "code_type");
#else // POF_SHT_VXLAN
    DEBUG_VAR(*p, reason_code, "0x%.8x");
#endif // POF_SHT_VXLAN
}

static void action_ADD_FIELD(const void *ph){
    pof_action_add_field *p = (pof_action_add_field *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"tag_pos=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->tag_pos);
    POF_DEBUG_CPRINT(1,CYAN,"tag_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->tag_len);
#ifdef POF_SHT_VXLAN
    value_byte(&p->tag, p->tag_type, "tag", "tag_type", POF_BITNUM_TO_BYTENUM_CEIL(p->tag_len));
#else // POF_SHT_VXLAN
    POF_DEBUG_CPRINT(1,CYAN,"tag_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->tag_id);
#ifdef POF_SD2N_AFTER1015
    POF_DEBUG_CPRINT(1,CYAN,"tag_value=");
    POF_DEBUG_CPRINT_0X_NO_ENTER(p->tag_value, POF_MAX_FIELD_LENGTH_IN_BYTE);
#else // POF_SD2N_AFTER1015
    POF_DEBUG_CPRINT(1,CYAN,"tag_value=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.16llx ",p->tag_value);
#endif // POF_SD2N_AFTER1015
#endif // POF_SHT_VXLAN
}

static void action_DELETE_FIELD(const void *ph){
    pof_action_delete_field *p = (pof_action_delete_field *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"tag_pos=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->tag_pos);
#ifdef POF_SD2N
    value(&p->tag_len, p->len_type, "tag_len", "len_type");
#else // POF_SD2N
    POF_DEBUG_CPRINT(1,CYAN,"tag_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->tag_len);
#endif // POF_SD2N
}

static void action_OUTPUT(const void *ph){
    pof_action_output *p = (pof_action_output *)ph;

#ifdef POF_SD2N
	value(&p->outputPortId, p->portId_type, "outputPortId", "portId_type");
#else // POF_SD2N
    POF_DEBUG_CPRINT(1,CYAN,"outputPortId=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->outputPortId);
#endif // POF_SD2N
    POF_DEBUG_CPRINT(1,CYAN,"metadata_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->metadata_offset);
    POF_DEBUG_CPRINT(1,CYAN,"metadata_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->metadata_len);
    POF_DEBUG_CPRINT(1,CYAN,"packet_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->packet_offset);
}

static void action_CALCULATE_CHECKSUM(const void *ph){
    pof_action_calculate_checksum *p = (pof_action_calculate_checksum *)ph;

#ifdef POF_SD2N_AFTER1015
    POF_DEBUG_CPRINT(1,CYAN,"checksum_pos_type=");
    if(p->checksum_pos_type == 0){
        POF_DEBUG_CPRINT(1,WHITE,"packet");
    }else{
        POF_DEBUG_CPRINT(1,WHITE,"metadata");
    }
#else // POF_SD2N_AFTER1015
#endif // POF_SD2N_AFTER1015
    POF_DEBUG_CPRINT(1,CYAN,"checksum_pos=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->checksum_pos);
    POF_DEBUG_CPRINT(1,CYAN,"checksum_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->checksum_len);
#ifdef POF_SD2N_AFTER1015
    POF_DEBUG_CPRINT(1,CYAN,"cal_startpos_type=");
    if(p->cal_startpos_type == 0){
        POF_DEBUG_CPRINT(1,WHITE,"packet");
    }else{
        POF_DEBUG_CPRINT(1,WHITE,"metadata");
    }
#else // POF_SD2N_AFTER1015
#endif // POF_SD2N_AFTER1015
    POF_DEBUG_CPRINT(1,CYAN,"cal_startpos=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->cal_startpos);
    POF_DEBUG_CPRINT(1,CYAN,"cal_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->cal_len);
}

static void action_COUNTER(const void *ph){
    pof_action_counter *p = (pof_action_counter *)ph;

#ifdef POF_SHT_VXLAN
    value(&p->counter_id, p->id_type, "counter_id", "id_type");
#else // POF_SHT_VXLAN
    DEBUG_VAR(*p, counter_id, "%u");
#endif // POF_SHT_VXLAN
}

static void action_GROUP(const void *ph){
    pof_action_group *p = (pof_action_group *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"group_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->group_id);
}

static void action_SET_FIELD(const void *ph){
    pof_action_set_field *p = (pof_action_set_field *)ph;

#ifdef POF_SHT_VXLAN
    POF_DEBUG_CPRINT(1,CYAN,"dst_field=");
    match(&p->dst_field);
    value_byte(&p->src, p->src_type, "src", "src_type", POF_MAX_FIELD_LENGTH_IN_BYTE);
#else // POF_SHT_VXLAN
    match_x(&p->field_setting);
#endif // POF_SHT_VXLAN
}

static void action_SET_FIELD_FROM_METADATA(const void *ph){
    pof_action_set_field_from_metadata *p = (pof_action_set_field_from_metadata *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"field_setting=");
    match(&p->field_setting);
    POF_DEBUG_CPRINT(1,CYAN,"metadata_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->metadata_offset);
}

static void action_MODIFY_FIELD(const void *ph){
    pof_action_modify_field *p = (pof_action_modify_field *)ph;

    match(&p->field);
    POF_DEBUG_CPRINT(1,CYAN,"increment=");
    POF_DEBUG_CPRINT(1,WHITE,"%d ",p->increment);
}

static void
action_EXPERIMENTER(const void *ph)
{
	POF_DEBUG_CPRINT(1,RED,"Unsupport action type.");
	return;
}

#ifdef POF_SHT_VXLAN
static void
action_ENCAP_VXLAN_HEADER(const unsigned char *ph)
{
    struct pof_action_encap_vxlan_header p = \
        *((struct pof_action_encap_vxlan_header *)ph);
    value_byte(&p.vni, p.vni_type, "vni", "vni_type", 3);
}

static void
action_ENCAP_UDP_HEADER(const unsigned char *ph)
{
    struct pof_action_encap_udp_header p = \
        *((struct pof_action_encap_udp_header *)ph);
    value_16(&p.sport, p.sport_type, "sport", "sport_type");
    value_16(&p.dport, p.dport_type, "dport", "dport_type");
}

static void
action_ENCAP_IP_HEADER(const unsigned char *ph)
{
    struct pof_action_encap_ip_header p = \
        *((struct pof_action_encap_ip_header *)ph);
    value_byte(&p.sip, p.sip_type, "sip", "sip_type", 4);
    value_byte(&p.dip, p.dip_type, "dip", "dip_type", 4);
    value_byte(&p.tos, p.tos_type, "tos", "tos_type", 1);
    DEBUG_VAR(p, protocol, "%u");
}

static void
action_ENCAP_MAC_HEADER(const unsigned char *ph)
{
    struct pof_action_encap_mac_header p = \
        *((struct pof_action_encap_mac_header *)ph);
    value_byte(&p.smac, p.smac_type, "smac", "smac_type", 6);
    value_byte(&p.dmac, p.dmac_type, "dmac", "dmac_type", 6);
	POF_DEBUG_CPRINT(1,CYAN,"ethType=");
	POF_DEBUG_CPRINT_0X_NO_ENTER(&p.ethType, POF_ETH_ALEN);
}

static void
action_CALCULATE_FIELD(const void *ph)
{
	struct pof_action_calc_field *p = \
			(struct pof_action_calc_field *)ph;

	POF_DEBUG_CPRINT(1,CYAN,"calc_type=");
	switch(p->calc_type){
#define CALCULATION(NAME,VALUE) \
		case POFCT_##NAME:								\
			POF_DEBUG_CPRINT(1,WHITE,"POFCT_"#NAME" ");	\
			break;

		CALCULATIONS
#undef CALCULATION
		default:
			POF_DEBUG_CPRINT(1,RED,"Unkown calculate type. ");
			break;
	}
	
	POF_DEBUG_CPRINT(1,CYAN,"dst_field=");
	match(&p->dst_field);

	value(&p->src_operand, p->src_type, "src_operand", "src_type");
}
#endif // POF_SHT_VXLAN

static void 
action(const void *ph)
{
    pof_action *p = (pof_action *)ph;

    POF_DEBUG_CPRINT(1,PINK,"[action] ");
    POF_DEBUG_CPRINT(1,CYAN,"type=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->type);
    POF_DEBUG_CPRINT(1,CYAN,"len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->len);

    switch(p->type){
#define ACTION(NAME,VALUE) \
		case POFAT_##NAME:							\
			POF_DEBUG_CPRINT(1,PINK,"["#NAME"] ");	\
			action_##NAME(p->action_data);			\
			break;

		ACTIONS
#undef ACTION
        default:
            POF_DEBUG_CPRINT(1,RED,"Unknow action type");
			break;
    }
}

static void instruction_APPLY_ACTIONS(const void *ph){
    pof_instruction_apply_actions *p = (pof_instruction_apply_actions *)ph;
    struct pof_action *act = p->action;
    uint8_t i, actNum = p->action_num;

    POF_DEBUG_CPRINT(1,CYAN,"action_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->action_num);

    for(i=0; i<p->action_num; i++){
        action(act);
#ifdef POF_SHT_VXLAN
        act = (struct pof_action *)((uint8_t *)act + act->len);
#else // POF_SHT_VXLAN
        act ++;
#endif // POF_SHT_VXLAN
    }
}

static void instruction_GOTO_TABLE(const void *ph){
    pof_instruction_goto_table *p = (pof_instruction_goto_table *)ph;
    uint32_t i;

    POF_DEBUG_CPRINT(1,CYAN,"next_table_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->next_table_id);
    POF_DEBUG_CPRINT(1,CYAN,"match_field_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->match_field_num);
    POF_DEBUG_CPRINT(1,CYAN,"packet_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->packet_offset);

    for(i=0; i<p->match_field_num; i++){
        match(&p->match[i]);
    }
}

static void instruction_GOTO_DIRECT_TABLE(const void *ph){
    pof_instruction_goto_direct_table *p = (pof_instruction_goto_direct_table *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"next_table_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->next_table_id);
#ifdef POF_SD2N
	value(&p->table_entry_index, p->index_type, "table_entry_index", "index_type");
#else // POF_SD2N
    POF_DEBUG_CPRINT(1,CYAN,"table_entry_index=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->table_entry_index);
#endif // POF_SD2N
    POF_DEBUG_CPRINT(1,CYAN,"packet_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->packet_offset);
}

static void instruction_METER(const void *ph){
    pof_instruction_meter *p = (pof_instruction_meter *)ph;
#ifdef POF_SHT_VXLAN
    value(&p->meter_id, p->id_type, "meter_id", "id_type");
#else // POF_SHT_VXLAN
    DEBUG_VAR(*p, meter_id, "%u");
#endif // POF_SHT_VXLAN
}

static void instruction_WRITE_METADATA(const void *ph){
    pof_instruction_write_metadata *p = (pof_instruction_write_metadata *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"metadata_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->metadata_offset);
    POF_DEBUG_CPRINT(1,CYAN,"len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->len);
#ifdef POF_SD2N
    POF_DEBUG_CPRINT(1,CYAN,"value=");
    POF_DEBUG_CPRINT_0X_NO_ENTER(p->value, POF_MAX_FIELD_LENGTH_IN_BYTE);
#else // POF_SD2N
    POF_DEBUG_CPRINT(1,CYAN,"value=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.8x ",p->value);
#endif // POF_SD2N
}

static void instruction_WRITE_METADATA_FROM_PACKET(const void *ph){
    pof_instruction_write_metadata_from_packet *p = \
                        (pof_instruction_write_metadata_from_packet *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"metadata_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->metadata_offset);
    POF_DEBUG_CPRINT(1,CYAN,"packet_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->packet_offset);
    POF_DEBUG_CPRINT(1,CYAN,"len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->len);
}

static void
instruction_WRITE_ACTIONS(const void *ph)
{
	POF_DEBUG_CPRINT(1,RED,"Unsupport instruction type.");
	return;
}

static void
instruction_CLEAR_ACTIONS(const void *ph)
{
	POF_DEBUG_CPRINT(1,RED,"Unsupport instruction type.");
	return;
}

static void
instruction_EXPERIMENTER(const void *ph)
{
	POF_DEBUG_CPRINT(1,RED,"Unsupport instruction type.");
	return;
}

static void
direction(uint8_t direction, const char *direction_name)
{
	POF_DEBUG_CPRINT(1,CYAN,"%s=", direction_name);
	if(direction == POFD_FORWARD){
		POF_DEBUG_CPRINT(1,WHITE,"POFD_FORWARD ");
	}else{
		POF_DEBUG_CPRINT(1,WHITE,"POFD_BACKWARD ");
	}
}

#ifdef POF_SD2N
static void
instruction_CONDITIONAL_JMP(const void *ph)
{
	struct pof_instruction_conditional_jump *p = \
			(struct pof_instruction_conditional_jump *)ph;

	POF_DEBUG_CPRINT(1,CYAN,"compare_field1=");
	match(&p->compare_field1);

	value(&p->compare_field2, p->field2_type, "compare_field2", "field2_type");

	direction(p->offset1_direction, "offset1_direction");
	value(&p->offset1, p->offset1_type, "offset1", "offset1_type");

	direction(p->offset2_direction, "offset2_direction");
	value(&p->offset2, p->offset2_type, "offset2", "offset2_type");

	direction(p->offset3_direction, "offset3_direction");
	value(&p->offset3, p->offset3_type, "offset3", "offset3_type");
}

static void
instruction_CALCULATE_FIELD(const void *ph)
{
	struct pof_instruction_calc_field *p = \
			(struct pof_instruction_calc_field *)ph;

	POF_DEBUG_CPRINT(1,CYAN,"calc_type=");
	switch(p->calc_type){
#define CALCULATION(NAME,VALUE) \
		case POFCT_##NAME:								\
			POF_DEBUG_CPRINT(1,WHITE,"POFCT_"#NAME" ");	\
			break;

		CALCULATIONS
#undef CALCULATION
		default:
			POF_DEBUG_CPRINT(1,RED,"Unkown calculate type. ");
			break;
	}
	
	POF_DEBUG_CPRINT(1,CYAN,"dst_field=");
	match(&p->dst_field);

	value(&p->src_operand, p->src_type, "src_operand", "src_type");
}

static void
instruction_MOVE_PACKET_OFFSET(const void *ph)
{
	struct pof_instruction_mov_packet_offset *p = \
			(struct pof_instruction_mov_packet_offset *)ph;

	direction(p->direction, "direction");
	value(&p->movement, p->value_type, "movement", "value_type");
}
#endif // POF_SD2N

#ifdef POF_SHT_VXLAN
static void slot_status(const unsigned char *ph){
    struct pof_slot_status p = *((struct pof_slot_status *)ph);
    pof_HtoN_transfer_slot_status(&p);

    DEBUG_VAR(p, slotID,        "0x%.4x");
    DEBUG_VAR(p, slot_status,   "%u");
    DEBUG_VAR(p, resend_flag,   "%u");
}

static void
slot_config(const unsigned char *ph)
{
    struct pof_slot_config p = *((struct pof_slot_config *)ph);
    pof_HtoN_transfer_slot_config(&p);
    DEBUG_VAR(p, slotID,    "0x%.4x");
    DEBUG_VAR(p, flag,      "0x%.2x");
}

static void
insBlock(const unsigned char *ph)
{
    struct pof_instruction_block p = *((struct pof_instruction_block *)ph);
    struct pof_instruction *ins = p.instruction;
    uint8_t i, insNum = p.instruction_num;
    pof_NtoH_transfer_insBlock(&p);
    DEBUG_VAR(p, command,               "0x%.2x");
    DEBUG_VAR(p, instruction_num,       "%u");
    DEBUG_VAR(p, related_table_id,      "%u");
    DEBUG_VAR(p, slotID,                "0x%.4x");
    DEBUG_VAR(p, instruction_block_id,  "%u");

    for(i=0; i<insNum; i++){
        instruction(ins);
        ins = (struct pof_instruction *)( \
                (uint8_t *)ins + ins->len );
    }
}

static void
instruction_SET_PACKET_OFFSET(const unsigned char *ph)
{
    struct pof_instruction_set_packet_offset p = \
        *((struct pof_instruction_set_packet_offset *)ph);

	POF_DEBUG_CPRINT(1,CYAN,"offset_type=");
	if(p.offset_type == POFVT_IMMEDIATE_NUM){
		POF_DEBUG_CPRINT(1,WHITE,"POFVT_IMMEDIATE_NUM ");
		POF_DEBUG_CPRINT(1,CYAN,"offset=");
		POF_DEBUG_CPRINT(1,CYAN,"[value]");
		POF_DEBUG_CPRINT(1,WHITE,"%d ",p.offset.value);
	}else{
		POF_DEBUG_CPRINT(1,WHITE,"POFVT_FIELD ");
		POF_DEBUG_CPRINT(1,CYAN,"offset=");
		match(&p.offset.field);
	}
}

static void
instruction_COMPARE(const unsigned char *ph)
{
    struct pof_instruction_compare p = \
        *((struct pof_instruction_compare *)ph);
    DEBUG_VAR(p, comp_result_field_num, "%u");
    POF_DEBUG_CPRINT(1,CYAN,"operand1=");
    match(&p.operand1);
    value(&p.operand2, p.operand2_type, "operand2", "operand2_type");
}

static void
instruction_BRANCH(const unsigned char *ph)
{
    struct pof_instruction_branch p = \
        *((struct pof_instruction_branch *)ph);
    DEBUG_VAR(p, skip_instruction_num, "%u");
    POF_DEBUG_CPRINT(1,CYAN,"operand1=");
    match(&p.operand1);
    value(&p.operand2, p.operand2_type, "operand2", "operand2_type");
}

static void
instruction_JMP(const unsigned char *ph)
{
    struct pof_instruction_jmp p = \
        *((struct pof_instruction_jmp *)ph);
    DEBUG_VAR(p, direction,         "%u");
    DEBUG_VAR(p, instruction_num,   "%u");
}

#endif // POF_SHT_VXLAN

static void instruction(const void *ph){
    pof_instruction *p = (pof_instruction *)ph;

    POF_DEBUG_CPRINT(1,PINK,"[instruction] ");
    POF_DEBUG_CPRINT(1,CYAN,"type=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->type);
    POF_DEBUG_CPRINT(1,CYAN,"len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->len);

    switch(p->type){
#define INSTRUCTION(NAME,VALUE)							\
		case POFIT_##NAME:								\
			POF_DEBUG_CPRINT(1,PINK,"["#NAME"] ");		\
			instruction_##NAME(p->instruction_data);	\
			break;

		INSTRUCTIONS
#undef INSTRUCTION
        default:
            POF_DEBUG_CPRINT(1,RED,"Unknown instruction ");
			break;
    }
}

static void 
poflp_flow_entry_simple(const void *ph)
{
    struct entryInfo *p = (struct entryInfo *)ph;
    int i;

    POF_DEBUG_CPRINT(1,PINK,"<entry[%d]> ",p->index);
    POF_DEBUG_CPRINT(1,CYAN,"index=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->index);
    POF_DEBUG_CPRINT(1,CYAN,"counter_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->counter_id);
    POF_DEBUG_CPRINT(1,CYAN,"priority=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->priority);
    POF_DEBUG_CPRINT(1,CYAN,"keyLen=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->keyLen);
    POF_DEBUG_CPRINT(1,CYAN,"value=");
    POF_DEBUG_CPRINT_0X_NO_ENTER(p->value, POF_BITNUM_TO_BYTENUM_CEIL(p->keyLen));
    POF_DEBUG_CPRINT(1,CYAN,"mask=");
    POF_DEBUG_CPRINT_0X_NO_ENTER(p->mask, POF_BITNUM_TO_BYTENUM_CEIL(p->keyLen));
#ifdef POF_SHT_VXLAN
    DEBUG_VAR(*p, insBlockID, "%u");
    DEBUG_VAR(*p, paraLen,   "%u");
    POF_DEBUG_CPRINT(1,CYAN,"para=");
    POF_DEBUG_CPRINT_0X_NO_ENTER(p->para, POF_BITNUM_TO_BYTENUM_CEIL(p->paraLen));
#else // POF_SHT_VXLAN
    POF_DEBUG_CPRINT(1,CYAN,"instruction_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->instruction_num);

    for(i=0; i<p->instruction_num; i++){
        instruction(&p->instruction[i]);
    }
#endif // POF_SHT_VXLAN

	return;
}

void 
poflp_flow_entry(const void *ph)
{
    pof_flow_entry *p = (pof_flow_entry *)ph;
    int i;

    POF_DEBUG_CPRINT(1,PINK,"<entry[%d][%d][%d]> ",p->table_type,
            p->table_id, p->index);
    POF_DEBUG_CPRINT(1,CYAN,"command=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->command);
    POF_DEBUG_CPRINT(1,CYAN,"slotID=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->slotID);
    POF_DEBUG_CPRINT(1,CYAN,"match_field_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->match_field_num);
    POF_DEBUG_CPRINT(1,CYAN,"counter_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->counter_id);
    POF_DEBUG_CPRINT(1,CYAN,"cookie=");
    LOG_PRINT_U64(p->cookie);
    POF_DEBUG_CPRINT(1,CYAN,"cookie_mask=");
    LOG_PRINT_U64(p->cookie_mask);
    POF_DEBUG_CPRINT(1,CYAN,"table_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->table_id);
    POF_DEBUG_CPRINT(1,CYAN,"table_type=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->table_type);
    POF_DEBUG_CPRINT(1,CYAN,"idle_timeout=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->idle_timeout);
    POF_DEBUG_CPRINT(1,CYAN,"hard_timeout=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->hard_timeout);
    POF_DEBUG_CPRINT(1,CYAN,"priority=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->priority);
    POF_DEBUG_CPRINT(1,CYAN,"index=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->index);
#ifdef POF_SHT_VXLAN
    DEBUG_VAR(*p, instruction_block_id,  "%u");
    DEBUG_VAR(*p, parameter_length,      "%u");
#else // POF_SHT_VXLAN
    POF_DEBUG_CPRINT(1,CYAN,"instruction_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->instruction_num);
#endif // POF_SHT_VXLAN

    for(i=0; i<p->match_field_num; i++){
        match_x(&p->match[i]);
    }

#ifdef POF_SHT_VXLAN
    POF_DEBUG_CPRINT(1,CYAN,"para=");
    POF_DEBUG_CPRINT_0X_NO_ENTER(p->parameters, \
            POF_BITNUM_TO_BYTENUM_CEIL(p->parameter_length));
#else // POF_SHT_VXLAN
    for(i=0; i<p->instruction_num; i++){
        instruction(&p->instruction[i]);
    }
#endif // POF_SHT_VXLAN

	return;
}

static void flow_entry(const unsigned char *ph){
//    pof_flow_entry p = *((pof_flow_entry *)ph);
//    pof_NtoH_transfer_flow_entry(p);
//	poflp_flow_entry(p);
}

static void meter(const unsigned char *ph){
    pof_meter p = *((pof_meter *)ph);

    pof_NtoH_transfer_meter(&p);

    POF_DEBUG_CPRINT(1,CYAN,"command=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.command);
#ifdef POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"slotID=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.slotID);
#endif // POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"rate=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.rate);
    POF_DEBUG_CPRINT(1,CYAN,"meter_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.meter_id);
}

static void group(const unsigned char *ph){
    pof_group p = *((pof_group *)ph);
    uint32_t i;

    pof_NtoH_transfer_group(&p);

    POF_DEBUG_CPRINT(1,CYAN,"command=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.command);
    POF_DEBUG_CPRINT(1,CYAN,"slotID=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.slotID);
    POF_DEBUG_CPRINT(1,CYAN,"type=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.type);
    POF_DEBUG_CPRINT(1,CYAN,"action_number=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.action_number);
    POF_DEBUG_CPRINT(1,CYAN,"group_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.group_id);
    POF_DEBUG_CPRINT(1,CYAN,"counter_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.counter_id);

    for(i=0; i<p.action_number; i++){
        action(&p.action[i]);
    }
}

static void counter(const unsigned char *ph){
    pof_counter p = *((pof_counter *)ph);
    pof_NtoH_transfer_counter(&p);

    POF_DEBUG_CPRINT(1,CYAN,"command=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.command);
#ifdef POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"slotID=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.slotID);
#endif // POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"counter_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.counter_id);
    POF_DEBUG_CPRINT(1,CYAN,"value=");
    LOG_PRINT_U64(p.value);
#ifdef POF_SD2N
    POF_DEBUG_CPRINT(1,CYAN,"byte_value=");
    LOG_PRINT_U64(p.byte_value);
#endif // POF_SD2N
}

static void packet_in(const unsigned char *ph){
    pof_packet_in p = *((pof_packet_in *)ph);
    pof_NtoH_transfer_packet_in(&p);

    POF_DEBUG_CPRINT(1,CYAN,"buffer_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.buffer_id);
    POF_DEBUG_CPRINT(1,CYAN,"total_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.total_len);
    POF_DEBUG_CPRINT(1,CYAN,"reason=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.reason);
    POF_DEBUG_CPRINT(1,CYAN,"table_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.table_id);
    POF_DEBUG_CPRINT(1,CYAN,"cookie=");
    LOG_PRINT_U64(p.cookie);
    POF_DEBUG_CPRINT(1,CYAN,"device_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.device_id);
//#ifdef POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"slotID=");
    POF_DEBUG_CPRINT(1,WHITE,"%.4x ",p.slotID);
    POF_DEBUG_CPRINT(1,CYAN,"port_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%.4x ",p.port_id);
//#endif // POF_MULTIPLE_SLOTS

    POF_DEBUG_CPRINT_0X_NO_ENTER(p.data, p.total_len);
}

static void error(const unsigned char *ph){
    pof_error p = *((pof_error *)ph);
    pof_NtoH_transfer_error(&p);

    POF_DEBUG_CPRINT(1,CYAN,"type=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.type);
    POF_DEBUG_CPRINT(1,CYAN,"code=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.4x ",p.code);
    POF_DEBUG_CPRINT(1,CYAN,"device_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p.device_id);
#ifdef POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"slotID=");
    POF_DEBUG_CPRINT(1,WHITE,"%.4x ",p.slotID);
#endif // POF_MULTIPLE_SLOTS
    POF_DEBUG_CPRINT(1,CYAN,"err_str=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p.err_str);
}

static void queryall_request(const unsigned char *ph){
    struct pof_queryall_request p = *((struct pof_queryall_request *)ph);
    pof_HtoN_transfer_queryall_request(&p);

    POF_DEBUG_CPRINT(1,CYAN,"slotID=");
    POF_DEBUG_CPRINT(1,WHITE,"%.4x ",p.slotID);
}

static void packet_raw(const void *ph, pof_header *header_ptr){
    POF_DEBUG_CPRINT(1,BLUE,"[Header:] ");
    POF_DEBUG_CPRINT(1,CYAN,"Ver=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%x ",header_ptr->version);
    POF_DEBUG_CPRINT(1,CYAN,"Type=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%x ",header_ptr->type);
    POF_DEBUG_CPRINT(1,CYAN,"Len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",header_ptr->length);
    POF_DEBUG_CPRINT(1,CYAN,"Xid=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",header_ptr->xid);

	POF_DEBUG_CPRINT(1,BLUE,"[PACKET_RAW] ");
    POF_DEBUG_CPRINT(1,CYAN,"pofHead=");
    POF_DEBUG_CPRINT_0X_NO_ENTER(ph, sizeof(pof_header));
    POF_DEBUG_CPRINT(1,CYAN," pofBody=");

	if(header_ptr->length < sizeof(pof_header)){
		return;
	}
    POF_DEBUG_CPRINT_0X_NO_ENTER(ph + sizeof(pof_header), header_ptr->length - sizeof(pof_header));

	return;
}

void pof_debug_cprint_packet(const void * ph, uint32_t flag, int len){
    pof_header header = *((pof_header *)ph), *header_ptr = &header;
    pof_type type;
    
    pof_NtoH_transfer_header(header_ptr);

    if( flag == 0 ){
        POF_DEBUG_CPRINT(1,PINK,"Recv ");
    }
    else if( flag == 1 ){
        POF_DEBUG_CPRINT(1,PINK,"Send ");
    }

    POF_DEBUG_CPRINT(1,PINK,"[%d] ", len);

    type = header_ptr->type;
    switch(type){
        case POFT_HELLO:
            POF_DEBUG_CPRINT(1,PINK,"[HELLO:] ");
			packet_raw(ph, header_ptr);
            break;
        case POFT_ECHO_REQUEST:
            POF_DEBUG_CPRINT(1,PINK,"[ECHO_REQUEST:] ");
			packet_raw(ph, header_ptr);
            break;
        case POFT_ECHO_REPLY:
            POF_DEBUG_CPRINT(1,PINK,"[ECHO_REPLY:] ");
			packet_raw(ph, header_ptr);
            break;
        case POFT_FEATURES_REQUEST:
            POF_DEBUG_CPRINT(1,PINK,"[FEATURES_REQUEST:] ");
			packet_raw(ph, header_ptr);
            break;
        case POFT_GET_CONFIG_REQUEST:
            POF_DEBUG_CPRINT(1,PINK,"[GET_CONFIG_REQUEST:]");
			packet_raw(ph, header_ptr);
            break;
        case POFT_GET_CONFIG_REPLY:
            POF_DEBUG_CPRINT(1,PINK,"[GET_CONFIG_REPLY:]");
            switch_config((unsigned char *)ph+sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_SET_CONFIG:
            POF_DEBUG_CPRINT(1,PINK,"[SET_CONFIG:]");
            set_config((uint8_t *)ph+sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_FEATURES_REPLY:
            POF_DEBUG_CPRINT(1,PINK,"[FEATURES_REPLY:] ");
            switch_features((unsigned char *)ph+sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_RESOURCE_REPORT:
            POF_DEBUG_CPRINT(1,PINK,"[RESOURCE_REPORT:] ");
            flow_table_resource((unsigned char *)ph+sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_PORT_STATUS:
            POF_DEBUG_CPRINT(1,PINK,"[PORT_STATUS:] ");
            port_status((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_TABLE_MOD:
            POF_DEBUG_CPRINT(1,PINK,"[TABLE_MOD:] ");
            flow_table((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_FLOW_MOD:
            POF_DEBUG_CPRINT(1,PINK,"[FLOW_MOD:] ");
            flow_entry((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_METER_MOD:
            POF_DEBUG_CPRINT(1,PINK,"[METER_MOD:] ");
            meter((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_GROUP_MOD:
            POF_DEBUG_CPRINT(1,PINK,"[GROUP_MOD:] ");
            group((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_COUNTER_MOD:
            POF_DEBUG_CPRINT(1,PINK,"[COUNTER_MOD:] ");
            counter((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_PORT_MOD:
            POF_DEBUG_CPRINT(1,PINK,"[PORT_MOD:] ");
            port_status((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_COUNTER_REQUEST:
            POF_DEBUG_CPRINT(1,PINK,"[COUNTER_REQUEST:] ");
			packet_raw(ph, header_ptr);
            break;
        case POFT_COUNTER_REPLY:
            POF_DEBUG_CPRINT(1,PINK,"[COUNTER_REPLY:] ");
            counter((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_PACKET_IN:
            POF_DEBUG_CPRINT(1,PINK,"[PACKET_IN:] ");
            packet_in((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_PACKET_OUT:
            POF_DEBUG_CPRINT(1,PINK,"[PACKET_OUT:] ");
			packet_raw(ph, header_ptr);
            break;
        case POFT_ERROR:
            POF_DEBUG_CPRINT(1,PINK,"[ERROR:] ");
            error((uint8_t *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_QUERYALL_REQUEST:
            POF_DEBUG_CPRINT(1,PINK,"[QUERYALL_REQUEST:] ");
            queryall_request((uint8_t *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_QUERYALL_FIN:
            POF_DEBUG_CPRINT(1,PINK,"[QUERYALL_FIN:] ");
			packet_raw(ph, header_ptr);
            break;
#ifdef POF_SHT_VXLAN
        case POFT_INSTRUCTION_BLOCK_MOD:
            POF_DEBUG_CPRINT(1,PINK,"[INS_BLOCK_MOD:] ");
            insBlock((uint8_t *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_SLOT_CONFIG:
            POF_DEBUG_CPRINT(1,PINK,"[SLOT_CONFIG:] ");
            slot_config((uint8_t *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_SLOT_STATUS:
            POF_DEBUG_CPRINT(1,PINK,"[SLOT_STATUS:] ");
            slot_status((uint8_t *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
#endif // POF_SHT_VXLAN
        default:
            POF_DEBUG_CPRINT(1,RED,"Unknown POF type. ");
            break;
    }
    POF_DEBUG_CPRINT(1,PINK,"\n");
}

struct log_util g_log = {
	NULL,
	PTHREAD_MUTEX_INITIALIZER,
	0,
#ifdef POF_DEBUG_COLOR_ON
	POFE_ENABLE,
#else // POF_DEBUG_COLOR_ON
	POFE_DISABLE,
#endif // POF_DEBUG_COLOR_ON

#ifdef POF_DEBUG_ON
	POFE_ENABLE,
#else // POF_DEBUG_ON
	POFE_DISABLE,
#endif // POF_DEBUG_ON

	POFE_ENABLE,
	POFE_ENABLE,
};
