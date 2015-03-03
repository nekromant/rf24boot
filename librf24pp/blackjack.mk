# Handle OS Specifics
ifeq ($(OS),Windows_NT)
HOST_OS=Windows
OS_WHICH=where
OS_NULL=shit
OS_ECHO=cecho
else
HOST_OS := $(shell uname -s)
OS_WHICH=which
OS_NULL=/dev/null
OS_ECHO=echo
endif


ifeq ($(HOST_OS),Windows)
tb_blk=\x1b[30m
tb_red=\x1b[31m
tb_grn=\x1b[32m
tb_ylw=\x1b[33m
tb_ylw=\x1b[33m
tb_blu=\x1b[34m
tb_pur=\x1b[35m
tb_cyn=\x1b[36m
tb_wht=\x1b[37m
col_rst=\x1b[0m
ECHO:=cecho
else

OS_ECHO:=echo
ifeq ($(HOST_OS),Linux)
 ECHOC:=echo -e '\e
endif

ifeq ($(HOST_OS),Darwin)
 ECHOC:=echo '\x1B
endif


t_blk=$(shell $(ECHOC)[0;30m')
t_red=$(shell $(ECHOC)[0;31m')
t_grn=$(shell $(ECHOC)[0;32m')
t_ylw=$(shell $(ECHOC)[0;33m')
t_blu=$(shell $(ECHOC)[0;34m')
t_pur=$(shell $(ECHOC)[0;35m')
t_cyn=$(shell $(ECHOC)[0;36m')
t_wht=$(shell $(ECHOC)[0;37m')

tb_blk=$(shell $(ECHOC)[1;30m')
tb_red=$(shell $(ECHOC)[1;31m')
tb_grn=$(shell $(ECHOC)[1;32m')
tb_ylw=$(shell $(ECHOC)[1;33m')
tb_blu=$(shell $(ECHOC)[1;34m')
tb_pur=$(shell $(ECHOC)[1;35m')
tb_cyn=$(shell $(ECHOC)[1;36m')
tb_wht=$(shell $(ECHOC)[1;37m')

tu_blk=$(shell $(ECHOC)[4;30m')
tu_red=$(shell $(ECHOC)[4;31m')
tu_grn=$(shell $(ECHOC)[4;32m')
tu_ylw=$(shell $(ECHOC)[4;33m')
tu_blu=$(shell $(ECHOC)[4;34m')
tu_pur=$(shell $(ECHOC)[4;35m')
tu_cyn=$(shell $(ECHOC)[4;36m')
tu_wht=$(shell $(ECHOC)[4;37m')

bg_blk=$(shell $(ECHOC)[40m')
bg_red=$(shell $(ECHOC)[41m')
bg_grn=$(shell $(ECHOC)[42m')
bg_ylw=$(shell $(ECHOC)[43m')
bg_blu=$(shell $(ECHOC)[44m')
bg_pur=$(shell $(ECHOC)[45m')
bg_cyn=$(shell $(ECHOC)[46m')
bg_wht=$(shell $(ECHOC)[47m')
col_rst=$(shell $(ECHOC)[0m')

endif


ifeq ($(DEBUG),)
SILENT_CONFIG    = @$(OS_ECHO) '  $(tb_cyn)[CONF]$(col_rst)      '$(subst \,/,$(@)) && 
SILENT_DEP       = @$(OS_ECHO) '  $(tb_wht)[DEP]$(col_rst)       '$(subst \,/,$(@)).dep && 
SILENT_NMCPP     = @$(OS_ECHO) '  $(tb_ylw)[NMCPP]$(col_rst)     '$(subst \,/,$(@)) && 
SILENT_ASM       = @$(OS_ECHO) '  $(tb_grn)[ASM]$(col_rst)       '$(subst \,/,$(@)) && 
SILENT_LINKER    = @$(OS_ECHO) '  $(tb_pur)[LINKER]$(col_rst)    '$(subst \,/,$(@)) && 
SILENT_AR        = @$(OS_ECHO) '  $(tb_cyn)[AR]$(col_rst)        '$(subst \,/,$(@)) && 
SILENT_NMDUMP    = @$(OS_ECHO) '  $(tb_blu)[NMDUMP]$(col_rst)    '$(subst \,/,$(@)) && 
SILENT_IMG       = @$(OS_ECHO) '  $(tb_cyn)[IMG]$(col_rst)       '$(subst \,/,$(@)) && 
SILENT_CLEAN     = @$(OS_ECHO) '  $(tb_cyn)[CLEAN]$(col_rst)     '$(subst \,/,$(@)) && 
SILENT_GEN       = @$(OS_ECHO) '  $(tb_grn)[GEN]$(col_rst)       '$(subst \,/,$(@)) && 
SILENT_CHECK     = @$(OS_ECHO) '  $(tb_ylw)[CHECK]$(col_rst)     '$(subst \,/,$$(1)) &&  
SILENT_CC        = @$(OS_ECHO) '  $(tb_ylw)[CC]$(col_rst)        ' $(subst \,/,$(@))  &&
SILENT_CXX       = @$(OS_ECHO) '  $(tb_ylw)[CXX]$(col_rst)       ' $(subst \,/,$(@))  &&
SILENT_LD        = @$(OS_ECHO) '  $(tb_pur)[LD]$(col_rst)        ' $(subst \,/,$(@)) &&
SILENT_DEB       = @$(OS_ECHO) '  $(tb_blu)[DPKG-DEB]$(col_rst)  ' nmc-utils-$(*).deb  &&
SILENT_AR        = @$(OS_ECHO) '  $(tb_cyn)[AR]$(col_rst)        ' $(subst \,/,$(@)) &&
SILENT_PKGCONFIG = @$(OS_ECHO) '  $(tb_cyn)[PKGCONFIG]$(col_rst) ' $(subst \,/,$(@)) &&
SILENT_INSTALL   = @$(OS_ECHO) '  $(tb_grn)[INSTALL]$(col_rst)   ' $(b)  &&

#Shut up this crap
MAKEFLAGS+=--no-print-directory
endif

