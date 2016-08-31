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
#include "../include/pof_local_resource.h"
#include "../include/pof_conn.h"
#include "../include/pof_datapath.h"
#include "../include/pof_byte_transfer.h"
#include "../include/pof_log_print.h"
#include "../include/pof_hmap.h"
#include "../include/pof_list.h"
#include "../include/pof_memory.h"
#include "string.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "net/if.h"
#include "sys/ioctl.h"
#include "arpa/inet.h"
#include "ifaddrs.h"

//add by wenjian 2015/12/02
#include <fcntl.h>
#include <inttypes.h>
#include <linux/if_tun.h>
#include <linux/if_packet.h>
#include <linux/ethtool.h>
#include <linux/rtnetlink.h>
#include <linux/sockios.h>
#include <linux/version.h>
#include <sys/types.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <stdlib.h>
#include <unistd.h>


struct portName {
    char name[PORT_NAME_LEN];
    uint16_t slotID;
    struct listNode node;
};

struct list portNameList = {
    {&portNameList.nil, &portNameList.nil}, 0
};

/* Port index base. */
uint8_t portIndex = 1;



//add by jelly
//qdisc is an abbr of queue discipline
/* All queues in a port, lie beneath a qdisc */
#define TC_QDISC 0x0001
/* This is a root class. In order to efficiently share excess bandwidth
 * tc requires that all classes are under a common root class */
#define TC_ROOT_CLASS 0xffff
/* This is the queue_id for packets that do not match in any other queue.
 * It has min_rate = 0. This is a placeholder for best-effort traffic
 * without any bandwidth guarantees */
#define TC_DEFAULT_CLASS 0xfffe
#define TC_MIN_RATE 1
/* This configures an HTB qdisc under the defined device. */
/*
 * the example of adding dev qdisc
 * tc qdisc add dev eth0 root handle 1: htb default 12
 * 杩欐潯鍛戒护鍒嗛厤浜嗕竴涓狧TB闃熷垪缁�eth0 骞朵笖鎸囧畾浜嗕竴涓悕绉颁负(handle" 1 鍙ユ焺 1: , 杩欎釜鍚嶇О鐢ㄤ簬鏍囪瘑瀹冧笅闈㈢殑瀛愮被, default 12 鐨勬剰鎬濇槸娌℃湁琚垎绫荤殑娴侀噺琚垎閰嶅埌绫�1:12
 */
#define COMMAND_ADD_DEV_QDISC "/sbin/tc qdisc add dev %s " \
                              "root handle %x: htb default %x"
#define COMMAND_DEL_DEV_QDISC "/sbin/tc qdisc del dev %s root"

/* This configures class under htb*/
/* an example
 * there must be at least one root class define like :
 * tc class add dev eth0 parent 1: classid 1:1 htb rate 100kbps ceil 100kbps
 * then we can define subclass adhereing to the root class
 * tc class add dev eth0 parent 1:1 classid 1:10 htb rate 30kbps ceil 100kbps
 *
 */
#define COMMAND_ADD_CLASS "/sbin/tc class add dev %s parent %x:%x " \
                          "classid %x:%x htb rate %dkbit ceil %dkbit"
#define COMMAND_CHANGE_CLASS "/sbin/tc class change dev %s parent %x:%x " \
                             "classid %x:%x htb rate %dkbit ceil %dkbit"
#define COMMAND_DEL_CLASS "/sbin/tc class del dev %s parent %x:%x classid %x:%x"

//add by little jelly
uint32_t
setNonBlocking(int fd)
{
	//get the file flag which is the second parameter for "open" function
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) {
    	//set the file to be nonblock if it is blocked;
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1) {
            return POF_OK;
        } else {
            return POF_ERROR;
        }
    } else {
        return POF_ERROR;
    }
}

/** Sets the priority for the socket. Returns 0 if successful,
 * otherwise a positive errno value */
uint32_t
setSocketPriority(int fd, int priority)
{
    int prio = priority;
    return setsockopt(fd, SOL_SOCKET, SO_PRIORITY, (char *)&prio, sizeof(prio));
}


//add by little jelly
//date 2015 3 22
static uint32_t
setPortFlag(struct portInfo *port,enum portFlags flag)
{
	 struct ifreq ifr;
	 strncpy(ifr.ifr_name, port->name, sizeof ifr.ifr_name);
	 ifr.ifr_flags = flag;
	 int af_inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
	 if(af_inet_sock<0)
	 {
		 close(af_inet_sock);
		 return POF_ERROR;
	 }
	 else if (ioctl(af_inet_sock, SIOCSIFFLAGS, &ifr) < 0) {
		    close(af_inet_sock);
	        return POF_ERROR;
	 }
	 close(af_inet_sock);
	 return POF_OK;
}


