/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 Bero
 *  Copyright (C) 2002 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef INESPRIV

void iNESStateRestore(int version);
extern uint32 iNESGameCRC32;

extern uint8 *ROM;
extern uint8 *VROM;
extern uint32 VROM_size;
extern uint32 ROM_size;

char LoadedRomFName[2048]; //bbit Edited: line added

extern int iNesSave(); //bbit Edited: line added

extern void (*MapStateRestore)(int version);
extern void (*MapClose)(void);
extern void (*MapperReset)(void);

/* This order is necessary */
#define WRAM          (GameMemBlock)
#define sizeofWRAM    8192

#define MapperExRAM   (GameMemBlock+sizeofWRAM)
#define sizeofMapperExRAM  32768
/* for the MMC5 code to work properly.  It might be fixed later... */

#define CHRRAM        (GameMemBlock+sizeofWRAM+sizeofMapperExRAM)
#define sizeofCHRRAM 8192

#define ExtraNTARAM   (GameMemBlock+sizeofWRAM+sizeofMapperExRAM+sizeofCHRRAM)
#define sizeofExtraNTARAM 2048

#define PRGBankList    (ExtraNTARAM+sizeofExtraNTARAM)

#define mapbyte1       (PRGBankList+4)
#define mapbyte2       (mapbyte1+8)
#define mapbyte3       (mapbyte2+8)
#define mapbyte4       (mapbyte3+8)
uint16 iNESCHRBankList[8];
int32 iNESIRQLatch,iNESIRQCount;
uint8 iNESMirroring;
uint8 iNESIRQa;

#define IRQa iNESIRQa
#define Mirroring iNESMirroring
#define IRQCount iNESIRQCount
#define IRQLatch iNESIRQLatch
#define CHRBankList iNESCHRBankList
#else
#endif

	typedef struct {
		char ID[4]; /*NES^Z*/
                uint8 ROM_size;
                uint8 VROM_size;
                uint8 ROM_type;
                uint8 ROM_type2;
                uint8 reserve[8];
	} iNES_HEADER;

void FASTAPASS(2) VRAM_BANK1(uint32 A, uint8 V);
void FASTAPASS(2) VRAM_BANK4(uint32 A,uint32 V);

void FASTAPASS(2) VROM_BANK1(uint32 A,uint32 V);
void FASTAPASS(2) VROM_BANK2(uint32 A,uint32 V);
void FASTAPASS(2) VROM_BANK4(uint32 A, uint32 V);
void FASTAPASS(1) VROM_BANK8(uint32 V);
void FASTAPASS(2) ROM_BANK8(uint32 A, uint32 V);
void FASTAPASS(2) ROM_BANK16(uint32 A, uint32 V);
void FASTAPASS(1) ROM_BANK32(uint32 V);

extern uint8 vmask;
extern uint32 vmask1;
extern uint32 vmask2;
extern uint32 vmask4;
extern uint32 pmask8;
extern uint8 pmask16;
extern uint8 pmask32;

void FASTAPASS(1) onemir(uint8 V);
void FASTAPASS(1) MIRROR_SET2(uint8 V);
void FASTAPASS(1) MIRROR_SET(uint8 V);


void DetectMMC1WRAMSize(void);
void DetectMMC5WRAMSize(void);

