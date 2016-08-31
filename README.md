POFSwitch
=============

www.poforwarding.org

Please send inquiries/comments/reports to:
	yujingzhou@huawei.com


## 1	POFSwitch Overview
-------------------------------------------------------------------------------------

POFSwitch is a Linux C-based, BSD-licensed, open source OpenFlow software Switch.

	POFSwitch is based on but not limited to OpenFlow Protocol 1.3. Specifically, it
	supports Protocol Oblivious Forwarding so that the packets with new type can be
	forwarded by new user-defined forwarding processes.

	POFSwitch works with POFController that supports the enhanced OpenFlow Protocol
	supporting Protocol Oblivious Forwarding. It provides a Linux C-based platform to
	receive, forward, process, and send the packets with the new types. The flow tables,
	flow entries, and forwarding processes can be created, modified, and deleted by
	the POFController easily.

	POFSwitch is licensed under BSD license. It is easy to run and re-development.
	Users can modify the code, implement new features, and redistribute the code under
	the BSD license.

	POFSwitch is developed and maintained by a group of developers from IP Research
	Department of Huawei Technologies Co., Ltd. Any suggestion, code contribution, or
	bug reports can be directed to [yujingzhou@huawei.com].


### 1.1	Authors and Contributors

Yu, Jingzhou		[yujingzhou@huawei.com]
Wang, Xiaozhong	[rossel.wangxiaozhong@huawei.com]

### 1.2	Acknowledgements

Thanks Song Jian, Zheng Yuanming, Wang Zhen, Chai Zhi, Haoyu Song, Cao Wei and Gong Jun
for the suggestions and the helps on debugging.



## 2	Download POFSwitch
-------------------------------------------------------------------------------------
You can download the POFSwitch source code from the website:
		http://www.poforwarding.org



## 3	Install POFSwitch
-------------------------------------------------------------------------------------

POFSwitch-1.0 supports Linux systems such as Ubuntu and Redhat. We have tested these
instructions with Ubuntu 12.04 with kernel 3.2.0, RHEL 5.5 with kernel 2.6.18, and
RHEL 6.3 with kernel 3.4.10. POFSwitch should be built manually with "./configure" and
"make", as a standard GNU software. Detailed install information are as follows.


### 3.1	Install Prerequisite Tools

GNU make program.

	Open a Terminal/Console, typing ¡°make -version¡± to check whether GNU make is
	already installed. If it is not installed, download and install GNU make program
	via anonymous ftp:

		ftp://ftp.gun.org/gnu/make

	As of this writing, we generally test with version 3.81.

ANSI C compiler

	An ANSI C compiler to build POFSwitch will be needed. The configure script will
	abort if your compiler is not ANSI compliant. If this happens, use the GNU Compiler
	Collection. Download and install the GCC via anonymous ftp:

		ftp://ftp.gnu.org/pub/gnu/gcc.

	As of this writing, we generally test with GCC 4.6.


### 3.2	Install POFSwitch

#### 3.2.1	Installation Steps

First of all, unpack the software tar file:

`% tar zxvf pofswitch-1.0.tar.gz`

To build POFSwitch, run the configure script in the top source directory:

```bash
% cd pofswitch-1.0
% ./configure
```

The configure script will determine your system attributes and generate an appropriate
Makefile from Makefile.in. Then run make in the top source directory:

`% make`

If everything goes well, you can become root by running ""su"" or another program. Then
install the executables into the running system with the default installation path
"/usr/local/bin":

`% make install`

After installation, the configuration file ¡°pofswitch_config.conf¡± will be copied to
"/etc/pofswitch/" directory. The detail of configuration file is described in section 4.2.
Clean up the useless intermediate files and the executable files in the source directory:

`% make clean`

So far, the installation is done.

#### 3.2.2	Optional Features and Functions
There are some optional features and functions you can change by using the configure
arguments while running the configure script.

By default, "make install" will install all of the executable files in "/usr/local/bin".
You can specify an installation prefix other than "/usr/local" using "--prefix":
	
`% ./configure --prefix=PREFIX`

By default, the debug log will not be printed. If you want to output the debug log,
you should enable the debug function:
	
`% ./configure --enable-debug`

By default, POFSwitch print the output with the default color (e.g. white or black).
If you want to print the output with multicolor, you should enable the color function:
	
`% ./configure --enable-color`

By default, the echo log will not be printed. If you want to output the echo log, you
should enable the echo log function. Notice, the echo log function will not work if
the debug function is turned off:
	
`% ./configure --enable-echolog`

POFSwitch have two main tasks, connection and communication with POFController, and
datapath to forward the packet. The datapath task need root privilege, and it will not
start if POFSwitch runs without ¡°sudo¡±. You should turn off the datapath function,
which is enable by default, if you are not a root user, otherwise an ERROR will be
occurred during the POFSwitch running:
	
