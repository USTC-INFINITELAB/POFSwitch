SWITCH_CONTROL_FOLDER = switch_control
pofswitch_SOURCES += $(SWITCH_CONTROL_FOLDER)/pof_config.c \
					 $(SWITCH_CONTROL_FOLDER)/pof_encap.c \
					 $(SWITCH_CONTROL_FOLDER)/pof_parse.c \
					 $(SWITCH_CONTROL_FOLDER)/pof_switch_listen.c \
					 $(SWITCH_CONTROL_FOLDER)/pof_switch.c
pofsctrl_SOURCES  += $(SWITCH_CONTROL_FOLDER)/pof_sctrl.c