//add by little jelly
//date 2015 3 22
/** open and bind a socket to it which receive data
 *
 */
static uint32_t
openPort(struct portInfo * port)
{
	struct   sockaddr_ll sockadr = {0}, from = {0};
	int      sock;
	int      ret;
	if((sock = socket(AF_PACKET, SOCK_RAW, POF_HTONS(ETH_P_ALL))) == -1){
	        POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE, g_upward_xid++);
	        /* Delay 0.1s to send error message upward to the Controller. */
	        pofbf_task_delay(100);
	        terminate_handler();
	}
	 sockadr.sll_family = PF_PACKET;
	 sockadr.sll_protocol = POF_HTONS(ETH_P_ALL);
	 sockadr.sll_ifindex = port->sysIndex;
	 if(bind(sock, (struct sockaddr *)&sockadr, sizeof(struct sockaddr_ll)) != 0){
	       POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_BIND_SOCKET_FAILURE, g_upward_xid++);
	       /* Delay 0.1s to send error message upward to the Controller. */
	       pofbf_task_delay(100);
	       terminate_handler();
	 }
	//ret=setNonBlocking(sock);
	//if(ret==POF_ERROR)
	//{
	//	 POF_DEBUG_CPRINT_FL(1,RED,"setNonBlocking error!");
	//	 terminate_handler();
	//}
	//POF_DEBUG_CPRINT_FL(1,GREEN,"setNonBlocking ok!");
	// set the flag like promisc
	//ret=setPortFlag(port,NETDEV_UP | NETDEV_PROMISC);
	//if(ret==POF_ERROR)
	//{
    // 	 POF_DEBUG_CPRINT_FL(1,RED,"setPortFlag error!");
	//     terminate_handler();
	//}
	port->queue_fd[0]=sock;
	port->num_queues=1;
	return POF_OK;
}

//add by little jelly
//date 2015 3 23
static uint32_t
openQueueSocket(const char * name, uint16_t class_id, int * fd)
{
	struct ifreq ifr;
	struct sockaddr_ll sll;
	uint32_t priority;
	uint32_t ret;
	//for size
	int size;
	int err;
	socklen_t optlen;

	*fd = socket(AF_PACKET, SOCK_RAW, POF_HTONS(ETH_P_ALL)); /* this is a write-only sock */
	if (*fd < 0) {
		 POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE, g_upward_xid++);
	     /* Delay 0.1s to send error message upward to the Controller. */
		 pofbf_task_delay(100);
		 terminate_handler();
	}
	return POF_OK;
}

/** Setup a classful queue for the specific device. Configured according to
 * HTB protocol. Note that this is linux specific. You will need to replace
 * this with the appropriate abstraction for different OS.
 *
 * The default configuration includes a root class and a default queue/class.
 * A root class is neccesary for efficient use of "unused" bandwidth. If we
 * have traffic A and B (given 80% and 20% of the link respectively), B can use
 * more than 20% if A doesn't use all its bandwidth. In order to allow this
 * "sharing", all queues must reside under a common root class.
 * A default queue/class is a queue  where "unclassified" traffic will fall to.
 * The default class has a best-effort behavior.
 *
 * More on Linux Traffic Control and Hierarchical Token Bucket(HTB) at :
 * http://luxik.cdi.cz/~devik/qos/htb/
 * http://luxik.cdi.cz/~devik/qos/htb/manual/userg.htm
 *
 * @param netdev_name the device to be configured
 * @return 0 on success, non-zero value when the configuration was not
 * successful.
 */
static uint32_t
doSetupQdisc(const char *netdev_name)
{
    char command[1024];
    int error;

    snprintf(command, sizeof(command), COMMAND_ADD_DEV_QDISC, netdev_name,
             TC_QDISC, TC_DEFAULT_CLASS);
    error = system(command);
    if (error) {
        return POF_ERROR;
    }
    return POF_OK;
}

/** Remove current queue disciplines from a net device
 * @param netdev_name the device under configuration
 */
static uint32_t
doRemoveQdisc(const char *netdev_name)
{
    char command[1024];

    snprintf(command, sizeof(command), COMMAND_DEL_DEV_QDISC, netdev_name);
    system(command);

    /* There is no need for a device to already be configured. Therefore no
     * need to indicate any error */
    return POF_OK;
}

