## Almost Universal Makefile
## Author: Przemysław Pawełczyk

BINS := \
 metastore

LIBS := \

### Directories
PROJ_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
SRCS_DIR = $(PROJ_DIR)src/
INCS_DIR = $(PROJ_DIR)include/
CFGS_DIR = $(PROJ_DIR)etc/
SHRS_DIR = $(PROJ_DIR)share/
OBJS_DIR = $(PROJ_DIR)obj/
BINS_DIR = $(PROJ_DIR)bin/
LIBS_DIR = $(PROJ_DIR)lib/
DOCS_DIR = $(PROJ_DIR)
MANS_DIR = $(PROJ_DIR)

###
METASTORE_VER  := $(shell "$(PROJ_DIR)"/version.sh)
UNAME_S        := $(shell uname -s)

###

DOCS := \
 AUTHORS \
 FILEFORMAT \
 LICENSE.GPLv2 \
 NEWS \
 README \
 metastore.txt \
 examples \

metastore_COMP := CC

metastore_SRCS := \
 metaentry.c \
 metastore.c \
 utils.c \

metastore_DLIBS := \

metastore_MANS := \
 man1/metastore.1 \

ifeq ($(findstring BSD,$(UNAME_S)),)
ifneq (DragonFly,$(UNAME_S))
ifneq (Darwin,$(UNAME_S))
metastore_DLIBS += -lbsd
endif
endif
endif

ifneq (,$(NO_XATTR))
ifneq (0,$(NO_XATTR))
ADDITIONAL_FLAGS += -DNO_XATTR
endif
endif

PVER := $(PROJ_DIR)Makefile.ver

SDEP := Makefile.dep

-include $(PVER)

# when building from project tree, then always use the same output layout
# otherwise use current working directory
ifneq (,$(findstring ^$(realpath $(PROJ_DIR)),^$(realpath ./)))
	SDEP := $(PROJ_DIR)$(SDEP)
else
	OBJS_DIR := ./obj/
	BINS_DIR := ./bin/
	LIBS_DIR := ./lib/
endif


# library name prefix
LIB_PRE := lib
# Dynamic Shared Object extension
DSO_EXT := .so
# Statically-Linked Library extension
SLL_EXT := .a


### Default target
all:   libs bins

bins:  $(BINS)
libs:  $(LIBS)

### Flags
MUSTHAVE_FLAGS    := \
 -DMETASTORE_VER="\"$(METASTORE_VER)\"" \
 -D_FILE_OFFSET_BITS=64 \
 -Wall -Wextra -pedantic \
 -g \
 $(ADDITIONAL_FLAGS) \

MUSTHAVE_CFLAGS   := -std=c99
MUSTHAVE_CXXFLAGS := -std=c++11
OPTIONAL_FLAGS    := -O2

### Install paths
PREFIX := /usr/local
EXECPREFIX := $(PREFIX)
BINDIR := $(EXECPREFIX)/bin
LIBDIR := $(EXECPREFIX)/lib
INCLUDEDIR := $(PREFIX)/include
SYSCONFDIR := $(PREFIX)/etc
DATAROOTDIR := $(PREFIX)/share
DATADIR := $(DATAROOTDIR)
DOCDIR := ${DATAROOTDIR}/doc/metastore
MANDIR := ${DATAROOTDIR}/man

### Phony targets
.PHONY: \
 all bins libs \
 strip strip-bins strips-libs \
 install install-bins install-libs install-includes \
 install-configs install-docs install-mans install-shares \
 uninstall uninstall-bins uninstall-libs uninstall-includes \
 clean distclean \
 dep \
 printvars \

### Debug
printvars:
	@$(foreach V, $(sort $(.VARIABLES)), \
	   $(if $(filter-out environment% default automatic, $(origin $V)), \
	        $(warning $V = $(value $V)) \
	        $(if $(findstring ^$(value $V)%,^$($V)%),,$(warning $V == $($V))) \
	   ) \
	)

