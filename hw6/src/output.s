.text
.global MAIN
_start_MAIN:
	addi sp, sp, -16
	sd ra, 8(sp)
	sd fp, 0(sp)
	mv fp, sp
	la ra, _frameSize_MAIN
	lw ra, 0(ra)
	sub sp, sp, ra
	sd x9, -8(fp)
	sd x18, -16(fp)
	sd x19, -24(fp)
	sd x20, -32(fp)
	sd x21, -40(fp)
	sd x22, -48(fp)
	sd x23, -56(fp)
	sd x24, -64(fp)
	sd x25, -72(fp)
	sd x26, -80(fp)
	sd x27, -88(fp)
	fsd f0, -96(fp)
	fsd f1, -104(fp)
	fsd f2, -112(fp)
	fsd f3, -120(fp)
	fsd f4, -128(fp)
	fsd f5, -136(fp)
	fsd f6, -144(fp)
	fsd f7, -152(fp)
	fsd f8, -160(fp)
	fsd f9, -168(fp)
	fsd f10, -176(fp)
	fsd f11, -184(fp)
	fsd f12, -192(fp)
	fsd f13, -200(fp)
	fsd f14, -208(fp)
	fsd f15, -216(fp)
.text
	li x9, 1
	addi x19, fp, -220
	sw x9, 0(x19)
.text
	li x9, 2
	addi x21, fp, -224
	sw x9, 0(x21)
	addi x24, fp, -220
	lw x9, 0(x24)
	addi x25, fp, -224
	lw x22, 0(x25)
	add x23, x9, x22
	addi x26, fp, -232
	sw x23, 0(x26)
	addi x22, fp, -232
	lw x9, 0(x22)
	addi x23, fp, -228
	sw x9, 0(x23)
	j _end_MAIN
_end_MAIN:
	ld x9, -8(fp)
	ld x18, -16(fp)
	ld x19, -24(fp)
	ld x20, -32(fp)
	ld x21, -40(fp)
	ld x22, -48(fp)
	ld x23, -56(fp)
	ld x24, -64(fp)
	ld x25, -72(fp)
	ld x26, -80(fp)
	ld x27, -88(fp)
	fld f0, -96(fp)
	fld f1, -104(fp)
	fld f2, -112(fp)
	fld f3, -120(fp)
	fld f4, -128(fp)
	fld f5, -136(fp)
	fld f6, -144(fp)
	fld f7, -152(fp)
	fld f8, -160(fp)
	fld f9, -168(fp)
	fld f10, -176(fp)
	fld f11, -184(fp)
	fld f12, -192(fp)
	fld f13, -200(fp)
	fld f14, -208(fp)
	fld f15, -216(fp)
	ld ra, 8(fp)
	addi sp, fp, -8
	ld fp, 0(fp)
	jr ra
.data
_frameSize_MAIN: .word 232