`% ./configure --disable-datapath`
	
Without the datapath function, you can also use the POFSwitch to connect and communicate
with POFController.

By default, the user command function is enabled. The Detail of this function is
described in following chapter. You can turn off the user command function if you want:
	
`% ./configure --disable-usrcmd`
	
The behavior of control task and datapath task will not be affected whether the user
command function is enabled or not.

Other optional parameters are introduced in "help". You can check out "help":
	
`% ./configure --help`

### 3.3	Installation Files
```
./
	aclocal.m4
	AUTHORS
	ChangeLog
	configure
	configure.in
	depcomp
	install-sh
	Makefile.am
	Makefile.in
	Missing
	NEWS
	pofswitch_config.conf
./common/
	automake.mk
	pof_basefunc.c
	pof_byte_transfer.c
	pof_command.c
	pof_log_print.c
./datapath/
	automake.mk
	pof_action.c
	pof_datapath.c
	pof_instruction.c
	pof_lookup.c
./include/
	automake.mk
	pof_byte_transfer.h
	pof_command.h
	pof_conn.h
	pof_datapath.h
	pof_global.h
	pof_local_resource.h
	pof_log_print.h
	pof_type.h
./local_resource/
	automake.mk
	pof_counter.c
	pof_flow_table.c
	pof_group.c
	pof_local_resource.c
	pof_meter.c
	pof_port.c
./switch_control/
	automake.mk
	pof_config.c
	pof_encap.c
	pof_parse.c
	pof_switch_control.c
```


## 4	Run POFSwitch
-------------------------------------------------------------------------------------

### 4.1	Start POFSwitch

After installation, POFSwitch can be started easily:

`% pofswitch`

If you want to use the datapath function and you are a root user, you should start
POFSwitch with "sudo":

`% sudo pofswitch`


### 4.2	Configure POFSwitch

Some features of POFSwitch is configurable for users, such as the connection information
to POFController, and the size of the local resource. There are two ways to configure
the POFSwitch: by file, and by command parameters.

#### 4.2.1	Configurable Features

- Controller_IP: The IP address of POFController, e.g. "192.168.1.1".
- Controller_port: The port number of the TCP connection to POFController, e.g. ""6633".
- MM_table_number: The maximum number of flow tables with MM type in local resource.
- LPM_table_number: The maximum number of flow tables with LPM type in local resource.
- EM_table_number: The maximum number of flow tables with EM type in local resource.
- DT_table_number: The maximum number of flow tables with DT type in local resource.
- Flow_table_size: The maximum size of flow tables.
- Flow_table_key_length: The maximum length of flow table key.
- Meter_number: The maximum number of meters in local resource.
- Counter_number: The maximum number of counters in local resource.
- Group_number: The maximum number of groups in local resource.
- Device_port_number_max: the maximum number of local physical net port.

#### 4.2.2	Configure by File

You can configure all of the features, showed above, of POFSwitch by a file. The
configuration file's name should be "pofswitch_config.conf".

You can specify the file and path by using command parameter with "-f" flag:

`% pofswitch --f FILE`

The FILE should have a absolute path, such as "/home/tom/config/pofswitch_config.conf".

If you run POFSwitch without "-f" optional flag or the file you specify cannot open,
the software will search the configuration file "pofswitch_config.conf" in the
current directory and "/etc/pofswitch/" directory.
The "/etc/pofswitch/pofswitch_config.conf" has been installed during the installation,
which described in section 3.2.1.

If the software cannot find any configuration file, it will load the default
features which may do not fit your PC. An ERROR, such as failure to connect to
POFController, may occur.

#### 4.2.3	Configure by Command Parameter

You also can configure POFSwitch by command parameters when start the software. But
only Controller_IP and Controller_port can be set by this way.

Set the Controller_IP with "-i" flag:

`% pofswitch --i IPADDR`

IPADDR here should be instead of Controller IP such as "192.168.1.1".

Set the Controller_port with "-p" flag:

`% pofswitch --p PORT`

PORT here should be instead of port number of TCP connection to POFController
such as "6633".

If a configuration conflict between command parameter and configuration file happens,
the command parameter configuration method has a higher priority.


### 4.3	Work with a POFController

POFSwitch works with a POFController automatically. It executes the commands in the
messages received from POFController through an OpenFlow channel. POFSwitch-1.0 does
not support user to set local resource of POFSwitch directly. So you should add, modify
or delete flow tables and flow entries in local resource of POFSwitch using POFController,
instead of operating POFSwitch directly. You can find the instruction of POFController
here:

		http://www.poforwarding.org

