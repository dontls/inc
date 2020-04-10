
TOPDIR := $(CURDIR)

LDFLAGS += -pthread

# 这里要使用绝对路径
INCLUDES += -I $(CURDIR)/include 

CXXFLAGS += $(INCLUDES)

DIR := example/

include $(TOPDIR)/scripts/top.mk