#===============================================================================
# ROM Build Definitions
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
	rom3.h

LK_FLAGS = \
	-bss $$020000-$$02ffff -code $$070000-$$07ffff

all:	$(TARGETS)

rom3.h: rom3.obj
	$(LK65) $(LK_FLAGS) -c -output $@ rom3.obj
	$(CP) $@ ..\..

clean:
	$(RM) *.obj
	$(RM) *.lst
	$(RM) *.map
	$(RM) *.h
	
#===============================================================================
# Dependencies
#-------------------------------------------------------------------------------

rom3.obj: \
	rom3.asm ..\w65c816.inc

