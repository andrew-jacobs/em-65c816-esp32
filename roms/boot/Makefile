#===============================================================================
# OS65 Kernel Build Definitions
#-------------------------------------------------------------------------------
# Copyright (C),2018 HandCoded Software Ltd.
# All rights reserved.
#
# This work is made available under the terms of the Creative Commons
# Attribution-NonCommercial 2.0 license. Open the following URL to see the
# details.
#
# http://creativecommons.org/licenses/by-nc/2.0/
#-------------------------------------------------------------------------------
#
# Notes:
#
#
#
#-------------------------------------------------------------------------------

DEV65_DIR = ..

include	../MakeDefns

#===============================================================================
# Targets
#-------------------------------------------------------------------------------

TARGETS	= \
	boot.inc

LK_FLAGS = \
	-bss $$0000-$$7FFF -code $$f000-$$ffff

all:	$(TARGETS)

boot.inc: boot.obj
	$(LK65) $(LK_FLAGS) -c -output $@ boot.obj

clean:
	$(RM) *.obj
	$(RM) *.s19
	$(RM) *.s28
	$(RM) *.lst
	$(RM) *.map
	$(RM) *.bin
	$(RM) *.log

#===============================================================================
# Dependencies
#-------------------------------------------------------------------------------

boot.obj: \
	boot.asm ..\w65c816.inc

