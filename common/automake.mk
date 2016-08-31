COMMON_FOLDER = common
pofswitch_SOURCES += $(COMMON_FOLDER)/pof_basefunc.c \
					 $(COMMON_FOLDER)/pof_byte_transfer.c \
					 $(COMMON_FOLDER)/pof_command.c \
					 $(COMMON_FOLDER)/pof_hmap.c \
					 $(COMMON_FOLDER)/pof_tree.c \
					 $(COMMON_FOLDER)/pof_list.c \
					 $(COMMON_FOLDER)/pof_memory.c \
					 $(COMMON_FOLDER)/pof_log_print.c
pofsctrl_SOURCES +=  $(COMMON_FOLDER)/pof_log_print.c \
					 $(COMMON_FOLDER)/pof_byte_transfer.c