As POFSwitch works automatically all the time, you can use it easily. The following
sections 4.3.1 and 4.3.2 describe how POFSwitch works in detail, which is not necessary
for beginner to read.

#### 4.3.1	OpenFlow Channel

After startup, POFSwitch automatically creates OpenFlow connection to POFController
through TCP channel with specified IP address and port number configured in section 4.2.
POFSwitch has eight work states: CHANNAL_INVALID, CHANNEL_CONNECTING, CHANNEL_CONNECTED,
HELLO, REQUEST_FEATURE, SET_CONFIG, REQUEST_GET_CONFIG, CHANNEL_RUN. POFSwitch runs
in these states in turn automatically. If there is no ERROR occurs,	the OpenFlow channel
with POFController has been created successfully after the REQUEST_GET_CONFIG state.
Then POFSwitch always works in the CHANNEL_RUN state. This OpenFlow channel can be used
for the communication between POFSwitch and POFController.

#### 4.3.2	Forward Packet

POFController sends command messages to POFSwitch through the OpenFlow channel to add,
modify or delete the flow tables, flow entries and so on. POFSwtich executes these
commands. POFSwitch keep listening all of the local network physical ports all the time.
As soon as receiving a new packet from a port, POFSwitch moves the packet into the first
flow table to find the flow entry matched with the packet. All of the instructions and
actions belong to this flow entry will be executed, such as WRITE_METADATA, ADD_TAG,
DROP, OUTPUT, GOTO_NEXT_TABLE and so on. Move the packet into the next flow table, if
GOTO_NEXT_TABLE instruction has been executed. Find the matched flow entry and execute
all of the instructions and actions. Then move the packet into the next flow table.
Repeat these operations in turn until the process of the packet is over. Process of
the packet will be over in the following cases:

	One of the OUTPUT, DROP and TOCP actions has been executed.
	There is not any flow entry in the flow table can match the packet in the lookup process.
	Any ERROR occurs.
	There is no instruction and action needs to be executed.

In POFSwitch-1.0, no matter what the table type is, POFSwitch returns the first matched
flow entry in the lookup process. Other table lookup techniques will be included in the
future version.

The performance of the packet forwarding depends on the flow tables in local resource
which is configured by POFController. Users can decide how to forward the packet using
POFController. You can find the further instruction of Protocol Oblivious Forwarding here:

		http://www.poforwarding.org


### 4.4	User command

You can inquire about the local resource by inputting user command while POFSwitch is
running. The supported user commands list:

- help: Print the supported user commands list.
- quit: Quit the POFSwitch.
- switch_config: Print the switch¡¯s configuration.
- switch_feature: Print the switch¡¯s feature.
- table_resource: Print the table resource.
- ports: Print the information of local network physical ports.
- tables: Print the content of local flow tables.
- groups: Print the content of local group tables.
- meters: Print the content of local meter tables.
- counters: Print the content of local counter tables.

User commands are just used for inquiring about the local resource, not for modifying
flow tables.



# 5	Frequently Asked Questions
-------------------------------------------------------------------------------------
Q:	What platforms can use POFSwitch?

A:	POFSwitch-1.0 supports Linux systems such as Ubuntu and Redhat. We have tested these
	instructions with Ubuntu 12.04 with kernel 3.2.0, RHEL 5.5 with kernel 2.6.18, and
	RHEL 6.3 with kernel 3.4.10.


Q:	Does POFSwitch have to work with POFController?

A:	Yes. As the first step, POFSwitch tries to connect to POFController at beginning.
	It doesn't work until the connection is successful. The local resource of POFSwitch
	can be configured only by POFController, so you cannot modify the local resource
	without POFController.

Q:	What is POFSwitch's performance in case that the connection to POFController is
	break?
	
A:	The connection is break as soon as POFController is turned off or the communication
	is interrupted by any unexpected reason. In this situation, POFSwitch keeps trying
	to reconnect to POFController every 2 seconds until the connection is successful or
	shut down the POFSwitch.


Q:	What is POFSwitch's performance in case that any error occurs?

A:	There are two different situations. If the error occurs during the forwarding process
	of one packet, POFSwitch drops the packet and keeps running. If the error occurs in
	OpenFlow control platform, or base function, shut down POFSwitch. No matter which one
	happens, the error information, including error code, error type and error string,
	will be printed in the screen and be send upward to POFController.


Q:  Does POFSwitch forward all packets?

A:  No. Only the packet which Dmac equal to the mac address of POFSwitch network port can
    be received by POFSwitch. At the same time, POFSwitch only forwards the packets received
	by the OpenFlow enabled network port. By default, the OpenFlow functions of all ports
	are disabled at beginning, so you need to enable the OpenFlow function of the data port
	before forwarding.
