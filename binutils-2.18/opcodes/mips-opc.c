/* mips-opc.c -- MIPS opcode list.
   Copyright 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002
   2003, 2004, 2005, 2007  Free Software Foundation, Inc.
   Contributed by Ralph Campbell and OSF
   Commented and modified by Ian Lance Taylor, Cygnus Support
   Extended for MIPS32 support by Anders Norlander, and by SiByte, Inc.
   MIPS-3D, MDMX, and MIPS32 Release 2 support added by Broadcom
   Corporation (SiByte).

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this file; see the file COPYING.  If not, write to the
   Free Software Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include <stdio.h>
#include "sysdep.h"
#include "opcode/mips.h"

/* Short hand so the lines aren't too long.  */

#define LDD     INSN_LOAD_MEMORY_DELAY
#define LCD	INSN_LOAD_COPROC_DELAY
#define UBD     INSN_UNCOND_BRANCH_DELAY
#define CBD	INSN_COND_BRANCH_DELAY
#define COD     INSN_COPROC_MOVE_DELAY
#define CLD	INSN_COPROC_MEMORY_DELAY
#define CBL	INSN_COND_BRANCH_LIKELY
#define TRAP	INSN_TRAP
#define SM	INSN_STORE_MEMORY

#define WR_d    INSN_WRITE_GPR_D
#define WR_t    INSN_WRITE_GPR_T
#define WR_D    INSN_WRITE_GPR_Dimm

#define RD_s    INSN_READ_GPR_S
#define RD_t    INSN_READ_GPR_T

#define RD_S	INSN_READ_SPR_Sspr
#define WR_q	INSN_WRITE_GPR_Dspr
#define WR_Q	INSN_WRITE_GPR_Dspr2
#define WR_V	INSN_WRITE_GPR_DVec
#define WR_A	INSN_WRITE_GPR_DAcc

#define I1	INSN_ISA1
#define I2	INSN_ISA2
#define I3	INSN_ISA3
#define I4	INSN_ISA4
#define I5	INSN_ISA5
#define I32	INSN_ISA32
#define I64     INSN_ISA64
#define I33	INSN_ISA32R2
#define I65	INSN_ISA64R2

/* Rigel 1 ISA */
#define IR32 INSN_RIGEL32

/* MIPS16 ASE support.  */
#define I16     INSN_MIPS16

/* MIPS64 MIPS-3D ASE support.  */
#define M3D     INSN_MIPS3D

/* MIPS32 SmartMIPS ASE support.  */
#define SMT	INSN_SMARTMIPS

/* MIPS64 MDMX ASE support.  */
#define MX      INSN_MDMX

#define P3	INSN_4650
#define L1	INSN_4010
#define V1	(INSN_4100 | INSN_4111 | INSN_4120)
#define T3      INSN_3900
#define M1	INSN_10000
#define SB1     INSN_SB1
#define N411	INSN_4111
#define N412	INSN_4120
#define N5	(INSN_5400 | INSN_5500)
#define N54	INSN_5400
#define N55	INSN_5500

#define G1      (T3             \
                 )

#define G2      (T3             \
                 )

#define G3      (I4             \
                 )

/* MIPS DSP ASE support.
   NOTE:
   1. MIPS DSP ASE includes 4 accumulators ($ac0 - $ac3).  $ac0 is the pair
   of original HI and LO.  $ac1, $ac2 and $ac3 are new registers, and have
   the same structure as $ac0 (HI + LO).  For DSP instructions that write or
   read accumulators (that may be $ac0), we add WR_a (WR_HILO) or RD_a
   (RD_HILO) attributes, such that HILO dependencies are maintained
   conservatively.

   2. For some mul. instructions that use integer registers as destinations
   but destroy HI+LO as side-effect, we add WR_HILO to their attributes.

   3. MIPS DSP ASE includes a new DSP control register, which has 6 fields
   (ccond, outflag, EFI, c, scount, pos).  Many DSP instructions read or write
   certain fields of the DSP control register.  For simplicity, we decide not
   to track dependencies of these fields.
   However, "bposge32" is a branch instruction that depends on the "pos"
   field.  In order to make sure that GAS does not reorder DSP instructions
   that writes the "pos" field and "bposge32", we add DSP_VOLA (INSN_TRAP)
   attribute to those instructions that write the "pos" field.  */

//#define WR_a	WR_HILO	/* Write dsp accumulators (reuse WR_HILO)  */
//#define RD_a	RD_HILO	/* Read dsp accumulators (reuse RD_HILO)  */
#define MOD_a	WR_a|RD_a
#define DSP_VOLA	INSN_TRAP
#define D32	INSN_DSP
#define D33	INSN_DSPR2
#define D64	INSN_DSP64