/*setup root class for qdisc and it just command line like shell*/
static uint32_t
portSetupRootClass(const char* name,uint16_t class_id,uint16_t rate)
{
	char command[1024];
	int actual_rate;
	/*we need to translate from .1% to kbps*/
	//the port speed is 1000mbs default
	actual_rate=rate*1000;
	snprintf(command,sizeof(command),COMMAND_ADD_CLASS,name,
			TC_QDISC,0,TC_QDISC, class_id, actual_rate, 1000*1000);
	if(system(command)!=0)
	{
		return POF_ERROR;
	}
	return POF_OK;
}

/** Defines a class for the specific queue discipline. A class
 * represents an OpenFlow queue.
 *
 * @param netdev the device under configuration
 * @param class_id unique identifier for this queue. TC limits this to 16-bits,
 * so we need to keep an internal mapping between class_id and OpenFlow
 * queue_id
 * @param rate the minimum rate for this queue in kbps
 * @return 0 on success, non-zero value when the configuration was not
 * successful.
 */
//it just looks like netdev_setup_root_class
static uint32_t
portSetupClass(const char *name, uint16_t class_id,
                   uint16_t rate)
{
    char command[1024];
    int actual_rate;

    /* we need to translate from .1% to kbps */
    actual_rate = rate*1000;

    snprintf(command, sizeof(command), COMMAND_ADD_CLASS, name,
             TC_QDISC, TC_ROOT_CLASS, TC_QDISC, class_id, actual_rate,
             1000*1000);
    if (system(command) != 0) {
        return POF_ERROR;
    }

    return POF_OK;
}

/** Changes a class already defined.
 *
 * @param netdev the device under configuration
 * @param class_id unique identifier for this queue. TC limits this to 16-bits,
 * so we need to keep an internal mapping between class_id and OpenFlow
 * queue_id
 * @param rate the minimum rate for this queue in kbps
 * @return 0 on success, non-zero value when the configuration was not
 * successful.
 */
static uint32_t
portChangeClass(const char* name, uint16_t class_id, uint16_t rate)
{
    char command[1024];
    int actual_rate;

    /* we need to translate from .1% to kbps */
    actual_rate = rate*1000;

    snprintf(command, sizeof(command), COMMAND_CHANGE_CLASS, name,
             TC_QDISC, TC_ROOT_CLASS, TC_QDISC, class_id, actual_rate,
             1000*1000 );
    if (system(command) != 0) {
        return POF_ERROR;
    }

    return POF_OK;
}


/** Deletes a class already defined to represent an OpenFlow queue.
 *
 * @param netdev the device under configuration
 * @param class_id unique identifier for this queue.
 * @param rate the minimum rate for this queue in kbps
 * @return 0 on success, non-zero value when the configuration was not
 * successful.
 */
static uint32_t
portDeleteClass(const char* name, uint16_t class_id)
{
    char command[1024];

    snprintf(command, sizeof(command), COMMAND_DEL_CLASS, name,
             TC_QDISC, TC_ROOT_CLASS, TC_QDISC, class_id);
    if (system(command) != 0) {
        return POF_ERROR;
    }

    return POF_OK;
}

//add by little jelly
//date 2015 3 23
static uint32_t
portSlicing(struct portInfo * port,uint16_t num_queues)
{
    int   i;
    int *fd;
    int ret;

    port->num_queues+=num_queues;
    //when i=0 ,it means the default queue,a queue binds a socket
    for (i=1; i <port->num_queues; i++) {

    	 fd = &port->queue_fd[i];
         ret = openQueueSocket(port->name,i,fd);
         if(ret==POF_ERROR)
         {
             POF_DEBUG_CPRINT_FL(1,RED,"open queue socket fail!");
             return POF_ERROR;
          }
     }

	return POF_OK;
}
////



static uint32_t
portNameInsert(const char *name, uint16_t slotID, struct list *list)
{
    struct portName *portName;
    POF_MALLOC_SAFE_RETURN(portName, 1, POF_ERROR);
    strncpy(portName->name, name, PORT_NAME_LEN);
    portName->slotID = slotID;
    list_nodeInsertTail(list, &portName->node);
    return POF_OK;
}

static hash_t
map_portHashByID(uint8_t id)
{
    return hmap_hashForLinear(id);
}

static hash_t
map_portHashByName(const char *name)
{
    return hmap_hashForString(name);
}

static void
map_portInsert(struct portInfo *port, struct pof_local_resource *lr)
{
    hmap_nodeInsert(lr->portPofIndexMap, &port->pofIndexNode);
    hmap_nodeInsert(lr->portNameMap, &port->nameNode);
    lr->portNum ++;
}