void Mapper0_init(void);
void Mapper1_init(void);
void Mapper2_init(void);
void Mapper3_init(void);
void Mapper4_init(void);
void Mapper5_init(void);
void Mapper6_init(void);
void Mapper7_init(void);
void Mapper8_init(void);
void Mapper9_init(void);
void Mapper10_init(void);
void Mapper11_init(void);
void Mapper12_init(void);
void Mapper13_init(void);
void Mapper14_init(void);
void Mapper15_init(void);
void Mapper16_init(void);
void Mapper17_init(void);
void Mapper18_init(void);
void Mapper19_init(void);
void Mapper20_init(void);
void Mapper21_init(void);
void Mapper22_init(void);
void Mapper23_init(void);
void Mapper24_init(void);
void Mapper25_init(void);
void Mapper26_init(void);
void Mapper27_init(void);
void Mapper28_init(void);
void Mapper29_init(void);
void Mapper30_init(void);
void Mapper31_init(void);
void Mapper32_init(void);
void Mapper33_init(void);
void Mapper34_init(void);
void Mapper35_init(void);
void Mapper36_init(void);
void Mapper37_init(void);
void Mapper38_init(void);
void Mapper39_init(void);
void Mapper40_init(void);
void Mapper41_init(void);
void Mapper42_init(void);
void Mapper43_init(void);
void Mapper44_init(void);
void Mapper45_init(void);
void Mapper46_init(void);
void Mapper47_init(void);
void Mapper48_init(void);
void Mapper49_init(void);
void Mapper50_init(void);
void Mapper51_init(void);
void Mapper52_init(void);
void Mapper53_init(void);
void Mapper54_init(void);
void Mapper55_init(void);
void Mapper56_init(void);
void Mapper57_init(void);
void Mapper58_init(void);
void Mapper59_init(void);
void Mapper60_init(void);
void Mapper61_init(void);
void Mapper62_init(void);
void Mapper63_init(void);
void Mapper64_init(void);
void Mapper65_init(void);
void Mapper66_init(void);
void Mapper67_init(void);
void Mapper68_init(void);
void Mapper69_init(void);
void Mapper70_init(void);
void Mapper71_init(void);
void Mapper72_init(void);
void Mapper73_init(void);
void Mapper74_init(void);
void Mapper75_init(void);
void Mapper76_init(void);
void Mapper77_init(void);
void Mapper78_init(void);
void Mapper79_init(void);
void Mapper80_init(void);
void Mapper81_init(void);
void Mapper82_init(void);
void Mapper83_init(void);
void Mapper84_init(void);
void Mapper85_init(void);
void Mapper86_init(void);
void Mapper87_init(void);
void Mapper88_init(void);
void Mapper89_init(void);
void Mapper90_init(void);
void Mapper91_init(void);
void Mapper92_init(void);
void Mapper93_init(void);
void Mapper94_init(void);
void Mapper95_init(void);
void Mapper96_init(void);
void Mapper97_init(void);
void Mapper98_init(void);
void Mapper99_init(void);
void Mapper100_init(void);
void Mapper101_init(void);
void Mapper102_init(void);
void Mapper103_init(void);
void Mapper104_init(void);
void Mapper105_init(void);
void Mapper106_init(void);
void Mapper107_init(void);
void Mapper108_init(void);
void Mapper109_init(void);
void Mapper110_init(void);
void Mapper111_init(void);
void Mapper112_init(void);
void Mapper113_init(void);
void Mapper114_init(void);
void Mapper115_init(void);
void Mapper116_init(void);
void Mapper117_init(void);
void Mapper118_init(void);
void Mapper119_init(void);
void Mapper120_init(void);
void Mapper121_init(void);
void Mapper122_init(void);
void Mapper123_init(void);
void Mapper124_init(void);
void Mapper125_init(void);
void Mapper126_init(void);
void Mapper127_init(void);
void Mapper128_init(void);
void Mapper129_init(void);
void Mapper130_init(void);
void Mapper131_init(void);
void Mapper132_init(void);
void Mapper133_init(void);
void Mapper134_init(void);
void Mapper135_init(void);
void Mapper136_init(void);
void Mapper137_init(void);
void Mapper138_init(void);
void Mapper139_init(void);
void Mapper140_init(void);
void Mapper141_init(void);
void Mapper142_init(void);
void Mapper143_init(void);
void Mapper144_init(void);
void Mapper145_init(void);
void Mapper146_init(void);
void Mapper147_init(void);
void Mapper148_init(void);
void Mapper149_init(void);
void Mapper150_init(void);
void Mapper151_init(void);
void Mapper152_init(void);
void Mapper153_init(void);
void Mapper154_init(void);
void Mapper155_init(void);
void Mapper156_init(void);
void Mapper157_init(void);
void Mapper158_init(void);
void Mapper159_init(void);
void Mapper160_init(void);
void Mapper161_init(void);
void Mapper162_init(void);
void Mapper163_init(void);
void Mapper164_init(void);
void Mapper165_init(void);
void Mapper166_init(void);
void Mapper167_init(void);
void Mapper168_init(void);
void Mapper169_init(void);
void Mapper170_init(void);
void Mapper171_init(void);
void Mapper172_init(void);
void Mapper173_init(void);
void Mapper174_init(void);
void Mapper175_init(void);
void Mapper176_init(void);
void Mapper177_init(void);
void Mapper178_init(void);
void Mapper179_init(void);
void Mapper180_init(void);
void Mapper181_init(void);
void Mapper182_init(void);
void Mapper183_init(void);
void Mapper184_init(void);
void Mapper185_init(void);
void Mapper186_init(void);
void Mapper187_init(void);
void Mapper188_init(void);
void Mapper189_init(void);
void Mapper190_init(void);
void Mapper191_init(void);
void Mapper192_init(void);
void Mapper193_init(void);
void Mapper194_init(void);
void Mapper195_init(void);
void Mapper196_init(void);
void Mapper197_init(void);
void Mapper198_init(void);
void Mapper199_init(void);
void Mapper200_init(void);
void Mapper201_init(void);
void Mapper202_init(void);
void Mapper203_init(void);
void Mapper204_init(void);
void Mapper205_init(void);
void Mapper206_init(void);
void Mapper207_init(void);
void Mapper208_init(void);
void Mapper209_init(void);
void Mapper210_init(void);
void Mapper211_init(void);
void Mapper212_init(void);
void Mapper213_init(void);
void Mapper214_init(void);
void Mapper215_init(void);
void Mapper216_init(void);
void Mapper217_init(void);
void Mapper218_init(void);
void Mapper219_init(void);
void Mapper220_init(void);
void Mapper221_init(void);
void Mapper222_init(void);
void Mapper223_init(void);
void Mapper224_init(void);
void Mapper225_init(void);
void Mapper226_init(void);
void Mapper227_init(void);
void Mapper228_init(void);
void Mapper229_init(void);
void Mapper230_init(void);
void Mapper231_init(void);
void Mapper232_init(void);
void Mapper233_init(void);
void Mapper234_init(void);
void Mapper235_init(void);
void Mapper236_init(void);
void Mapper237_init(void);
void Mapper238_init(void);
void Mapper239_init(void);
void Mapper240_init(void);
void Mapper241_init(void);
void Mapper242_init(void);
void Mapper243_init(void);
void Mapper244_init(void);
void Mapper245_init(void);
void Mapper246_init(void);
void Mapper247_init(void);
void Mapper248_init(void);
void Mapper249_init(void);
void Mapper250_init(void);
void Mapper251_init(void);
void Mapper252_init(void);
void Mapper253_init(void);
void Mapper254_init(void);
void Mapper255_init(void);

void NSFVRC6_Init(void);
void NSFMMC5_Init(void);
void NSFAY_Init(void);
void NSFN106_Init(void);
void NSFVRC7_Init(void);
void Mapper19_ESI(void);
