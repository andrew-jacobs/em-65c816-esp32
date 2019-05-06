//==============================================================================
//  _____ __  __        __  ____   ____ ___  _  __   
// | ____|  \/  |      / /_| ___| / ___( _ )/ |/ /_  
// |  _| | |\/| |_____| '_ \___ \| |   / _ \| | '_ \ 
// | |___| |  | |_____| (_) |__) | |__| (_) | | (_) |
// |_____|_|__|_|___ __\___/____/ \____\___/|_|\___/ 
// | ____/ ___||  _ \___ /___ \                      
// |  _| \___ \| |_) ||_ \ __) |                     
// | |___ ___) |  __/___) / __/                      
// |_____|____/|_|  |____/_____|                     
//
//------------------------------------------------------------------------------                                                   
// Copyright (C),2019 Andrew John Jacobs
// All rights reserved.
//
// This work is made available under the terms of the Creative Commons
// Attribution-NonCommercial-ShareAlike 4.0 International license. Open the
// following URL to see the details.
//
// http://creativecommons.org/licenses/by-nc-sa/4.0/
//------------------------------------------------------------------------------
// Notes:
//
//==============================================================================

#include <Arduino.h>

#pragma GCC optimize ("-O4")

#include "Emulator.h"

//=============================================================================

#define	OPCODE_SET \
	&do_irq, &do_nmi, &do_abort, \
	{ \
		&op_00, &op_01, &op_02, &op_03, &op_04, &op_05, &op_06, &op_07, \
		&op_08, &op_09, &op_0a, &op_0b, &op_0c, &op_0d, &op_0e, &op_0f, \
		&op_10, &op_11, &op_12, &op_13, &op_14, &op_15, &op_16, &op_17, \
		&op_18, &op_19, &op_1a, &op_1b, &op_1c, &op_1d, &op_1e, &op_1f, \
		&op_20, &op_21, &op_22, &op_23, &op_24, &op_25, &op_26, &op_27, \
		&op_28, &op_29, &op_2a, &op_2b, &op_2c, &op_2d, &op_2e, &op_2f, \
		&op_30, &op_31, &op_32, &op_33, &op_34, &op_35, &op_36, &op_37, \
		&op_38, &op_39, &op_3a, &op_3b, &op_3c, &op_3d, &op_3e, &op_3f, \
		&op_40, &op_41, &op_42, &op_43, &op_44, &op_45, &op_46, &op_47, \
		&op_48, &op_49, &op_4a, &op_4b, &op_4c, &op_4d, &op_4e, &op_4f, \
		&op_50, &op_51, &op_52, &op_53, &op_54, &op_55, &op_56, &op_57, \
		&op_58, &op_59, &op_5a, &op_5b, &op_5c, &op_5d, &op_5e, &op_5f, \
		&op_60, &op_61, &op_62, &op_63, &op_64, &op_65, &op_66, &op_67, \
		&op_68, &op_69, &op_6a, &op_6b, &op_6c, &op_6d, &op_6e, &op_6f, \
		&op_70, &op_71, &op_72, &op_73, &op_74, &op_75, &op_76, &op_77, \
		&op_78, &op_79, &op_7a, &op_7b, &op_7c, &op_7d, &op_7e, &op_7f, \
		&op_80, &op_81, &op_82, &op_83, &op_84, &op_85, &op_86, &op_87, \
		&op_88, &op_89, &op_8a, &op_8b, &op_8c, &op_8d, &op_8e, &op_8f, \
		&op_90, &op_91, &op_92, &op_93, &op_94, &op_95, &op_96, &op_97, \
		&op_98, &op_99, &op_9a, &op_9b, &op_9c, &op_9d, &op_9e, &op_9f, \
		&op_a0, &op_a1, &op_a2, &op_a3, &op_a4, &op_a5, &op_a6, &op_a7, \
		&op_a8, &op_a9, &op_aa, &op_ab, &op_ac, &op_ad, &op_ae, &op_af, \
		&op_b0, &op_b1, &op_b2, &op_b3, &op_b4, &op_b5, &op_b6, &op_b7, \
		&op_b8, &op_b9, &op_ba, &op_bb, &op_bc, &op_bd, &op_be, &op_bf, \
		&op_c0, &op_c1, &op_c2, &op_c3, &op_c4, &op_c5, &op_c6, &op_c7, \
		&op_c8, &op_c9, &op_ca, &op_cb, &op_cc, &op_cd, &op_ce, &op_cf, \
		&op_d0, &op_d1, &op_d2, &op_d3, &op_d4, &op_d5, &op_d6, &op_d7, \
		&op_d8, &op_d9, &op_da, &op_db, &op_dc, &op_dd, &op_de, &op_df, \
		&op_e0, &op_e1, &op_e2, &op_e3, &op_e4, &op_e5, &op_e6, &op_e7, \
		&op_e8, &op_e9, &op_ea, &op_eb, &op_ec, &op_ed, &op_ee, &op_ef, \
		&op_f0, &op_f1, &op_f2, &op_f3, &op_f4, &op_f5, &op_f6, &op_f7, \
		&op_f8, &op_f9, &op_fa, &op_fb, &op_fc, &op_fd, &op_fe, &op_ff  \
	}

const OpcodeSet		CpuModeE11::opcodeSet =
{
	OPCODE_SET
};

const OpcodeSet		CpuModeN00::opcodeSet =
{
	OPCODE_SET
};

const OpcodeSet		CpuModeN01::opcodeSet =
{
	OPCODE_SET
};

const OpcodeSet		CpuModeN10::opcodeSet =
{
	OPCODE_SET
};

const OpcodeSet		CpuModeN11::opcodeSet =
{
	OPCODE_SET
};