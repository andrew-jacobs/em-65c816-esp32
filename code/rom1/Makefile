#===============================================================================
# ROM Build Definitions
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
	rom1.h

LK_FLAGS = \
	-bss $$0000-$$7FFF -code $$050000-$$05ffff

all:	$(TARGETS)

rom1.h: rom1.obj
	$(LK65) $(LK_FLAGS) -c -output $@ rom1.obj
	$(CP) $@ ..\..

clean:
	$(RM) *.obj
	$(RM) *.lst
	$(RM) *.map
	$(RM) *.h
	
#===============================================================================
# Dependencies
#-------------------------------------------------------------------------------

rom1.obj: \
	rom1.asm ..\w65c816.inc

