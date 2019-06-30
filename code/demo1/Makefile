#===============================================================================
# Demo S28 Definitions
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
	fibonacci.s28

LK_FLAGS = \
	-bss $$001000-$$001fff -code $$002000-$$0023ff

all:	$(TARGETS)

fibonacci.s28: fibonacci.obj
	$(LK65) $(LK_FLAGS) -s28 -output $@ fibonacci.obj

clean:
	$(RM) *.obj
	$(RM) *.lst
	$(RM) *.map
	$(RM) *.s28
	
#===============================================================================
# Dependencies
#-------------------------------------------------------------------------------

fibonacci.obj: \
	fibonacci.asm ..\w65c816.inc

