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
	rom2.h

LK_FLAGS = \
	-bss $$0000-$$7FFF -code $$060000-$$06ffff

all:	$(TARGETS)

rom2.h: rom2.obj
	$(LK65) $(LK_FLAGS) -c -output $@ rom2.obj
	$(CP) $@ ..\..

clean:
	$(RM) *.obj
	$(RM) *.lst
	$(RM) *.map
	$(RM) *.h
	
#===============================================================================
# Dependencies
#-------------------------------------------------------------------------------

rom2.obj: \
	rom2.asm ..\w65c816.inc

