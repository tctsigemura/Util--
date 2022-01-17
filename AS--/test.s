;; equ と整数
x	equ	8		; 正の数
y	equ	-8		; 負の数
z	equ	0x1234		; 16進
v	equ	0123		; 8進
;zz	equ
;	equ	123
	;;

;; DATA セグメント
a	dw	x		; 定数
b	db	-0x1234		; 負の16進
c	db	-0123		; 負の8進
	db	'a'		; 文字
	db	'\n'		; 制御文字
	db	'\t'		; 制御文字
	db	'\r'		; 制御文字
	db	'\t'		; 制御文字
	db	'\x12'		; 16進
	db	'\12'		; 8進
d	dw	'\"'		; エスケープ+アライメント
	db	'\''		; エスケープ
	dw	e		; drが増加
;	db	e		; エラーになる
;; ここに１バイトアライメント調整

;; BSS
e	ws	2
f	bs	5
g	ws	5
;	ws	5
;h	bs	-1
h	bs	1

;; ストリングセグメント
xyz	string  "abcd"
xzz	string  "xyz"
zzz	string  "xyz"

;; テキストセグメント
start	no
	ld	g0,g1		; レジスタ、レジスタ
	ld	g0,a		; ダイレクト
	ld	g1,b,g0  	; インデクスド
	ld	g2,b,g1
	ld	g3,b,g2
	ld	g4,b,g3
	ld	g5,b,g4
	ld	g6,b,g5
	ld	g7,b,g6
	ld	g8,b,g7
	ld	g9,b,g8
	ld	g10,b,g9
	ld	g11,b,g10
	ld	g12,b,g11
	ld	fp,b,g12
	ld	sp,b,fp
	ld	usp,b,sp
	ld	flag,b,usp
	ld	g1,b,flag
	ld	g1,#-9		; イミディエイト
	ld	g1,#65527	; = -9
	ld	g1,#-8
	ld	g1,#65528	; = -8
	ld	g1,#7
	ld	g1,#8
	ld	g1,-17,g12	; FP相対
	ld	g1,-16,g12
	ld	g1,-15,g12
	ld	g1,14,fp
	ld	g1,15,fp
	ld	g1,16,fp
	ld	g1,%g2		; インダイレクト
	ld	g1,0,g2		; インダイレクトと同じ
	ld	g1,@g2		; バイトインダイレクト
	ld	g1,flag         ; flag も汎用レジスタ
	add	g1,flag		; 
	ld	flag,g1		;
;; 
;	st	g0,g1		; レジスタ、レジスタ
	st	g2,a		; ダイレクト
	st	g2,b,g3  	; インデクスド
;	st	g2,#-9		; イミディエイト
;	st	g2,#-8
;	st	g2,#7
;	st	g2,#8
	st	g2,-17,g12	; FP相対
	st	g2,-16,g12
	st	g2,-15,g12
	st	g2,14,fp
	st	g2,15,fp
	st	g2,16,fp
	st	g2,%g3
	st	g2,0,g3
	st	g2,@g3
;; 
	add	g1,g2
	sub	g2,g3
	cmp	g3,g4
	and	g4,g5
	or	g5,g6
	xor	g6,g7
	adds	g7,g8
	mul	g9,g10
	div	g11,g12
	mod	g12,fp
	shla	sp,usp
	shll	usp,flag
	shra	flag,g0
	shrl	g0,g1
;	shla	g1,#0
	shla	g1,#8
	shla	g1,#16
;	shla	g1,#17
;	shla	g1,#-8
;	shla	g1,#-9
;;
loop	jz	loop
	jc	loop,g1
	jm	loop
	jo	loop
	jgt	loop
	jge	loop
	jle	loop
	jlt	loop
	jnz	loop
	jnc	loop
	jnm	loop
	jno	loop
	jhi	loop
	jls	loop
	jmp	loop
	jmp	0,g0
	jmp	%g0
	call	printf
	call	printf,flag
	call	0,g0
end
	in	g0,012
	in	g1,%g1
	in	g1,g1
	in	g2,@g1
	out	g2,0x11
	out	g1 , % g5
	out	g1 ,g5
	out	g0,@g7
	push	g0
	push	flag
	pop	flag
	pop	g0
	ret
	reti
	svc
	halt