/* Malloc memory for port information. should be free by map_portDelete(). */
static struct portInfo *
map_portCreate()
{
    struct portInfo *port;
    POF_MALLOC_SAFE_RETURN(port, 1, NULL);
    return port;
}

static void
map_portDelete(struct portInfo *port, struct pof_local_resource *lr)
{
    hmap_nodeDelete(lr->portPofIndexMap, &port->pofIndexNode);
    hmap_nodeDelete(lr->portNameMap, &port->nameNode);
    lr->portNum --;
    FREE(port);
}

/* Check whether there is a port with the name in the system. */
static uint32_t 
checkPortNameInSys(const char *name)
{
    struct ifreq ifr; /* Interface request. */
    int    sock;
    uint32_t ret;

    if(!name){
        return POF_ERROR;
    }

    if(-1 == (sock = socket(AF_INET, SOCK_STREAM, 0))){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE);
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, name, strlen(name)+1);

    /* Get port index. */
    ret = ioctl(sock, SIOCGIFINDEX, &ifr);
    close(sock);
    return ret;
}

struct portInfo *
poflr_get_port_with_pofindex(uint8_t pofIndex, const struct pof_local_resource *lr)
{
    struct portInfo *port, *ptr;
    return HMAP_STRUCT_GET(port, pofIndexNode, \
            map_portHashByID(pofIndex), lr->portPofIndexMap, ptr);
}

struct portInfo *
poflr_get_port_with_name(const char *name, const struct pof_local_resource *lr)
{
    struct portInfo *port, *ptr;
    return HMAP_STRUCT_GET(port, nameNode, \
            map_portHashByName(name), lr->portNameMap, ptr);
}

/* Remove a port with the name from local resource. */
uint32_t
poflr_del_port(const char *ethName, struct pof_local_resource *lr)
{
    struct portInfo *port;
    uint32_t ret;

    /* Get the port. */
    if( !(port = poflr_get_port_with_name(ethName, lr))){
        return POF_ERROR;
    }

    /* Report to the Controllerr. */
	ret = poflr_port_report(POFPR_DELETE, port);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Shut down the task which listen to the port. */
	ret = pofbf_task_delete(&port->taskID);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Remove the port, and free the memory. */
    map_portDelete(port, lr);

    return POF_OK;
}

/* Check the link state. */
static uint32_t poflr_check_port_link(const char *name){
	struct ifreq ifr;
	int sock_fd;

    if(-1 == (sock_fd = socket(AF_INET, SOCK_DGRAM, 0))){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE);
    }
	strcpy(ifr.ifr_name, name);
	if(ioctl(sock_fd, SIOCGIFFLAGS, &ifr) < 0){
		close(sock_fd);
		return POF_ERROR;
	}

	if((ifr.ifr_flags & IFF_RUNNING)){
		close(sock_fd);
		return POF_OK;
	}
	else{
		close(sock_fd);
		return POF_ERROR;
	}
}

/* Check port up or not. */
static uint32_t poflr_check_port_up(const char *name){
	struct ifreq ifr;
	int sock_fd;

    if(-1 == (sock_fd = socket(AF_INET, SOCK_DGRAM, 0))){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE);
    }
	strcpy(ifr.ifr_name, name);
	if(ioctl(sock_fd, SIOCGIFFLAGS, &ifr) < 0){
		close(sock_fd);
		return POF_ERROR;
	}

	if(ifr.ifr_flags & IFF_UP){
		close(sock_fd);
		return POF_OK;
	}
	else{
		close(sock_fd);
		return POF_ERROR;
	}
}

/* Check the port state. */
static uint32_t
pofStateCheck(const char *name)
{
	if( (POF_OK == poflr_check_port_up(name)) && \
			(POF_OK == poflr_check_port_link(name)) ){
        return POFPS_LIVE;
	}
	else{
        return POFPS_LINK_DOWN;
	}
}

/***********************************************************************
 * Get the HWaddr and the index of the local physical ports.
 * Form:     uint32_t poflr_get_hwaddr_index_by_name(const char *name, \
 *                                              unsigned char *hwaddr, \
                                                char *ip,              \
 *                                              uint32_t *index)
 * Input:    port name string
 * Output:   HWaddr, port index, ip address
 * Return:   POF_OK or ERROR code
 * Discribe: This function will get the HWaddr, the index and the ip 
 *           address of the local physical ports by the port name string.
 ***********************************************************************/
