# ==========================================================================
# Building
# ==========================================================================

PHONY := all
all:

obj-y :=
subdir-y :=
target-y :=

include Makefile

__subdir-y := $(patsubst %/,%,$(filter %/, $(obj-y)))
subdir-y += $(__subdir-y)

subdir-objs := $(foreach f, $(subdir-y), $(f)/built-in.o) 

objs := $(filter-out %/,$(obj-y))
target-objs := $(addsuffix .o, $(target-y))

PHONY += $(subdir-y)
 
-include $(objs:.o=.d)
-include $(target-objs:.o=.d)


all-objs := $(subdir-y) 

ifdef_any_of = $(filter-out undefined, $(foreach v,$(1),$(origin $(v))))

ifdef objs
all-objs += built-in.o
endif

all-objs += $(target-y)

all: $(all-objs) 
$(subdir-y):
	$(Q)@make -C $@ -f $(TOPDIR)/scripts/builtin_build.Makefile
  
built-in.o: $(objs) $(subdir-objs)
	$(Q)$(LD) -r -o $@ $^

ifdef objs
$(target-y): $(target-objs) built-in.o
	$(Q)$(CXX) built-in.o $(addsuffix .o, $@) -o $@ $(LDFLAGS) 
#	$(Q)@install $@ $(TOPDIR)/
else
$(target-y): $(target-objs)
	$(Q)$(CXX) $(addsuffix .o, $@) -o $@ $(LDFLAGS)
#	$(Q)@install $@ $(TOPDIR)/ 
endif

%.d: %.cpp
	$(Q)$(CXX) $(CXXFLAGS) -MM $< -MT $(basename $@).o -o $(basename $@).d

%.o: %.cpp
	$(ECHO) "CXX " $@;
	$(Q)$(CXX) $(CXXFLAGS) -c $< -o $@

%.d: %.c
	$(Q)$(CC) $(CFLAGS) -MM $< -MT $(basename $@).o -o $(basename $@).d

%.o: %.c
	$(ECHO) "CC " $@;
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

.PHONY: $(PHONY)