### Install tools
INSTALL := install
INSTALL_EXEC := $(INSTALL) -m 0755
COPY_NODEREF := cp -P

### Build tools
DEFCC  := gcc
DEFCXX := g++
ifeq ($(origin CC),default)
CC  := $(DEFCC)
endif
ifeq ($(origin CXX),default)
CXX := $(DEFCXX)
endif
ifneq ($(origin CC),environment)
CC  := $(CROSS_COMPILE)$(CC)
endif
ifneq ($(origin CXX),environment)
CXX := $(CROSS_COMPILE)$(CXX)
endif
ifneq ($(origin CCLD),environment)
CCLD  := $(CC)
endif
ifneq ($(origin CXXLD),environment)
CXXLD := $(CXX)
endif
ifneq ($(origin STRIP),environment)
STRIP := $(CROSS_COMPILE)strip
endif
CC_PARAMS  = $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH)
CXX_PARAMS = $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH)

### Final flags
ifneq ($(origin CFLAGS),environment)
CFLAGS   = $(OPTIONAL_FLAGS)
endif
ifneq ($(origin CXXFLAGS),environment)
CXXFLAGS = $(OPTIONAL_FLAGS)
endif
override CFLAGS   += $(MUSTHAVE_FLAGS) $(MUSTHAVE_CFLAGS) \
                     $(if $(INCS_DIR),-I$(INCS_DIR),) -I$(SRCS_DIR)
override CXXFLAGS += $(MUSTHAVE_FLAGS) $(MUSTHAVE_CXXFLAGS) \
	                 $(if $(INCS_DIR),-I$(INCS_DIR),) -I$(SRCS_DIR)

vpath %.h   $(SRCS_DIR) $(INCS_DIR)
vpath %.c   $(SRCS_DIR)
vpath %.cpp $(SRCS_DIR)

### Support for hiding command-line
V ?= 0
HIDE_0 := @
HIDE_1 :=
HIDE := $(HIDE_$(V))


### Rules for directories

$(sort $(OBJS_DIR) $(BINS_DIR) $(LIBS_DIR)):
	@mkdir -p $@

### Templated rules

define BIN_template

$(1)_OBJS := $$(addsuffix .o,$$(basename $$($(1)_SRCS)))

$$(BINS_DIR)$(1): \
 $$(addsuffix $$(DSO_EXT),$$(addprefix $$(LIBS_DIR)$$(LIB_PRE),$$($(1)_DEPS))) \
 $$(addsuffix $$(SLL_EXT),$$(addprefix $$(LIBS_DIR)$$(LIB_PRE),$$($(1)_DEPS))) \
 $$(addprefix $$(OBJS_DIR),$$($(1)_OBJS)) $$(SDEP) | $$(BINS_DIR)
ifeq ($$($(1)_COMP),CC)
	@echo "        CCLD    $$@"
else
	@echo "        CXXLD   $$@"
endif
	$$(HIDE)$$($$($(1)_COMP)LD) $$(LDFLAGS) $$(TARGET_ARCH) \
	  -o $$@ $$(filter %.o,$$^) \
	  -Wl,-Bstatic $$($(1)_SLIBS) -Wl,-Bdynamic $$($(1)_DLIBS)

$(1): $$(BINS_DIR)$(1)
.PHONY: $(1)

SRCS += $$($(1)_SRCS)
OBJS += $$($(1)_OBJS)

CFGS += $$($(1)_CFGS)
MANS += $$($(1)_MANS)
SHRS += $$($(1)_SHRS)

endef

define LIB_template

$(1)_OBJS := $$(addsuffix .o,$$(basename $$($(1)_SRCS)))

