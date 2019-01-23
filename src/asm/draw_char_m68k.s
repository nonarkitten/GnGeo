	.align	2
	.globl	_draw_fix_char_m68k
	
_draw_fix_char_m68k:
draw_fix_char_m68k:
	move.l 	4(%sp),d1		| byte1 (tile)
	move.l 	8(%sp),d0		| byte2 (palette)
	move.l 	12(%sp),a1		| screen pointer

	movem.l  %d2/%a2,-(%sp)
	
	move.l 	_current_pc_pal,%a0
	lsl.l 	#6,%d0
	add.l 	%d0,%a0
	
	move.l 	_current_fix,%a2	
	lsl.l 	#5,%d1
	add.l 	%d1,%a2
	
	moveq.l	#7,%d2
	
	.even
	
.start:
	move.l	(%a2)+,d1
	
	bfextu	d1{#0:#4},d0
	beq		.pix1
	movew 	a0@(2,d0:l:4),a1@(14)
.pix1:
	bfextu	d1{#4:#4},d0
	beq		.pix2
	movew 	a0@(2,d0:l:4),a1@(12)
.pix2:
	bfextu	d1{#8:#4},d0
	beq		.pix3
	movew 	a0@(2,d0:l:4),a1@(10)
.pix3:
	bfextu	d1{#12:#4},d0
	beq		.pix4
	movew 	a0@(2,d0:l:4),a1@(8)
.pix4:
	bfextu	d1{#16:#4},d0
	beq		.pix5
	movew 	a0@(2,d0:l:4),a1@(6)
.pix5:
	bfextu	d1{#20:#4},d0
	beq		.pix6
	movew 	a0@(2,d0:l:4),a1@(4)
.pix6:
	bfextu	d1{#24:#4},d0
	beq		.pix7
	movew 	a0@(2,d0:l:4),a1@(2)
.pix7:
	bfextu	d1{#28:#4},d0
	beq		.next
	movew 	a0@(2,d0:l:4),a1@(0)
.next:
	lea		640(%a1),%a1
	dbra	%d2,.start
	movem.l	(%sp)+,%d2/%a2
	rts
