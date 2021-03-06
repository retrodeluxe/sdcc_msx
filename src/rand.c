#include <rand.h>

////////////////////////////////////////////////////////////////////////
// GENERIC UTILITY FUNCTIONS

static uint16_t rand16_seed1;
static uint16_t rand16_seed2;


#ifdef MSX
static void rand16_placeholder(void) {	
    __asm
    _rand16::
	push bc
    ld hl,(#_rand16_seed1)
    ld b,l
    ld c,h
    add hl,hl
    add hl,hl
    inc l
    add hl,bc
    ld (#_rand16_seed1),hl
    ld hl,(#_rand16_seed2)
    add hl,hl
    sbc a, a
    add a, #45
    xor a,l
    ld l,a
    ld (#_rand16_seed2),hl
;    ld a, c
;    ld c, b
;    ld b, a
    add hl,bc
    pop bc
    ret	
    __endasm;
}
#else
	
uint16_t rand16() {

	uint16_t hl = rand16_seed1;
	uint16_t bc = (hl<<8) + (hl>>8);
	hl <<= 2;
	hl++;
	hl += bc;
	rand16_seed1 = hl;
	hl = rand16_seed2;
    if (hl&0x8000) {
        hl = 44 ^ (hl<<1);
    } else {
        hl = 45 ^ (hl<<1);
    }
	rand16_seed2 = hl;
	hl += bc;
	return hl;
}

#endif
	
	/* if need to be:
// from: http://wikiti.brandonw.net/index.php?title=Z80_Routines:Math:Random
prng16:
;Inputs:
;   (seed1) contains a 16-bit seed value
;   (seed2) contains a NON-ZERO 16-bit seed value
	push bc
    ld hl,(seed1)
    ld b,l
    ld c,h
    add hl,hl
    add hl,hl
    inc l
    add hl,bc
    ld (seed1),hl
    ld hl,(seed2)
    add hl,hl
    sbc a,a
    add $00101101
    xor l
    ld l,a
    ld (seed2),hl
    add hl,bc
    pop bc
    ret	

	*/
	
void rand16_seed(uint16_t seed1, uint16_t seed2) {

//  (seed1) contains a 16-bit seed value
//  (seed2) contains a NON-ZERO 16-bit seed value
    rand16_seed1 = seed1 ^ 0xDEAD;
    rand16_seed2 = seed2 ^ 0xC0DE;
    if (rand16_seed2==0) seed2++;
}

void rand16_seedTime() {

//  (seed1) contains a 16-bit seed value
//  (seed2) contains a NON-ZERO 16-bit seed value
    uint8_t s = rand7();
    rand16_seed1 = s;
    rand16_seed2 = s|1;
}

#ifdef MSX
inline static void rand7_placeholder(void) {	
    __asm
    _rand7::
	ld a,r
	ld l,a
	ret
    __endasm;
}
#else

#include <stdlib.h>
#include <time.h>
uint8_t rand7() { 
    srand (time(NULL));
    return rand()%0x80; 
}
#endif