/* MIPS MT ASE support.  */
#define MT32	INSN_MT

/* The order of overloaded instructions matters.  Label arguments and
   register arguments look the same. Instructions that can have either
   for arguments must apear in the correct order in this table for the
   assembler to pick the right one. In other words, entries with
   immediate operands must apear after the same instruction with
   registers.

   Because of the lookup algorithm used, entries with the same opcode
   name must be contiguous.
 
   Many instructions are short hand for other instructions (i.e., The
   jal <register> instruction is short for jalr <register>).  */

#ifdef RIGEL_32REG_CHANGES
// XXX:
// XXX: Get from build_isa in rigel/lib/libisa/
// XXX:
	#include "rigel-isa.h"
#else
const struct mips_opcode mips_builtin_opcodes[] =
{
/* These instructions appear first so that the disassembler will find
   them first.  The assemblers uses a hash table based on the
   instruction name anyhow.  */
/* name,    args,	match,	    mask,	pinfo,          	pinfo2,		membership */
{"nop",		"",       0x00000000, 0xffffffff, 0,				INSN2_ALIAS,	IR32},
/* System instructions */
/* hi: 0x0 */
{"hlt",		"",       0x30000000, 0xffffffff, 0,				INSN2_ALIAS,	IR32},
{"fread",	"",       0x30010000, 0xffffffff, 0,				INSN2_ALIAS,	IR32},
{"fwrite","",       0x30020000, 0xffffffff, 0,				INSN2_ALIAS,	IR32},
{"fopen",	"",       0x30030000, 0xffffffff, 0,				INSN2_ALIAS,	IR32},
/* hi: 0x1 */
{"fclose","",       0x30040000, 0xffffffff, 0,				INSN2_ALIAS,	IR32},
{"abort","",       0x30050000, 0xffffffff, 0,				INSN2_ALIAS,	IR32},
{"printreg","",       0x30060000, 0xffffffff, 0,				INSN2_ALIAS,	IR32},
// S field five bits are used to encode a register for the syscall struct's address
// in memory
{"syscall","d",       0x30070000, 0xffffffc0, WR_d,				INSN2_ALIAS,	IR32},
/* hi: 0x2 */
{"tq.init","",      0x30080000, 0xffffffff, 0,				INSN2_ALIAS,	IR32},
{"tq.end","",       0x30090000, 0xffffffff, 0,				INSN2_ALIAS,	IR32},
{"tq.enqueue","",   0x300a0000, 0xffffffff, 0,				INSN2_ALIAS,	IR32},
{"tq.dequeue","",   0x300b0000, 0xffffffff, 0,				INSN2_ALIAS,	IR32},
/* hi: 0x3 */
{"tq.loop","",   0x300c0000, 0xffffffff, 0,				INSN2_ALIAS,	IR32},

/* Arithmetic */
{"add", 	"d,s,t",	0x80000000, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"addv", 	"d,s,t,V",	0x80000000, 0xf000ff00, RD_t|RD_s|WR_d|WR_V,	0,	IR32 },
{"sub", 	"d,s,t",	0x80000400, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"subv", 	"d,s,t,V",	0x80000400, 0xf000ff00, RD_t|RD_s|WR_d|WR_V,	0,	IR32 },
{"addi", 	"D,s,j",	0x20000000, 0xf0000000, RD_s|WR_D,		0,	IR32 },
{"subi", 	"D,s,j",	0x00000000, 0xf0000000, RD_s|WR_D,		0,	IR32 },
{"and", 	"d,s,t",	0x80002000, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"or",	 	"d,s,t",	0x80002400, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"xor",	 	"d,s,t",	0x80002800, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"nor",	 	"d,s,t",	0x80002c00, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"andi", 	"D,s,i",	0x50000000, 0xf0000000, RD_s|WR_D,		0,	IR32 },
{"ori", 	"D,s,i",	0x40000000, 0xf0000000, RD_s|WR_D,		0,	IR32 },
{"xori", 	"D,s,i",	0x60000000, 0xf0000000, RD_s|WR_D,		0,	IR32 },

/* Logic */
{"sll", 	"d,s,t",	0x80001000, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"srl", 	"d,s,t",	0x80001800, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"sra", 	"d,s,t",	0x80001c00, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"slli", 	"D,s,5",	0x80005000, 0xf000ffe0, RD_s|WR_D,		0,	IR32 },
{"srli", 	"D,s,5",	0x80005800, 0xf000ffe0, RD_s|WR_D,		0,	IR32 },
{"srai", 	"D,s,5",	0x80005c00, 0xf000ffe0, RD_s|WR_D,		0,	IR32 },

/* Compare */
{"ceq", 	"d,s,t",	0x80003800, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"clt", 	"d,s,t",	0x80003400, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"cle", 	"d,s,t",	0x80003c00, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"cltu", 	"d,s,t",	0x8000b400, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"cleu", 	"d,s,t",	0x8000bc00, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"cltf", 	"d,s,t",	0x8000f400, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"cltef", 	"d,s,t",	0x8000fc00, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },

/* Conditional */
{"cmeq", 	"d,s,t",	0x80009400, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"cmne", 	"d,s,t",	0x80009800, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"mfsr", 	"q,S",		0x70200000, 0xf03ffffc, RD_S|WR_q,		0,	IR32 },
{"mtsr", 	"Q,s",		0x70300000, 0xf03ffffc, RD_s|WR_Q,		0,	IR32 },
{"mvui", 	"q,i",		0x70240000, 0xf03f0000, WR_q,			0,	IR32 },

/* Branch */

{"beq", 	"s,t,p",	0xb0000000, 0xf0000000, RD_t|RD_s,		0,	IR32 },
{"bne", 	"s,t,p",	0xc0000000, 0xf0000000, RD_t|RD_s,		0,	IR32 },
{"blt", 	"s,p",		0x70020000, 0xf03f0000, RD_s,			0,	IR32 },
{"bgt", 	"s,p",		0x70030000, 0xf03f0000, RD_s,			0,	IR32 },
{"ble", 	"s,p",		0x70040000, 0xf03f0000, RD_s,			0,	IR32 },
{"bge", 	"s,p",		0x70050000, 0xf03f0000, RD_s,			0,	IR32 },

/* Jump */
{"jmp", 	"j",		0x70060000, 0xffff0000, 0,				0,	IR32 },
{"jal", 	"q,p",		0x70070000, 0xf03f0000, WR_q,			0,	IR32 },

{"jmpr", 	"s",		0x70080000, 0xf03fffff, RD_s,			0,	IR32 },
{"jalr", 	"d,s",		0x700d0000, 0xf03fffc0, WR_d|RD_s,		0,	IR32 },
{"rfe", 	"",			0x700f0000, 0xffffFFFF, 0,				0,	IR32 },

/* Load Store */ /* FIXME - This may want to be 'j' not 'p' */
{"ldw", 	"D,s,j",	0x10000000, 0xf0000000, RD_s|WR_D,		0,	IR32 },
{"stw", 	"s,t,j",	0x90000000, 0xf0000000, RD_s|RD_t,		0,	IR32 },
{"ldl", 	"D,s,j",	0xd0000000, 0xf0000000, RD_s|WR_D,		0,	IR32 },
{"atominc", 	"s,t,j",	0xe0000000, 0xf0000000, RD_s|RD_t,		0,	IR32 },

/* System */
/*{"nop",		"",         0x00000000, 0xffffffff, 0, INSN2_ALIAS,	IR32      }, */
{"mb",		"",         0x70380000, 0xffffffff, 0, 0,	IR32      }, 
{"wait",	"",         0x70100000, 0xffffffff, 0, 0,	IR32      }, 
{"ib",		"",         0x70390000, 0xffffffff, 0, 0,	IR32      }, 
{"brk",		"",         0xf0000000, 0xffffffff, 0, 0,	IR32      }, 

/* FPU */
{"fadd", 	"d,s,t",	0xa0000800, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"faddv", 	"d,s,t,V",	0xa0000800, 0xf000ff00, RD_t|RD_s|WR_d|WR_V,	0,	IR32 },
{"fadda", 	"d,s,t,A",	0xa0000c00, 0xf000fcc0, RD_t|RD_s|WR_d|WR_A,	0,	IR32 },
{"faddva", 	"d,s,t,A,V",	0xa0000c00, 0xf000fc00, RD_t|RD_s|WR_d|WR_A|WR_V,	0,	IR32 },

{"fsub", 	"d,s,t",	0xa0001800, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"fsubv", 	"d,s,t,V",	0xa0001800, 0xf000ff00, RD_t|RD_s|WR_d|WR_V,	0,	IR32 },
{"fsuba", 	"d,s,t,A",	0xa0001c00, 0xf000fcc0, RD_t|RD_s|WR_d|WR_A,	0,	IR32 },
{"fsubva", 	"d,s,t,A,V",	0xa0001c00, 0xf000fc00, RD_t|RD_s|WR_d|WR_A|WR_V,	0,	IR32 },

{"fmul", 	"d,s,t",	0xa0000000, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"fmulv", 	"d,s,t,V",	0xa0000000, 0xf000ff00, RD_t|RD_s|WR_d|WR_V,	0,	IR32 },
{"fmula", 	"d,s,t,A",	0xa0000400, 0xf000fcc0, RD_t|RD_s|WR_d|WR_A,	0,	IR32 },
{"fmulva", 	"d,s,t,A,V",	0xa0000400, 0xf000fc00, RD_t|RD_s|WR_d|WR_A|WR_V,	0,	IR32 },

{"fmadd", 	"d,s,t",	0xa0009800, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"fmaddv", 	"d,s,t,V",	0xa0009800, 0xf000ff00, RD_t|RD_s|WR_d|WR_V,	0,	IR32 },
{"fmadda", 	"d,s,t,A",	0xa0009c00, 0xf000fcc0, RD_t|RD_s|WR_d|WR_A,	0,	IR32 },
{"fmaddva",	"d,s,t,A,V",	0xa0009c00, 0xf000fc00, RD_t|RD_s|WR_d|WR_A|WR_V,	0,	IR32 },

{"fmsub", 	"d,s,t",	0xa000a000, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"fmsubv", 	"d,s,t,V",	0xa000a000, 0xf000ff00, RD_t|RD_s|WR_d|WR_V,	0,	IR32 },
{"fmsuba", 	"d,s,t,A",	0xa000a400, 0xf000fcc0, RD_t|RD_s|WR_d|WR_A,	0,	IR32 },
{"fmsubva",	"d,s,t,A,V",	0xa000a400, 0xf000fc00, RD_t|RD_s|WR_d|WR_A|WR_V,	0,	IR32 },

{"fmsubr", 	"d,s,t",	0xa000a800, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"fmsubrv",	"d,s,t,V",	0xa000a800, 0xf000ff00, RD_t|RD_s|WR_d|WR_V,	0,	IR32 },
{"fmsubra",	"d,s,t,A",	0xa000ac00, 0xf000fcc0, RD_t|RD_s|WR_d|WR_A,	0,	IR32 },
{"fmsubrva","d,s,t,A,V",	0xa000ac00, 0xf000fc00, RD_t|RD_s|WR_d|WR_A|WR_V,	0,	IR32 },

{"frcp", 	"d,s",		0xa0003000, 0xf03fffc0, RD_s|WR_d,	0,	IR32 },
{"frsq", 	"d,s",		0xa0003800, 0xf03fffc0, RD_s|WR_d,	0,	IR32 },
{"fabs", 	"d,s",		0xa0004000, 0xf03fffc0, RD_s|WR_d,	0,	IR32 },
{"fmrs", 	"d,s",		0xa0002000, 0xf03fffc0, RD_s|WR_d,	0,	IR32 },


/* Float-to-int */
{"f2i", 	"d,s",		0xa0005800, 0xf03fffc0, RD_s|WR_d,	0,	IR32 },
{"f2iv", 	"d,s,V",	0xa0005800, 0xf03fff00, RD_s|WR_d|WR_V,	0,	IR32 },

{"i2f",		"d,s",		0xa0006000, 0xf03fffc0, RD_s|WR_d,	0,	IR32 },
{"i2fv", 	"d,s,V",	0xa0006000, 0xf03fff00, RD_s|WR_d|WR_V,	0,	IR32 },

{"mul", 	"d,s,t",	0xa0008000, 0xf000ffc0, RD_t|RD_s|WR_d,	0,	IR32 },
{"mulv", 	"d,s,t,V",	0xa0008000, 0xf000ff00, RD_t|RD_s|WR_d|WR_V,	0,	IR32 },

{"clz", 	"d,s",		0xa0004800, 0xf03fffc0, RD_s|WR_d,	0,	IR32 },
{"clzv", 	"d,s,V",	0xa0004800, 0xf03fff00, RD_s|WR_d|WR_V,	0,	IR32 },

/* Sign extension */
{"sexts",   "d,s",      0xa000b000, 0xf03fffc0, RD_s|WR_d,  0,  IR32 },
{"zexts",   "d,s",      0xa000b800, 0xf03fffc0, RD_s|WR_d,  0,  IR32 },
{"sextb",   "d,s",      0xa000c000, 0xf03fffc0, RD_s|WR_d,  0,  IR32 },
{"zextb",   "d,s",      0xa000c800, 0xf03fffc0, RD_s|WR_d,  0,  IR32 }
};
#endif

#define MIPS_NUM_OPCODES \
	((sizeof mips_builtin_opcodes) / (sizeof (mips_builtin_opcodes[0])))
const int bfd_mips_num_builtin_opcodes = MIPS_NUM_OPCODES;

/* const removed from the following to allow for dynamic extensions to the
 * built-in instruction set. */
struct mips_opcode *mips_opcodes =
  (struct mips_opcode *) mips_builtin_opcodes;
int bfd_mips_num_opcodes = MIPS_NUM_OPCODES;
#undef MIPS_NUM_OPCODES
