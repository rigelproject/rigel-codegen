	.text
_start:
	addsubps 0x12345678,%xmm1
	comisd 0x12345678,%xmm1
	comiss 0x12345678,%xmm1
	cvtdq2pd 0x12345678,%xmm1
	cvtpd2dq 0x12345678,%xmm1
	cvtps2pd 0x12345678,%xmm1
	cvttps2dq 0x12345678,%xmm1
	haddps 0x12345678,%xmm1
	movdqu %xmm1,0x12345678
	movdqu 0x12345678,%xmm1
	movhpd %xmm1,0x12345678
	movhpd 0x12345678,%xmm1
	movhps %xmm1,0x12345678
	movhps 0x12345678,%xmm1
	movlpd %xmm1,0x12345678
	movlpd 0x12345678,%xmm1
	movlps %xmm1,0x12345678
	movlps 0x12345678,%xmm1
	movshdup 0x12345678,%xmm1
	movsldup 0x12345678,%xmm1
	pshufhw $0x90,0x12345678,%xmm1
	pshuflw $0x90,0x12345678,%xmm1
	punpcklbw 0x12345678,%mm1
	punpckldq 0x12345678,%mm1
	punpcklwd 0x12345678,%mm1
	punpcklbw 0x12345678,%xmm1
	punpckldq 0x12345678,%xmm1
	punpcklwd 0x12345678,%xmm1
	punpcklqdq 0x12345678,%xmm1
	ucomisd 0x12345678,%xmm1
	ucomiss 0x12345678,%xmm1

	cmpeqsd (%eax),%xmm0
	cmpeqss (%eax),%xmm0
	cvtpi2pd (%eax),%xmm0
	cvtpi2ps (%eax),%xmm0
	cvtps2pi (%eax),%mm0
	cvtsd2si (%eax),%eax
	cvtsd2ss (%eax),%xmm0
	cvtss2sd (%eax),%xmm0
	cvtss2si (%eax),%eax
	divsd (%eax),%xmm0
	divss (%eax),%xmm0
	maxsd (%eax),%xmm0
	maxss (%eax),%xmm0
	minss (%eax),%xmm0
	minss (%eax),%xmm0
	movntsd %xmm0,(%eax)
	movntss %xmm0,(%eax)
	movsd (%eax),%xmm0
	movsd %xmm0,(%eax)
	movss (%eax),%xmm0
	movss %xmm0,(%eax)
	mulsd (%eax),%xmm0
	mulss (%eax),%xmm0
	rcpss (%eax),%xmm0
	roundsd $0,(%eax),%xmm0
	roundss $0,(%eax),%xmm0
	rsqrtss (%eax),%xmm0
	sqrtsd (%eax),%xmm0
	sqrtss (%eax),%xmm0
	subsd (%eax),%xmm0
	subss (%eax),%xmm0

	pmovsxbw (%eax),%xmm0
	pmovsxbd (%eax),%xmm0
	pmovsxbq (%eax),%xmm0
	pmovsxwd (%eax),%xmm0
	pmovsxwq (%eax),%xmm0
	pmovsxdq (%eax),%xmm0
	pmovzxbw (%eax),%xmm0
	pmovzxbd (%eax),%xmm0
	pmovzxbq (%eax),%xmm0
	pmovzxwd (%eax),%xmm0
	pmovzxwq (%eax),%xmm0
	pmovzxdq (%eax),%xmm0
	insertps $0x0,(%eax),%xmm0

	.intel_syntax noprefix
	cvtss2si eax,DWORD PTR [eax]
	cvtsd2si eax,QWORD PTR [eax]
