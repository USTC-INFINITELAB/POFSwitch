AUTOMAKE_OPTIONS = foreign 
bin_PROGRAMS = pofswitch pofsctrl
pofswitch_SOURCES = 
pofsctrl_SOURCES =
POFCONFIG_FILE = pofswitch_config.conf
INSTALL_DATA_PATH = /etc/pofswitch
LOG_FILE_PATH = /usr/local/var/log
SBIN_PATH = /sbin

INCLUDES = -I $(top_srcdir)/include

EXTRA_DIST = pofswitch_config.conf

install-data-local:
	mkdir -p $(INSTALL_DATA_PATH)
	mkdir -p $(LOG_FILE_PATH)
	cp $(top_srcdir)/$(POFCONFIG_FILE) $(INSTALL_DATA_PATH)
	cp $(top_srcdir)/$(bin_PROGRAMS) $(SBIN_PATH)
uninstall-local:
	rm -rf $(INSTALL_DATA_PATH)
	rm -f $(SBIN_PATH)/$(bin_PROGRAMS) $(LOG_FILE_PATH)/pofswitch.log
clean-local:
	rm -f cscope* tags

include common/automake.mk
include datapath/automake.mk
include local_resource/automake.mk
include switch_control/automake.mk
include include/automake.mk