$(1)_SOLINK := $$(LIB_PRE)$(1)$$(DSO_EXT)
$(1)_SONAME := $$(LIB_PRE)$(1)$$(DSO_EXT).$$($(1)_MAJV)
$(1)_SOFILE := $$(LIB_PRE)$(1)$$(DSO_EXT).$$($(1)_MAJV)$$($(1)_MINV)
$(1)_ARFILE := $$(LIB_PRE)$(1)$$(SLL_EXT)

$$(LIBS_DIR)$$($(1)_SOLINK): $$(LIBS_DIR)$$($(1)_SONAME)
	@echo "        LN      $$@ -> $$($(1)_SONAME)"
	$$(HIDE)(cd "$(LIBS_DIR)" && ln -sf $$($(1)_SONAME) $$($(1)_SOLINK))

ifneq ($$($(1)_SONAME),$$($(1)_SOFILE))
$$(LIBS_DIR)$$($(1)_SONAME): $$(LIBS_DIR)$$($(1)_SOFILE)
	@echo "        LN      $$@ -> $$($(1)_SOFILE)"
	$$(HIDE)(cd "$$(LIBS_DIR)" && ln -sf $$($(1)_SOFILE) $$($(1)_SONAME))
endif

$$(LIBS_DIR)$$($(1)_SOFILE): \
 $$(addsuffix $$(DSO_EXT),$$(addprefix $$(LIBS_DIR)$$(LIB_PRE),$$($(1)_DEPS))) \
 $$(addsuffix $$(SLL_EXT),$$(addprefix $$(LIBS_DIR)$$(LIB_PRE),$$($(1)_DEPS))) \
 $$(addprefix $$(OBJS_DIR),$$($(1)_OBJS)) $$(SDEP) | $$(LIBS_DIR)
ifeq ($$($(1)_COMP),CC)
	@echo "        CCLD    $$@"
else
	@echo "        CXXLD   $$@"
endif
	$$(HIDE)$$($$($(1)_COMP)LD) $$(LDFLAGS) $$(TARGET_ARCH) \
	  -shared -Wl,-soname,$$($(1)_SONAME) \
	  -o $$@ $$(filter %.o,$$^) \
	  -Wl,-Bstatic $$($(1)_SLIBS) -Wl,-Bdynamic $$($(1)_DLIBS)

$$(LIBS_DIR)$$($(1)_ARFILE): \
 $$(addprefix $$(OBJS_DIR),$$($(1)_OBJS)) $$(SDEP) | $$(LIBS_DIR)
	@echo "        AR      $$@"
	$$(HIDE)$$(AR) r $$@ $$(filter %.o,$$^)

$(1): \
 $$(LIBS_DIR)$$($(1)_SOFILE) \
 $$(LIBS_DIR)$$($(1)_SOLINK) \
 $$(LIBS_DIR)$$($(1)_ARFILE)
.PHONY: $(1)

SRCS += $$($(1)_SRCS)
OBJS += $$($(1)_OBJS)

INCS += $$($(1)_INCS)
CFGS += $$($(1)_CFGS)
MANS += $$($(1)_MANS)
SHRS += $$($(1)_SHRS)

LIBS_FILES += $$($(1)_SOFILE) $$($(1)_ARFILE)
LIBS_ALL_FILES += $$($(1)_SOLINK) $$($(1)_SONAME) $$($(1)_SOFILE) \
                  $$($(1)_ARFILE)

endef

$(foreach BIN,$(BINS),$(eval $(call BIN_template,$(BIN))))

$(foreach LIB,$(LIBS),$(eval $(call LIB_template,$(LIB))))


### Rules for normal targets

$(OBJS_DIR)%.o: %.c
	@echo "        CC      $@"
	$(HIDE)mkdir -p $(dir $@)
	$(HIDE)$(CC) $(CC_PARAMS) -c -o $@ $<

$(OBJS_DIR)%.o: %.cpp
	@echo "        CXX     $@"
	$(HIDE)mkdir -p $(dir $@)
	$(HIDE)$(CXX) $(CXX_PARAMS) -c -o $@ $<