static uint32_t poflr_get_hwaddr_index_ip_by_name(const char *name,      \
                                                  unsigned char *hwaddr, \
                                                  char *ip,              \
                                                  uint32_t *index)
{
    struct ifreq ifr; /* Interface request. */
    struct sockaddr_in *sin;
    int    sock;

    if(!name || !hwaddr || !ip || !index){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_GET_PORT_INFO_FAILURE);
    }

    if(-1 == (sock = socket(AF_INET, SOCK_STREAM, 0))){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE);
    }

    memset(hwaddr, 0, POF_ETH_ALEN);
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, name, strlen(name)+1);

    /* Get hardware address. */
    if(-1 == ioctl(sock, SIOCGIFHWADDR, &ifr)){
        close(sock);
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_GET_PORT_INFO_FAILURE);
    }else{
        memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, POF_ETH_ALEN);
    }

    /* Get port index. */
    if(-1 == ioctl(sock, SIOCGIFINDEX, &ifr)){
        close(sock);
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_GET_PORT_INFO_FAILURE);
    }else{
        *index = ifr.ifr_ifindex;
    }

    /* Get IP address. */
    ioctl(sock, SIOCGIFADDR, &ifr);
    sin = (struct sockaddr_in *)&(ifr.ifr_addr);
    strcpy(ip, inet_ntoa(sin->sin_addr));

    close(sock);
    return POF_OK;
}

/* Set the port information, including the hash value, state.
 * Input:   name, portIndex
 * Output:  port */
static uint32_t 
poflr_set_port(const char *name, struct portInfo *port, struct pof_local_resource *lr)
{
	uint32_t ret = POF_OK;

	ret = poflr_get_hwaddr_index_ip_by_name(name, port->hwaddr, port->ip, &port->sysIndex);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    port->pofIndex = portIndex;
    port->pofState = pofStateCheck(name);
    strcpy(port->name, name);
    port->of_enable = POFE_DISABLE;

    /* Fill the hash value. */
    port->nameNode.hash = map_portHashByName(name);
    port->pofIndexNode.hash = map_portHashByID(portIndex);

    port->slotID = lr->slotID;

    portIndex ++;

    //jelly's key point
        //open and bind a receiving socket to the port
        ret = openPort(port);
        if(ret==POF_ERROR)
        {
        	POF_DEBUG_CPRINT_FL(1,RED,"openPort fail!");
        }
        //use htb and open 1 sending socket
        ret = portSlicing(port,1);
        if(ret==POF_ERROR)
        {
        	POF_DEBUG_CPRINT_FL(1,RED,"portSlicing fail!");
        }

	return POF_OK;
}

/* For detecting a new one during running, and add a new port using pof_command. 
 * Including create, report and task create. */
uint32_t
poflr_add_port(const char *name, struct pof_local_resource *lr)
{
    struct portInfo *port;
    uint32_t ret;

    /* Check whether already have. */
    if(poflr_get_port_with_name(name, lr)){
        POF_ERROR_CPRINT_FL("Add port Failed: Already have %s", name);
        return POF_ERROR;
    }

    /* Check whether system has this port. */
    if(POF_OK != checkPortNameInSys(name)){
        POF_ERROR_CPRINT_FL("There is no port named %s in system", name);
        return POF_ERROR;
    }

    /* Create port, fill the information and insert to the local resource. */
    port = map_portCreate();
    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(port);
    poflr_set_port(name, port, lr);
    map_portInsert(port, lr);

    /* Report to the Controller for adding a new one. */
	ret = poflr_port_report(POFPR_ADD, port);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Create a new task for listening to the port. */
	ret = pofdp_create_port_listen_task(port);
	if(POF_OK != ret){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TASK_CREATE_FAIL);
	}

    return POF_OK;
}

/* For port initialization when pofswitch begin.
 * Just create the port, no report or task. */
static uint32_t
poflr_add_port_no_report_no_task(const char *name, struct pof_local_resource *lr)
{
    struct portInfo *port = poflr_get_port_with_name(name, lr);
    if(port){
        POF_ERROR_CPRINT_FL("Add port Failed: Already have %s", name);
        return POF_ERROR;
    }

    /* Check whether system has this port. */
    if(POF_OK != checkPortNameInSys(name)){
        POF_ERROR_CPRINT_FL("There is no port named %s in system", name);
        return POF_ERROR;
    }

    /* Create port, fill the information and insert to the local resource. */
    port = map_portCreate();
    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(port);
    poflr_set_port(name, port, lr);
    map_portInsert(port, lr);

    return POF_OK;
}

/* Only for command option when start the pofswitch.
 * Only add the port name to the namespace, no creation or report or task. */
