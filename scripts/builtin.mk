ifeq ($(V),1)
Q :=
ECHO := @true
else
Q := @
ECHO := @echo
endif
TOPDIR := $(CURDIR)
export Q ECHO TOPDIR
#-------------------------------------------------------------------------------

# Compilation tools
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld
STRIP = $(CROSS_COMPILE)strip
NM = $(CROSS_COMPILE)nm
# export CC CXX AR LD NM STRIP 

#-------------------------------------------------------------------------------
build ?= debug
CFLAGS_OPT ?= -Os 

ifeq ($(build), debug)
CFLAGS_OPT += -Wall -g
else
CFLAGS_OPT += -DNDEBUG 
endif

#-------------------------------------------------------------------------------
# 这里定义通用的编译参数，不同项目在对应Makefile中配置
# c 编译参数
CFLAGS += -O2 -fPIC -Wdeprecated-declarations $(CFLAGS_OPT)
# c++ 编译参数
CXXFLAGS += $(CFLAGS) -std=c++11
# 链接so, 在子层Makefile中配置

export CFLAGS CXXFLAGS LDFLAGS 

# 需要在子层目录中写Makefile, 可以控制编译文件
OBJS := $(patsubst %/, %/built-in.o, $(DIR))

PHONY := all 
all:: $(OBJS) $(APP-build) $(LIB-build)
	$(ECHO) -e "\033[36mDone $^\033[0m"

$(OBJS)::
	$(Q)@make -C $(DIR) -f $(TOPDIR)/scripts/builtin_build.Makefile

$(LIB-build):: 
	$(Q)$(CXX) $(CXXFLAGS) -shared -Wl,-soname,$@ $(LDFLAGS) -o $@ $(OBJS) $>

$(APP-build)::
	$(Q)$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $> $(LDFLAGS) 

# make clean distclean
#
clean-dirs := $(addprefix _clean_, . $(DIR))
PHONY += clean distclean
clean: $(clean-dirs)
	$(Q)@rm -f $(APP-build) $(LIB-build)

PHONY += $(clean-dirs)
$(clean-dirs):
	$(Q)@make -C $(patsubst _clean_%,%,$@) -f $(TOPDIR)/scripts/builtin_clean.Makefile

distclean: clean
	$(Q)@rm -f $(shell find $(CURDIR) -name "*.d")

.PHONY: $(PHONY)