$(SDEP): SRCS_PATH := $(SRCS_DIR)
$(SDEP): SRCS_DIR := ./
$(SDEP): PROJ_DIR := ../
$(SDEP): $(SRCS) $(PROJ_DIR)Makefile
	@echo "        DEPS";
	$(HIDE)echo "# This file is automatically (re)generated by make." >$(SDEP)
	$(HIDE)echo >>$(SDEP)
	$(HIDE)(cd "$(SRCS_PATH)" && for FILE in $(SRCS); do \
		if [ "$${FILE##*.}" = "c" ]; then \
			$(CC) $(CFLAGS) -MT "\$$(OBJS_DIR)$${FILE%.*}.o" -MM "$$FILE"; \
		else \
			$(CXX) $(CXXFLAGS) -MT "\$$(OBJS_DIR)$${FILE%.*}.o" -MM "$$FILE"; \
		fi \
	done) >>$(SDEP)

$(DOCS_DIR)%.txt: $(MANS_DIR)/man1/%.1
	@echo "        GROFF   $@"
	$(HIDE)groff -mandoc -Kutf8 -Tutf8 $^ | col -bx >$@


### Rules for phony targets

clean:
	@echo "        CLEAN"
	$(HIDE)$(RM) $(addprefix $(BINS_DIR),$(BINS)) \
	             $(addprefix $(LIBS_DIR),$(LIBS_ALL_FILES)) \
	             $(addprefix $(OBJS_DIR),$(OBJS)) \
	             $(SDEP)

distclean: clean
	$(HIDE)(find \
		$(OBJS_DIR) \
		$(LIBS_DIR) \
		$(BINS_DIR) \
		-type d -exec rmdir --parents {} \; \
		2>/dev/null ; exit 0)

dep: $(SDEP)

strip: strip-bins strip-libs

strip-bins: $(addprefix $(BINS_DIR),$(BINS))
ifneq ($(strip $(BINS)),)
	@echo "        STRIP   $^"
	$(HIDE)$(STRIP) $^
endif

strip-libs: $(addprefix $(LIBS_DIR),$(LIBS_FILES))
ifneq ($(strip $(LIBS)),)
	@echo "        STRIP   $^"
	$(HIDE)$(STRIP) $^
endif

install: install-bins install-libs install-includes \
         install-configs install-docs install-mans install-shares \

install-bins: $(addprefix $(BINS_DIR),$(BINS))
ifneq ($(strip $(BINS)),)
	@echo "        INSTALL $^"
	$(HIDE)$(INSTALL) -d $(DESTDIR)$(BINDIR)
	$(HIDE)$(INSTALL_EXEC) \
		$(addprefix $(BINS_DIR),$(BINS)) \
		$(DESTDIR)$(BINDIR)/
endif

install-libs: $(addprefix $(LIBS_DIR),$(LIBS_ALL_FILES))
ifneq ($(strip $(LIBS)),)
	@echo "        INSTALL $^"
	$(HIDE)$(INSTALL) -d $(DESTDIR)$(LIBDIR)
	$(HIDE)$(COPY_NODEREF) \
		$(addprefix $(LIBS_DIR),$(LIBS_ALL_FILES)) \
		$(DESTDIR)$(LIBDIR)/
endif

install-includes: $(addprefix $(INCS_DIR),$(INCS))
ifneq ($(strip $(INCS)),)
	@echo "        INSTALL $^"
	$(HIDE)$(INSTALL) -d $(DESTDIR)$(INCLUDEDIR)
	$(HIDE)( cd $(INCS_DIR) && $(COPY_NODEREF) --parents \
		$(INCS) \
		$(DESTDIR)$(INCLUDEDIR)/ \
	)
endif

