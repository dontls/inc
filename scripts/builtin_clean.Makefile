# ==========================================================================
# Cleaning up
# ==========================================================================

PHONY := __clean
__clean:

# Figure out what we need to build from the various variables
# ==========================================================================

obj-y :=
subdir-y := 
target-y :=

include Makefile

__subdir-y	:= $(patsubst %/,%,$(filter %/, $(obj-y)))
subdir-y	+= $(__subdir-y)

objs := $(filter-out %/,$(obj-y))
objs += built-in.o
objs += $(addsuffix .o, $(target-y))
target-y += $(addprefix $(TOPDIR)/, $(target-y))

# ==========================================================================

PHONY += $(subdir-y) 

__clean: $(subdir-y) 
	$(Q)@rm -f $(objs)
	$(Q)@rm -f $(target-y)

# Descending
# ---------------------------------------------------------------------------
$(subdir-y):
	$(Q)@make -C $@ -f $(TOPDIR)/scripts/builtin_clean.Makefile


# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable se we can use it in if_changed and friends.

.PHONY: $(PHONY)
