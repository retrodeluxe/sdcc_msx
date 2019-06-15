#pragma once

#include <msxhal.h>

////////////////////////////////////////////////////////////////////////
// Typedefs


typedef enum {
BTransparent=0x0,
BBlack,
BMediumGreen,
BLightGreen,
BDarkBlue,  	
BLightBlue,
BDarkRed,
BCyan,
BMediumRed,
BLightRed,
BDarkYellow,
BLightYellow,
BDarkGreen,
BMagenta,
BGray,
BWhite} 
TMS99X8_BackgroundColor;

typedef enum {
FTransparent=0x00,
FBlack=0x10,
FMediumGreen=0x20,
FLightGreen=0x30,
FDarkBlue=0x40,
FLightBlue=0x50,
FDarkRed=0x60,
FCyan=0x70,
FMediumRed=0x80,	
FLightRed=0x90,
FDarkYellow=0xA0,
FLightYellow=0xB0,
FDarkGreen=0xC0,
FMagenta=0xD0,
FGray=0xE0,
FWhite=0xF0} 
TMS99X8_ForegroundColor;


////////////////////////////////////////////////////////////////////////
// Mode2 Interface

typedef enum {
MODE2_ADDRESS_PN0 = 0x1800,
MODE2_ADDRESS_PN1 = 0x1C00,
MODE2_ADDRESS_CT  = 0x2000,
MODE2_ADDRESS_PG  = 0x0000,
MODE2_ADDRESS_SA0 = 0x1F00,
MODE2_ADDRESS_SA1 = 0x1F80,
MODE2_ADDRESS_SG  = 0x3800} 
EM2_Adresses;

typedef struct {
	uint8_t y,x;
	uint8_t pattern;
	uint8_t color;} 
EM2_Sprite;

#define N_SPRITES 32
#define TILE_WIDTH 32
#define TILE_HEIGHT 24
#define TEX_WIDTH (TILE_WIDTH*8)
#define TEX_HEIGHT (TILE_HEIGHT*8)

typedef uint8_t T_PN[TILE_HEIGHT][TILE_WIDTH]; // Pattern Name Table
typedef U8x8 T_CT[3][256]; // Pattern Color Table
typedef U8x8 T_PG[3][256]; // Pattern Generator Table
typedef EM2_Sprite T_SA[N_SPRITES]; // Sprite Attribute Table
typedef U8x8 T_SG[256]; // Sprite Generator Table

typedef enum {
MODE2_ROWS_0__7 = 1,
MODE2_ROWS_8_15 = 2,
MODE2_ROWS16_23 = 4,
MODE2_ALL_ROWS  = 7}
EM2_RowPageFlags;

typedef uint8_t RowPageIdx;


typedef enum {
MODE2_BUFFER_0 = 0,
MODE2_BUFFER_1 = 1} 
EM2_Buffer;

////////////////////////////////////////////////////////////////////////
// High Level Interface

void TMS99X8_activateMode2 (EM2_RowPageFlags rowPages);
void TMS99X8_activateBuffer(EM2_Buffer buffer);

////////////////////////////////////////////////////////////////////////
// Registers and Flags

typedef enum {
TMS99X8_M2 = 0x0002,
TMS99X8_EXTVID = 0x0001,
TMS99X8_MEM416K = 0x8000,
TMS99X8_ENABLE = 0x4000,
TMS99X8_GINT = 0x2000,
TMS99X8_M1 = 0x1000,
TMS99X8_M3 = 0x0800,
TMS99X8_SI = 0x0200,
TMS99X8_MAG = 0x0100} 
TMS99X8_Flags;
typedef uint16_t TMS99X8_TFlags;

typedef struct {
	union {
		uint8_t reg[8];
		uint16_t flags;
		struct {
			struct {
				uint8_t extvid : 1;		
				uint8_t mode2 : 1;
				uint8_t reserved1 : 6;
			};
			struct {
				uint8_t magnifySprites : 1;
				uint8_t sprites16 : 1;
				uint8_t reserved2: 1;
				uint8_t mode3 : 1;
				uint8_t mode1 : 1;
				uint8_t generateInterrupts : 1;
				uint8_t blankScreen : 1;
				uint8_t mem416K : 1;
			};
			uint8_t pn10, ct6, pg11, sa7, sg11;
			struct {
				uint8_t backdrop : 4;
				uint8_t textcolor : 4;
			};
		};
	};
} 
TMS99X8_Register;

////////////////////////////////////////////////////////////////////////
// Mid Level Interface

extern T_SA SA0, SA1;

void TMS99X8_clear();

void TMS99X8_setFlags(TMS99X8_TFlags flags);
void TMS99X8_memcpy(uint16_t dst, const uint8_t *src, uint16_t size);
void TMS99X8_memset(uint16_t dst, uint8_t value, uint16_t size);

void TMS99X8_writeSprite8(uint8_t pos, U8x8 s);
void TMS99X8_writeSprite16(uint8_t pos, U16x16 s);

void TMS99X8_writeSpriteAttributes(EM2_Buffer buffer, T_SA sa);

////////////////////////////////////////////////////////////////////////
// Low Level Interface

void TMS99X8_setRegister(uint8_t reg, uint8_t val);

#ifdef MSX
	__sfr __at 0x98 VDP0;
	__sfr __at 0x99 VDP1;
	TMS99X8_Register __at 0xF3DF TMS99X8;

	inline void TMS99X8_syncRegister(uint8_t reg) {

		VDP1 = TMS99X8.reg[reg];
		NOP();
		VDP1 = 0x80 | reg;
		NOP();
	}

#else

	extern TMS99X8_Register TMS99X8;
	extern uint8_t TMS99X8VRAM[0x4000];
	inline static void TMS99X8_syncRegister(uint8_t reg) {(void)reg;}
#endif