uint32_t
poflr_add_port_only_name(const char *name, uint16_t slotID)
{
    uint32_t i, j;

    /* Check whether system has this port. */
    if(POF_OK != checkPortNameInSys(name)){
        POF_ERROR_CPRINT_FL("There is no port named %s in system", name);
        return POF_ERROR;
    }

    portNameInsert(name, slotID, &portNameList);
    return POF_OK;
}

static void poflr_port_print(struct portInfo *p){
    uint32_t i;

	POF_DEBUG_CPRINT(1,WHITE,"\n");
    POF_DEBUG_CPRINT(1,CYAN,"name=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p->name);
    POF_DEBUG_CPRINT(1,CYAN,"hwaddr=");
    POF_DEBUG_CPRINT_0X_NO_ENTER(p->hwaddr, POF_ETH_ALEN);
    POF_DEBUG_CPRINT(1,CYAN,"ip=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p->ip);
    POF_DEBUG_CPRINT(1,CYAN,"sysIndex=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->sysIndex);
    POF_DEBUG_CPRINT(1,CYAN,"pofIndex=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->pofIndex);
    POF_DEBUG_CPRINT(1,CYAN,"pofState=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->pofState);
    POF_DEBUG_CPRINT(1,CYAN,"of_enable=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->of_enable);
    POF_DEBUG_CPRINT(1,CYAN,"taskID=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.lx ",p->taskID);

	POF_DEBUG_CPRINT(1,WHITE,"\n");
}

/***********************************************************************
 * Get the number and the names of the local physical net ports.
 * Form:     static uint32_t poflr_get_port_num_name_by_systemfile(char **name \
                                                                uint16_t *num  \
                                                                uint16_t num_max)
 * Input:    NONE
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function will get the number and the names of the local
 *           physical net ports. The number will be stored in poflr_port_num,
 *           and the names will be stored in portName. 
 ***********************************************************************/
static uint32_t 
poflr_get_port_num_name_by_systemfile(char **name, uint16_t *num, uint16_t num_max)
{
    uint16_t count = 0, s_len = 0;
	FILE *fp = NULL;
	char filename[] = "/proc/net/dev";
	char s_line[512] = "\0";

	if( (fp = fopen(filename, "r")) == NULL ){
		POF_ERROR_CPRINT_FL("Open /proc/net/dev failed.");
		exit(0);
	}

	if(fgets(s_line, sizeof(s_line), fp) == 0){
		POF_ERROR_CPRINT_FL("Open /proc/net/dev failed.");
        exit(0);
    }

	if(fgets(s_line, sizeof(s_line), fp) == 0){
		POF_ERROR_CPRINT_FL("Open /proc/net/dev failed.");
        exit(0);
    }

	while(fgets(s_line, sizeof(s_line), fp)){
		sscanf(s_line, "%*[^1-9a-zA-Z]%[^:]", name[count]);
		s_len = strlen(name[count]);
		if(s_len <= 0)
			continue;
		if(name[count][s_len-1] == ':')
			name[count][s_len-1] = 0;
		if(strcmp(name[count], "lo") == 0)
			continue;

		count ++;
		
		if(count >= num_max)
			break;
	}

	*num = count;

	fclose(fp);
    return POF_OK;
}

/* Thanks Sergio for reporting bug and offering the diff file. */
static uint32_t 
poflr_get_port_num_name_by_getifaddrs(struct list *list)
{
    struct ifaddrs *addrs, *tmp;
    uint16_t slotID = POF_SLOT_ID_BASE;

    getifaddrs(&addrs);
    for(tmp=addrs; tmp!=NULL; tmp=tmp->ifa_next){
        if(tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET){
            if(strcmp(tmp->ifa_name, "lo")==0){
                continue;
            }
            portNameInsert(tmp->ifa_name, slotID, list);
        }
    }
    freeifaddrs(addrs);
    return POF_OK;
}

/***********************************************************************
 * Get the hardware address of the local physical net ports by IP address.
 * Form:     uint32_t poflr_get_hwaddr_by_ipaddr(uint8_t *hwaddr, 
 *                                               char *ipaddr_str, 
 *                                               uint8_t *port_id_ptr)
 * Input:    ipaddr_str
 * Output:   hwaddr
 * Return:   POF_OK or Error code
 * Discribe: This function will get the hardware address of the local
 *           physical net ports by IP address. For the device ID.
 ***********************************************************************/
uint32_t poflr_get_hwaddr_by_ipaddr(uint8_t *hwaddr, char *ipaddr_str, struct pof_local_resource *lr){
    struct portInfo *port = NULL, *next;
    uint32_t i;
    
    if(!hwaddr || !ipaddr_str || !lr){
        POF_ERROR_CPRINT_FL("Get hwaddr by IP address failed!");
        return POF_ERROR;
    }

    HMAP_NODES_IN_STRUCT_TRAVERSE(port, next, pofIndexNode, lr->portPofIndexMap){
        if(strncmp(ipaddr_str, port->ip, POF_IP_ADDRESS_STRING_LEN) == POF_OK){
            memcpy(hwaddr, port->hwaddr, POF_ETH_ALEN);
            //return POF_OK;
            return port->pofIndex;
        }
    }

    return POF_ERROR;
}

/***********************************************************************
 * Initialize the local physical net port infomation.
 * Form:     uint32_t poflr_init_port(struct pof_local_resource *lr)
 * Input:    NONE
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will initialize the local physical net port
 *           infomation, including port name, port index, port MAC address
 *           and so on.
 ***********************************************************************/
uint32_t 
poflr_init_port(struct pof_local_resource *lr)
{
    uint32_t i, j, ret = POF_OK;
    struct portName *portName, *next;

    /* Port map initialization. */
    lr->portPofIndexMap = hmap_create(lr->portNumMax);
    lr->portNameMap = hmap_create(lr->portNumMax);
    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(lr->portPofIndexMap);
    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(lr->portNameMap);

    if(!(lr->portFlag & POFLRPF_FROM_CUSTOM) && lr->slotID == POF_SLOT_ID_BASE){
        /* Detect all ports in the system. */
        ret = poflr_get_port_num_name_by_getifaddrs(&portNameList);
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    }

    /* Create all ports in the namespace, no report or task. */
    LIST_NODES_IN_STRUCT_TRAVERSE(portName, next, node, &portNameList){
        if(portName->slotID != lr->slotID){
            continue;
        }
        ret = poflr_add_port_no_report_no_task(portName->name, lr);
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    }

    return POF_OK;
}

/***********************************************************************
 * Modify the OpenFlow function enable flag of the port.
 * Form:     uint32_t poflr_port_openflow_enable(uint8_t port_id,       \
 *                                          uint8_t ulFlowEnableFlg,    \
 *                                          struct pof_local_resource *lr)
 * Input:    port id, OpenFlow enable flag
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will modify the OpenFlow function flag of the
 *           port.
 ***********************************************************************/
uint32_t poflr_port_openflow_enable(uint32_t port_id, uint8_t ulFlowEnableFlg, struct pof_local_resource *lr){
    struct portInfo *port = NULL;
    uint32_t ret = POF_ERROR;

    /* Get the port. */
    if((port = poflr_get_port_with_pofindex(port_id, lr)) == NULL){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_PORT_MOD_FAILED, POFPMFC_BAD_PORT_ID, g_recv_xid);
    }
    port->of_enable = ulFlowEnableFlg;
    POF_DEBUG_CPRINT_FL(1,GREEN,"Port openflow enable MOD SUC!");
    return POF_OK;
}

/* Disable all port OpenFlow forwarding. */
uint32_t poflr_disable_all_port(struct pof_local_resource *lr){
    struct portInfo *port, *next;
	uint32_t i;
    /* Traverse all ports. */
    HMAP_NODES_IN_STRUCT_TRAVERSE(port, next, pofIndexNode, lr->portPofIndexMap){
        port->of_enable = POFE_DISABLE;
    }
	return POF_OK;
}

/* Detect all ports in the system, and create a new one if doesn't exist in pofswitch. */
static uint32_t 
poflr_port_detect_new_add(struct pof_local_resource *lr)
{
    struct portName *portName, *next;
    uint32_t ret, i;

    if(!(lr->portFlag & POFLRPF_FROM_CUSTOM) && lr->slotID == POF_SLOT_ID_BASE){
        /* Clear the name list. */
        LIST_CLEAR_ALL(portName, next, node, &portNameList);

        /* Detect all ports in the system. */
        ret = poflr_get_port_num_name_by_getifaddrs(&portNameList);
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

        LIST_NODES_IN_STRUCT_TRAVERSE(portName, next, node, &portNameList){
            if(poflr_get_port_with_name(portName->name, lr)){
                /* Already have. */
                continue;
            }
            /* Doesn't have. */
            poflr_add_port(portName->name, lr);
        }
    }

	return POF_OK;
}

/* Update the port information from new one to an old one. */
static void
updatePorts(struct portInfo *port, const struct portInfo *portNew)
{
    memcpy(port->hwaddr, portNew->hwaddr, POF_ETH_ALEN);
    port->sysIndex = portNew->sysIndex;
    port->pofState = portNew->pofState;
}

/* Check whether the two ports are the same. */
static bool
comparePorts(const struct portInfo *p1, const struct portInfo *p2)
{
    return ( (POF_OK == memcmp(p1->hwaddr, p2->hwaddr, POF_ETH_ALEN)) &&  
             (p1->sysIndex == p2->sysIndex) && 
             (p1->pofState == p2->pofState) );
}

/* Detect all old ports in pofswitch.
 * Delete the port which has removed from the system.
 * Modify the port information which has been changed. */
static uint32_t 
poflr_port_detect_old(struct pof_local_resource *lr)
{
    uint32_t ret;
    struct portInfo *port, *next, tmp;

    /* Traverse all ports. */
    HMAP_NODES_IN_STRUCT_TRAVERSE(port, next, pofIndexNode, lr->portPofIndexMap){
        /* Check whether the system still have the port. */
        if(checkPortNameInSys(port->name) != POF_OK){
            poflr_del_port(port->name, lr);
            continue;
        }
        /* Get the latest information and the state of the port to compare to the old one. */
        ret = poflr_get_hwaddr_index_ip_by_name(port->name, tmp.hwaddr, tmp.ip, &tmp.sysIndex);
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
        tmp.pofState = pofStateCheck(port->name);
        if(comparePorts(port, &tmp) != TRUE){
            /* If the port has been changed, update the port information and report to Controller. */
            updatePorts(port, &tmp);
            ret = poflr_port_report(POFPR_MODIFY, port);
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
        }
    }

	return POF_OK;
}

static uint32_t 
poflr_port_detect_main(struct pof_local_resource *lr)
{
	uint32_t ret = POF_OK;

	/* Detect all of ports infomation right now. */
	ret = poflr_port_detect_old(lr);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	/* Detect all of ports infomation right now. */
	ret = poflr_port_detect_new_add(lr);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	return POF_OK;
}

/* Detect all of ports, and report to Controller if any change. */
uint32_t poflr_port_detect_task(){
	uint32_t ret = POF_OK;
    struct pof_local_resource *lr, *next;
    struct pof_datapath *dp = &g_dp;

	while(1){
        /* Detect once per 5 seconds. */
		sleep(5);
        HMAP_NODES_IN_STRUCT_TRAVERSE(lr, next, slotNode, dp->slotMap){
            ret = poflr_port_detect_main(lr);
            POF_CHECK_RETVALUE_TERMINATE(ret);
        }
	}
}

static void
port_report_fill_msg(struct pof_port *p, const struct portInfo *port)
{
#ifdef POF_MULTIPLE_SLOTS
    p->slotID = port->slotID;
#endif // POF_MULTIPLE_SLOTS
    p->port_id = port->pofIndex;
    memcpy(p->hw_addr, port->hwaddr, POF_ETH_ALEN);
	strncpy((char *)p->name, port->name, PORT_NAME_LEN);
    p->of_enable = port->of_enable;
    p->state = port->pofState;

	/* Fill the port's other infomation. */
	p->config = 0;
	p->curr = POFPF_10MB_HD | POFPF_10MB_FD;
	p->device_id = g_poflr_dev_id;
	p->supported = 0xffffffff;
	p->advertised = POFPF_10MB_FD | POFPF_100MB_FD;
	p->peer = POFPF_10MB_FD | POFPF_100MB_FD;
    return;
}

/* Report to the Controller about the port information. */
uint32_t 
poflr_port_report(uint8_t reason, const struct portInfo *port)
{
    pof_port_status port_status = {0};

	port_status.reason = reason;
    port_report_fill_msg(&port_status.desc, port);
	pof_HtoN_transfer_port_status(&port_status);

	if(POF_OK != pofec_reply_msg(POFT_PORT_STATUS, g_upward_xid, sizeof(pof_port_status), (uint8_t *)&port_status)){
		POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
	}

    return POF_OK;
}

/* Delete all listening task. */
uint32_t
poflr_ports_task_delete(struct pof_local_resource *lr)
{
    uint32_t ret;
    struct portInfo *port, *next;
    HMAP_NODES_IN_STRUCT_TRAVERSE(port, next, pofIndexNode, lr->portPofIndexMap){
        if(port->taskID != POF_INVALID_TASKID){
            ret = pofbf_task_delete(&port->taskID);
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
        }
    }
    return POF_OK;
}
