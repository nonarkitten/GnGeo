	.align 2
	.globl	_clr_screen_m68k
	.globl  _clr_border_m68k
	.globl  _dim_screen_m68k
	
_clr_screen_m68k:
	move.l 4(sp),a0
	move.w 10(sp),d0
	
	swap   d0
	move.w 10(sp),d0
	
	adda   #((16*384 + 8)*2),a0
	
	move.w #223,d1
.loop:
	.rept  (304/2)
	move.l d0,(a0)+
	.endr
	adda   #(((32+8)*2)*2),a0
	dbra   d1, .loop	
	rts

_dim_screen_m68k:
	move.l 4(sp),a0	
	adda   #((16*384 + 8)*2),a0
	move.w #223,d1
.loop2:
	.rept  (304/2)
	move.l (a0),d0
	and.l #2078178270,d0
	lsr.l  #1,d0
	move.l d0,(a0)+
	.endr
	adda   #(((32+8)*2)*2),a0
	dbra   d1, .loop2
	rts


	
_clr_border_m68k:
	move.l 4(sp),a0
	move.w 10(sp),d0
	
	swap   d0
	move.w 10(sp),d0

	move.l #15,d1
.loop1:
	.rept  (384/2)
	move.l d0,(a0)+
	.endr
	dbra   d1,.loop1
	
	rts
