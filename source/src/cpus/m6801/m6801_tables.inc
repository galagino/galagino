// Generated from MAME 0.289 m6801.cpp (cycles_6803[], m6803_insn[]) by make_ops.sh
#define XX 4 // illegal opcode unknown cycle count
static const uint8_t DRAM_ATTR cycles_6803[256] =
{
		/* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
	/*0*/ XX, 2,XX,XX, 3, 3, 2, 2, 3, 3, 2, 2, 2, 2, 2, 2,
	/*1*/  2, 2,XX,XX,XX,XX, 2, 2,XX, 2,XX, 2,XX,XX,XX,XX,
	/*2*/  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	/*3*/  3, 3, 4, 4, 3, 3, 3, 3, 5, 5, 3,10, 4,10, 9,12,
	/*4*/  2,XX,XX, 2, 2,XX, 2, 2, 2, 2, 2,XX, 2, 2,XX, 2,
	/*5*/  2,XX,XX, 2, 2,XX, 2, 2, 2, 2, 2,XX, 2, 2,XX, 2,
	/*6*/  6,XX,XX, 6, 6,XX, 6, 6, 6, 6, 6,XX, 6, 6, 3, 6,
	/*7*/  6,XX,XX, 6, 6,XX, 6, 6, 6, 6, 6,XX, 6, 6, 3, 6,
	/*8*/  2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 2, 4, 6, 3, 3,
	/*9*/  3, 3, 3, 5, 3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 4, 4,
	/*A*/  4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 5, 5,
	/*B*/  4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 5, 5,
	/*C*/  2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 2, 3,XX, 3, 3,
	/*D*/  3, 3, 3, 5, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4,
	/*E*/  4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5,
	/*F*/  4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5
};
#undef XX
static void (*const DRAM_ATTR m6803_insn[0x100])() = {
// 0/8                     1/9                        2/A                        3/B                        4/C                        5/D                        6/E                        7/F
&illegl1,&nop,    &illegl1,&illegl1,&lsrd,   &asld,   &tap,    &tpa,    // 0
&inx,    &dex,    &clv,    &sev,    &clc,    &sec,    &cli,    &sei,
&sba,    &cba,    &illegl1,&illegl1,&illegl1,&illegl1,&tab,    &tba,    // 1
&illegl1,&daa,    &illegl1,&aba,    &illegl1,&illegl1,&illegl1,&illegl1,
&bra,    &brn,    &bhi,    &bls,    &bcc,    &bcs,    &bne,    &beq,    // 2
&bvc,    &bvs,    &bpl,    &bmi,    &bge,    &blt,    &bgt,    &ble,
&tsx,    &ins,    &pula,   &pulb,   &des,    &txs,    &psha,   &pshb,   // 3
&pulx,   &rts,    &abx,    &rti,    &pshx,   &mul,    &wai,    &swi,
&nega,   &illegl1,&illegl1,&coma,   &lsra,   &illegl1,&rora,   &asra,   // 4
&asla,   &rola,   &deca,   &illegl1,&inca,   &tsta,   &illegl1,&clra,
&negb,   &illegl1,&illegl1,&comb,   &lsrb,   &illegl1,&rorb,   &asrb,   // 5
&aslb,   &rolb,   &decb,   &illegl1,&incb,   &tstb,   &illegl1,&clrb,
&neg_ix, &illegl1,&illegl1,&com_ix, &lsr_ix, &illegl1,&ror_ix, &asr_ix, // 6
&asl_ix, &rol_ix, &dec_ix, &illegl1,&inc_ix, &tst_ix, &jmp_ix, &clr_ix,
&neg_ex, &illegl1,&illegl1,&com_ex, &lsr_ex, &illegl1,&ror_ex, &asr_ex, // 7
&asl_ex, &rol_ex, &dec_ex, &illegl1,&inc_ex, &tst_ex, &jmp_ex, &clr_ex,
&suba_im,&cmpa_im,&sbca_im,&subd_im,&anda_im,&bita_im,&lda_im, &sta_im, // 8
&eora_im,&adca_im,&ora_im, &adda_im,&cpx_im ,&bsr,    &lds_im, &sts_im,
&suba_di,&cmpa_di,&sbca_di,&subd_di,&anda_di,&bita_di,&lda_di, &sta_di, // 9
&eora_di,&adca_di,&ora_di, &adda_di,&cpx_di ,&jsr_di, &lds_di, &sts_di,
&suba_ix,&cmpa_ix,&sbca_ix,&subd_ix,&anda_ix,&bita_ix,&lda_ix, &sta_ix, // A
&eora_ix,&adca_ix,&ora_ix, &adda_ix,&cpx_ix ,&jsr_ix, &lds_ix, &sts_ix,
&suba_ex,&cmpa_ex,&sbca_ex,&subd_ex,&anda_ex,&bita_ex,&lda_ex, &sta_ex, // B
&eora_ex,&adca_ex,&ora_ex, &adda_ex,&cpx_ex ,&jsr_ex, &lds_ex, &sts_ex,
&subb_im,&cmpb_im,&sbcb_im,&addd_im,&andb_im,&bitb_im,&ldb_im, &stb_im, // C
&eorb_im,&adcb_im,&orb_im, &addb_im,&ldd_im, &std_im, &ldx_im, &stx_im,
&subb_di,&cmpb_di,&sbcb_di,&addd_di,&andb_di,&bitb_di,&ldb_di, &stb_di, // D
&eorb_di,&adcb_di,&orb_di, &addb_di,&ldd_di, &std_di, &ldx_di, &stx_di,
&subb_ix,&cmpb_ix,&sbcb_ix,&addd_ix,&andb_ix,&bitb_ix,&ldb_ix, &stb_ix, // E
&eorb_ix,&adcb_ix,&orb_ix, &addb_ix,&ldd_ix, &std_ix, &ldx_ix, &stx_ix,
&subb_ex,&cmpb_ex,&sbcb_ex,&addd_ex,&andb_ex,&bitb_ex,&ldb_ex, &stb_ex, // F
&eorb_ex,&adcb_ex,&orb_ex, &addb_ex,&ldd_ex, &std_ex, &ldx_ex, &stx_ex
};
