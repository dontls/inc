
LDFLAGS += -pthread

INCLUDES += -I$(CURDIR)/include

CXXFLAGS += $(INCLUDES)

DIR := example/

include scripts/builtin.mk