install-configs: $(addprefix $(CFGS_DIR),$(CFGS))
ifneq ($(strip $(CFGS)),)
	@echo "        INSTALL $^"
	$(HIDE)$(INSTALL) -d $(DESTDIR)$(SYSCONFDIR)
	$(HIDE)( cd $(CFGS_DIR) && $(COPY_NODEREF) --parents \
		$(CFGS) \
		$(DESTDIR)$(SYSCONFDIR)/ \
	)
endif

install-docs: $(addprefix $(DOCS_DIR),$(DOCS))
ifneq ($(strip $(DOCS)),)
	@echo "        INSTALL $^"
	$(HIDE)$(INSTALL) -d $(DESTDIR)$(DOCDIR)
	$(HIDE)( cd $(DOCS_DIR) && $(COPY_NODEREF) --parents -r \
		$(DOCS) \
		$(DESTDIR)$(DOCDIR)/ \
	)
endif

install-mans: $(addprefix $(MANS_DIR),$(MANS))
ifneq ($(strip $(MANS)),)
	@echo "        INSTALL $^"
	$(HIDE)$(INSTALL) -d $(DESTDIR)$(MANDIR)
	$(HIDE)( cd $(MANS_DIR) && $(COPY_NODEREF) --parents \
		$(MANS) \
		$(DESTDIR)$(MANDIR)/ \
	)
endif

install-shares: $(addprefix $(SHRS_DIR),$(SHRS))
ifneq ($(strip $(SHRS)),)
	@echo "        INSTALL $^"
	$(HIDE)$(INSTALL) -d $(DESTDIR)$(DATADIR)
	$(HIDE)( cd $(SHRS_DIR) && $(COPY_NODEREF) --parents \
		$(SHRS) \
		$(DESTDIR)$(DATADIR)/ \
	)
endif

uninstall: uninstall-bins uninstall-libs uninstall-includes \
           uninstall-docs uninstall-mans uninstall-shares \

uninstall-bins:
ifneq ($(strip $(BINS)),)
	@echo "        UNINST  $(addprefix $(DESTDIR)$(BINDIR)/,$(BINS))"
	$(HIDE)$(RM) $(addprefix $(DESTDIR)$(BINDIR)/,$(BINS))
endif

uninstall-libs:
ifneq ($(strip $(LIBS)),)
	@echo "        UNINST  $(addprefix $(DESTDIR)$(LIBDIR)/,$(LIBS_ALL_FILES))"
	$(HIDE)$(RM) $(addprefix $(DESTDIR)$(LIBDIR)/,$(LIBS_ALL_FILES))
endif

uninstall-includes:
ifneq ($(strip $(INCS)),)
	@echo "        UNINST  $(addprefix $(DESTDIR)$(INCLUDEDIR)/,$(INCS))"
	$(HIDE)$(RM) $(addprefix $(DESTDIR)$(INCLUDEDIR)/,$(INCS))
endif

uninstall-docs:
ifneq ($(strip $(DOCS)),)
	@echo "        UNINST  $(addprefix $(DESTDIR)$(DOCDIR)/,$(DOCS))"
	$(HIDE)$(RM) -r $(addprefix $(DESTDIR)$(DOCDIR)/,$(DOCS))
endif

uninstall-mans:
ifneq ($(strip $(MANS)),)
	@echo "        UNINST  $(addprefix $(DESTDIR)$(MANDIR)/,$(MANS))"
	$(HIDE)$(RM) $(addprefix $(DESTDIR)$(MANDIR)/,$(MANS))
endif

uninstall-shares:
ifneq ($(strip $(SHRS)),)
	@echo "        UNINST  $(addprefix $(DESTDIR)$(DATADIR)/,$(SHRS))"
	$(HIDE)$(RM) $(addprefix $(DESTDIR)$(DATADIR)/,$(SHRS))
endif


### Dependencies
ifneq ($(MAKECMDGOALS),distclean)
ifneq ($(MAKECMDGOALS),clean)
-include $(SDEP)
endif
endif
