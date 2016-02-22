/*
 * TaC Assembler Source Code
 *    Tokuyama kousen Educational Computer 16bit Ver.
 *
 * Copyright (C) 2008-2012 by
 *                      Dept. of Computer Science and Electronic Engineering,
 *                      Tokuyama College of Technology, JAPAN
 *
 *   上記著作権者は，Free Software Foundation によって公開されている GNU 一般公
 * 衆利用許諾契約書バージョン２に記述されている条件を満たす場合に限り，本ソース
 * コード(本ソースコードを改変したものを含む．以下同様)を使用・複製・改変・再配
 * 布することを無償で許諾する．
 *
 *   本ソースコードは＊全くの無保証＊で提供されるものである。上記著作権者および
 * 関連機関・個人は本ソースコードに関して，その適用可能性も含めて，いかなる保証
 * も行わない．また，本ソースコードの利用により直接的または間接的に生じたいかな
 * る損害に関しても，その責任を負わない．
 *
 *
 */

/*
 * lexical.h : AS--の字句解析ルーチンの外部インタフェース
 *
 * 2012.09.19           : FLAG を追加
 * 2012.08.01  v2.0     : TaC-CPU V2 対応
 * 2012.03.02           : 予約語の個数(NUM_RSV)に誤り発見
 * 2012.01.07  v0.7     : Halt 命令追加
 * 2010.07.20           : Subversion による管理を開始
 * 2009.03.11  v0.6     : 文字列の長さ上限を 127 に変更
 * 2008.07              : 初期バージョン
 *
 * $Id$
 *
 */

/*
 * 字句解析が返す値の定義
 */
#define NO       0x1001
#define LD       0x1002
#define ST       0x1003
#define ADD      0x1004
#define SUB      0x1005
#define CMP      0x1006
#define AND      0x1007
#define OR       0x1008
#define XOR      0x1009
#define ADDS     0x100a
#define MUL      0x100b
#define DIV      0x100c
#define MOD      0x100d
#define MULL     0x100e
#define DIVL     0x100f
#define SHLA     0x1010
#define SHLL     0x1011
#define SHRA     0x1012
#define SHRL     0x1013
#define JZ       0x1014
#define JC       0x1015
#define JM       0x1016
#define JO       0x1017
#define JGT      0x1018
#define JGE      0x1019
#define JLE      0x101a
#define JLT      0x101b
#define JNZ      0x101c
#define JNC      0x101d
#define JNM      0x101e
#define JNO      0x101f
#define JHI      0x1020
#define JLS      0x1021
#define JMP      0x1022
#define CALL     0x1023
#define IN       0x1024
#define OUT      0x1025
#define PUSH     0x1026
#define POP      0x1027
#define RET      0x1028
#define RETI     0x1029
#define EI       0x102a
#define DI       0x102b
#define SVC      0x102c
#define HALT     0x102d

#define isInst(x) (NO<=(x)&&(x)<=HALT)

#define EQU      0x1101
#define STR      0x1102
#define DB       0x1103
#define DW       0x1104
#define BS       0x1105
#define WS       0x1106

#define G0       0x1200  /* 以下個別レジスタ */
#define G1       0x1201
#define G2       0x1202
#define G3       0x1203
#define G4       0x1204
#define G5       0x1205
#define G6       0x1206
#define G7       0x1207
#define G8       0x1208
#define G9       0x1209
#define G10      0x120a
#define G11      0x120b
#define G12      0x120c
#define FP       0x120c
#define SP       0x120d
#define USP      0x120e
#define PC       0x120f

#define isReg(x) (((x)&0xff00)==0x1200)
#define regNo(x) ((x)&0x00ff)

#define INTEGER  0x1300
#define STRING   0x1301
#define NAME     0x1302

#define FLAG     0x1400  // FLAG は一般のレジスタとは別の扱いにする

#define MAX_NAME 127                    // 名前の長さの上限 ### 将来見直し ###

int  ln;                                // 行番号
int  val;                               // 数値を返す場合、その値
char str[MAX_NAME+1];                   // 名前を返す場合、その綴
FILE * in;                              // ソースコードを含むファイル

// 字句解析の結果を次の変数に残す
boolean  lab;                           // ラベルあり
char     label[MAX_NAME+1];             // ラベルの綴り
char     oprlab[MAX_NAME+1];            // オペランドにラベルがある場合綴り
int      op;                            // 命令
int      opr[3];                        // オペランド
boolean  imd;                           // 第2オペランドに # あり
boolean  ind;                           // 第2オペランドに @ あり
boolean  bind;                          // 第2オペランドに % あり
#define  MAX_LINE 999
char     line[MAX_LINE+1];

int  getLine();                         // 字句解析ルーチン
void rewindSrc();                       // ソースを最初に戻る
