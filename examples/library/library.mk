include ../../../environment.mk

# flow.mk defines all of the host compilationk, link, and execution rules
include $(EXAMPLES_PATH)/flow.mk

DEFINES += -D_XOPEN_SOURCE=500 -D_BSD_SOURCE
INCLUDES += -I$(EXAMPLES_PATH)
CDEFINES   += $(DEFINES)
CXXDEFINES += $(DEFINES)

FLAGS     = -g -Wall
CFLAGS   += -std=c99 $(FLAGS) 
CXXFLAGS += -std=c++11 $(FLAGS)
