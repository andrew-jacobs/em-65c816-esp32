#===============================================================================
# Boot ROM Build Definitions
#-------------------------------------------------------------------------------
# Copyright (C),2019 HandCoded Software Ltd.
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
	boot.h rom0.h

LK1_FLAGS = \
	-bss $$0000-$$efff -code $$00f000-$$00ffff

LK2_FLAGS = \
	-bss $$0000-$$efff -code $$040000-$$04ffff

all:	$(TARGETS)

boot.h: boot.obj
	$(LK65) $(LK1_FLAGS) -c -output $@ boot.obj
	$(CP) $@ ..\..

rom0.h: boot.obj
	$(LK65) $(LK2_FLAGS) -c -output $@ boot.obj
	$(CP) $@ ..\..

clean:
	$(RM) *.obj
	$(RM) *.lst
	$(RM) *.map
	$(RM) *.h
	
#===============================================================================
# Dependencies
#-------------------------------------------------------------------------------

boot.obj: \
	boot.asm ..\w65c816.inc

