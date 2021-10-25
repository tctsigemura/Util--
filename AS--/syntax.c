/*
 * TaC Assembler Source Code
 *    Tokuyama kousen Educational Computer 16bit Ver.
 *
 * Copyright (C) 2008-2021 by
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
 * syntax.c : AS--の構文解析ルーチン
 *
 * 2021.10.14  v4.0.0   : TaC-CPU V3 に対応
 * 2019.03.12           : ソースファイル名をパス名でも扱えるように改良
 * 2016.10.09  v3.0.0   : バージョン番号はUtil--全体で同じものを使うようにする
 * 2016.02.05  v2.1.1   : 先に BSS に置かれたものが、
 *                        後で DATA になると２重定義のエラーになるバグ訂正
 * 2012.09.28  v2.1.0   : IN/OUT 命令で OP Rd,Rx を OP Rd,%Rx と解釈する
 * 2012.09.28           : OP Rd,0,Rx を OP Rd,%Rx と解釈する
 * 2012.09.28           : DB が .o ファイルに２バイトの出力されるバグ訂正
 * 2012.09.19           : LD Rd,FLAG を追加
 * 2012.09.14           : シフトのビット数は正の値だけでよい
 *                        (シフトに限りショートイミディエイトの範囲は1〜16)
 * 2012.09.14           : ショートイミディエイト,オフセットの判定を改良
 *                        (ビット表現で同じになる正の大きな値も同じに扱う)
 * 2012.08.01  v2.0.0   : TaC-CPU V2 に対応
 * 2012.01.07  v0.7     : HALT 命令を追加, -v -h オプション追加
 * 2010.07.20           : Subversion による管理を開始
 * 2010.03.14           : 内部未定義ラベルのエラーメッセージにラベルの綴を追加
 * 2009.05.08           : 未使用変数を削除
 * 2009.04.21  v0.6.1   : fopen のモード文字列に "b" を追加(Cygwin 対応)
 * 2009.03.11  v0.6     : 文字列の 16 進表示でコードが負数の場合を改良
 * 2008.08.24  v0.5     : IN/OUT 命令の OP Rd,Rs 形式を追加
 * 2008.07.15  v0.0     : 初期バージョン
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "util.h"
#include "lexical.h"
#define  WORD      2                            // 1ワードは 2バイト

FILE *out;                                      // .o ファイル
FILE *lst;                                      // .lstファイル
int textSize;                                   // text セグメントサイズ
int strSize;                                    // string セグメントサイズ
int dataSize;                                   // data セグメントサイズ
int bssSize;                                    // bss セグメントサイズ
int symSize;                                    // 名前表サイズ
int trSize;                                     // tr セグメントサイズ
int drSize;                                     // dr セグメントサイズ

void putB(int x) {                              // 1バイト出力ルーチン
  fputc(x,out);
}

void putW(int x) {                              // 1ワード出力ルーチン
  fputc(x>>8,out);
  fputc(x,out);
}

void outHdr() {                                 // ヘッダ出力ルーチン
  fprintf(lst, "magic = %04x\n", 0x0107);
  fprintf(lst, "text  = %04x\n", textSize+strSize);
  fprintf(lst, "data  = %04x\n", dataSize);
  fprintf(lst, "bss   = %04x\n", bssSize);
  fprintf(lst, "syms  = %04x\n", symSize);
  fprintf(lst, "entry = %04x\n", 0);
  fprintf(lst, "trsize= %04x\n", trSize);
  fprintf(lst, "drsize= %04x\n", drSize);
  putW(0x0107);
  putW(textSize+strSize);
  putW(dataSize);
  putW(bssSize);
  putW(symSize);
  putW(0);
  putW(trSize);
  putW(drSize);
}

/* 名前表 */
#define TBL_SIZ  3000                           // 名前表の大きさ
#define STR_SIZ  12000                          // 文字列表の大きさ(最大16k)

struct SymTbl {                                 // 名前表のエントリ
  int name;                                     //   文字列表の idx
  int type;                                     //   type の意味は下に #define
  int val;                                      //   名前の値
};

/* SymTbl.type の値 */
#define SYMUNDF 0                               // 未定義ラベル
#define SYMTEXT 1                               // TEXTのラベル
#define SYMDATA 2                               // DATAのラベル
#define SYMBSS  3                               // BSSのラベル
#define SYMEQU  8                               // EQUのラベル(出力しない)
#define SYMSTR  16                              // STRINGのラベル(後にTEXT扱い)

char strTbl[STR_SIZ];                           // 文字列表
int  strIdx = 0;                                // 表のどこまで使用したか

struct SymTbl symTbl[TBL_SIZ];                  // 名前表の定義
int symIdx = 0;                                 // 表のどこまで使用したか
int equIdx = 0;                                 // EQUラベルの使用範囲

void outStrTbl() {                              // 文字列表を lst と out へ出力
  int offs = 0;                                 //   出力した文字数
  fprintf(lst, "Offset\tString\n");             //   ヘッダ "Offset  String"
  for (int i=equIdx; i<symIdx; i=i+1) {         //   EUQ 以外の全ラベルについて
    int j = symTbl[i].name;
    fprintf(lst, "%04x\t",offs);
    while (strTbl[j]!='\0') {                   //   一つの文字列を出力する
      fprintf(lst, "%c",strTbl[j]);
      putB(strTbl[j]);
      offs = offs + 1;
      j    = j    + 1;
    }
    fprintf(lst, "\n");                         // 文字列の終端
    putB('\0');
    offs = offs + 1;
  }
}

int strLen(int n) {                             // 文字列表中の文字列(n)の長さ
  int i = n;
  while(strTbl[i]!='\0')
    i = i + 1;
  return i - n + 1;                             // '\0' も数えた値を返す
}

void addStr(char *name) {                       // 文字列表に name を追加する
  for (int i=0; name[i]!='\0'; i=i+1) {
    if (strIdx>=STR_SIZ-1) error("文字列表がパンクした");
    strTbl[strIdx] = name[i];
    strIdx = strIdx + 1;
  }
  strTbl[strIdx] = '\0';
  strIdx = strIdx + 1;
}

boolean cmpStr(int n, char *str) {              // 文字列表[n]〜と str の比較
  for (int i=0; ; i=i+1) {
    char t = strTbl[n+i];
    char s = str[i];
    if (t!=s) return false;                     //   異なる
    if (t=='\0') break;
  }
  return true;                                  //   同じ
}

int srcName(char *name, boolean claim) {        // 名前表から name を探す
  for (int i=0; i<symIdx; i=i+1) {              //   名前表の全エントリについて
    if (cmpStr(symTbl[i].name,name)) {          //     文字列表と比較する
      return i;                                 //       見つかったら番号を返す
    }
  }
  if (claim) error("未定義ラベル");             //   claim ならエラーで止まる
  return -1;                                    //   見つからない
}

int addName(char *name) {                       // 名前表に新しい項目を作る
  if (symIdx>=TBL_SIZ) error("名前表がパンク");
  symTbl[symIdx].name = strIdx;                 //   名前表に場所を記録して
  addStr(name);                                 //   文字列表に name を登録する
  symTbl[symIdx].type = SYMUNDF;                //   名前表の初期値は  未定義
  symTbl[symIdx].val   = 0;                     //                     値 0
  symIdx = symIdx + 1;
  return symIdx - 1;                            // 登録した位置を返す
}

void defName(int val, int type) {                // ラベルを名前表に定義する
  int n = srcName(label, false);                // 既にラベルを検索し
  if (n<0) n=addName(label);                    // 見つからないならラベルを作る

  int stype = symTbl[n].type;
  int sval  = symTbl[n].val;
  if (stype==SYMUNDF) {                          // 作ったばかりか未定義ならOK
  } else if (stype==SYMBSS && type==SYMBSS) {   // BSS と BSS なら
    if (sval>val) val = sval;                   //   大きい領域に統合する
  } else if (stype==SYMBSS && type==SYMDATA) {  // BSS と DATA なら定義し直す
  } else if (stype==SYMDATA && type==SYMBSS) {  // DATA と BSS なら
    type = stype;                               //   type も
    val = sval;                                 //     val も変更しない
  } else error("ラベルの2重定義");              // どれでもなければ２重定義

  symTbl[n].type = type;
  symTbl[n].val  = val;
}

int chkName(int opr) {                          // ペランドはラベルか調べる
  int inc = 0;                                  // (オペランド解析時に使用)
  if (opr==NAME) {                              //   オペランドは名前だ
    int n = srcName(oprlab, false);             //   その名前は定義されているか
    if (n<0) n= addName(oprlab);                //   未定義ラベルなら登録
    if (symTbl[n].type!=SYMEQU) inc = 2*WORD;   //   定数以外なら再配置表拡大
  }
  return inc;                                   //   再配置表の拡大バイト数
}

void printSymType(int type) {                   // ラベルの種類を印刷する
  if (type==SYMTEXT) fprintf(lst, "TEXT");      //   = 1 は TEXT
  else if (type==SYMDATA) fprintf(lst, "DATA"); //   = 2 は DATA
  else if (type==SYMBSS)  fprintf(lst, "BSS");  //   = 3 は BSS
  else if (type==SYMUNDF) fprintf(lst, "UNDF"); //   = 0 は未定義
  else error("バグ");                           //   どれでもないならバグ？
}

void outSymTbl() {                              // 名前表を lst と out へ出力
  fprintf(lst, "No.\tName\tType\tValue\tOffset\n");
  int sIdx = 0;                                 //   out へ出力した文字数
  int idx  = 0;                                 //   out へ出力した項目数
  for (int i=equIdx; i<symIdx; i=i+1) {         //   EQUラベル後の全てについて
    char* name = &(strTbl[symTbl[i].name]);
    int   type = symTbl[i].type;
    int   val  = symTbl[i].val;
    if (name[0]=='.' && type==SYMUNDF) {        //   ファイル内ローカルラベルが
      fprintf(stderr, "%s : ", name);           //     完結していない
      error("内部未定義ラベル");
    }
    fprintf(lst, "%d\t%s\t",idx,name);
    if (type==SYMSTR) {                         //   STRING セグメントは
      type = SYMTEXT;                           //    実は TEXT セグメントの一部
      val  = val + textSize;
    }
    printSymType(type);                         //   ラベルの種類を印刷
    fprintf(lst, "\t%04x\t%04x\n", val&0xffff, sIdx&0x3fff);
    if (sIdx>=0x3fff) error("文字列表がパンクした");
    putW((type<<14) | sIdx);                    //   名前表を出力
    putW(val);
    sIdx = sIdx + strLen(symTbl[i].name);       //   名前の長さだけ文字数を増加
    idx  = idx  + 1;                            //   名前表の項目数を増加
  }
}

/* 再配置表 */
#define REL_SIZ  6000                           // 再配置表の大きさ

struct Reloc {                                  // 再配置表エントリ
  int addr;                                     //  ポインタのセグメント内 Offs
  int sym;                                      // シンボルテーブル上の番号
};

struct Reloc relTbl[REL_SIZ];                   // 再配置表の定義
int relIdx = 0;                                 // 表のどこまで使用したか
int drIdx = 0;                                  // Dr領域の開始位置

void defRel(int addr, int sym) {                // 再配置表に登録
  if (relIdx>=REL_SIZ) error("再配置表がパンクした");
  relTbl[relIdx].addr = addr;
  relTbl[relIdx].sym  = sym;
  relIdx = relIdx + 1;
}

void endTr() {                                  // Tr領域を終える、以降はDr領域
  drIdx = relIdx;
}

void outRelTbl(int s, int e) {                  // 再配置表の指定範囲を出力
  fprintf(lst, "Addr\tName\tType\tNo.\n");
  for (int i=s; i<e; i=i+1) {
    int addr = relTbl[i].addr;
    int sym  = relTbl[i].sym;
    int name = symTbl[sym].name;
    int type = symTbl[sym].type;
    fprintf(lst, "%04x\t%s\t",addr,&(strTbl[name]));
    if (type==SYMSTR) type = SYMTEXT;           // STRINGは実はTEXTの一部
    printSymType(type);                         // ラベルの種類を印刷
    sym = sym - equIdx;                         // EQUラベルは出力されないので
    fprintf(lst, "\t%3d\n", sym);
    if ((addr & 1)!=0) error("as--のバグ(1)");
    putW(addr);                                 // 再配置情報の出力
    putW((type<<14) | sym);
  }
}

/* 命令表 */  
struct InstTbl {                                // 命令表のエントリ
  int tok;                                      //   トークン
  int code;                                     //   機械語命令の場合オペコード
  int type;                                     //   命令の種類
};

/* InstTbl.type の値 */
#define M1 1    // NO,RET,RETI,SVC,HALT
#define M2 2    // PUSH,POP
#define M3 3    // IN,OUT
#define M4 4    // LD,ADD,SUBZ,CMP,AND,OR,XOR,ADDS,MUL,DIV,MOD
#define M5 5    // ST
#define M6 6    // JZ,JC,JM,JO,JGT,JGE,JLE,JLT,JNZ,JNC,JNM,JNO,JHI,JLS,JMP,CALL
#define M7 7    // SHLA,SHLL,SHRA,SHRL
#define P1 8    // EQU, DB, DW, BS, WS
#define P2 9    // STRING

struct InstTbl instTbl[] = {
  {NO,  0x0000, M1}, {LD,  0x0800, M4}, {ST,  0x1000, M5}, {ADD, 0x1800, M4},
  {SUB, 0x2000, M4}, {CMP, 0x2800, M4}, {AND, 0x3000, M4}, {OR,  0x3800, M4},
  {XOR, 0x4000, M4}, {ADDS,0x4800, M4}, {MUL, 0x5000, M4}, {DIV, 0x5800, M4},
  {MOD, 0x6000, M4},
  {SHLA,0x8000, M7}, {SHLL,0x8800, M7}, {SHRA,0x9000, M7}, {SHRL,0x9800, M7},
  {JZ,  0xA000, M6}, {JC,  0xA010, M6}, {JM,  0xA020, M6}, {JO,  0xA030, M6},
  {JGT, 0xA040, M6}, {JGE, 0xA050, M6}, {JLE, 0xA060, M6}, {JLT, 0xA070, M6}, 
  {JNZ, 0xA080, M6}, {JNC, 0xA090, M6}, {JNM, 0xA0A0, M6}, {JNO, 0xA0B0, M6},
  {JHI, 0xA0C0, M6},                    {JLS, 0xA0E0, M6}, {JMP, 0xA0F0, M6},
  {CALL,0xA800, M6}, {IN,  0xB000, M3}, {OUT, 0xB800, M3}, {PUSH,0xC000, M2},
  {POP, 0xC400, M2}, {RET, 0xD000, M1}, {RETI,0xD400, M1}, 
  {SVC, 0xF000, M1}, {HALT,0xFF00, M1},
  {EQU, 0x0000, P1}, {STR, 0x0000, P2},
  {DB,  0x0000, P1}, {DW,  0x0000, P1}, {BS,  0x0000, P1}, {WS,  0x0000, P1},
  {0,   0     , 0 }
};

struct InstTbl *srcInst(int tok) {              // 命令表の探索
  for (int i=0; instTbl[i].tok!=0; i++)
    if (instTbl[i].tok==tok) return &instTbl[i];
  error("命令がみつからない");
  return NULL;
}

void chkReg() {                                 // 第１オペランドはレジスタか
  if (!isReg(opr[0])) error("第１オペランドに汎用レジスタがない");
}

void chkOpr() {                                 // 全オペランドを使用したか
  if (imd) error("イミディエイトは使用できない");
  if (ind || bind) error("インダイレクトは使用できない");
  if (opr[0]!=0 || opr[1]!=0 || opr[2]!=0)
    error("オペランドに矛盾がある");
}

boolean isShrtImd(int tok) {                    // 4bit 定数で表現できるか
  int v = val & 0xfff8;
  if (tok==INTEGER && (v==0xfff8 || v==0))      //   整数の -8〜+7 なら
    return true;                                //     表現できる
  if (tok==NAME) {                              //   ラベルの場合は
    int n=srcName(oprlab, false);               //    名前表を探索し
    if (n>=0) {                                 //    見つかったなら 
      struct SymTbl s = symTbl[n];              //     EQU ラベルで -8〜+7 の
      int w = s.val & 0xfff8;                   //      ものを認める
      if (s.type==SYMEQU &&                     //      (他にも可能な場合が
	  (w==0xfff8 || w==0)) return true;     //        あるけど、C--に不要)
    }
  }
  return false;
}

boolean isShrtOfs(int tok) {                    // 4bit 定数で表現できるか
  int v = val & 0xfff1;
  if (tok==INTEGER && (v==0xfff0 || v==0))      //   偶数の整数で-16〜+14 なら
    return true;                                //    表現できる
  if (tok==NAME) {                              //   ラベルの場合は
    int n=srcName(oprlab, false);               //    名前表を検索し
    if (n>=0) {                                 //    見つかったなら 
      struct SymTbl s = symTbl[n];              //     EQU ラベルで -16〜+14 の
      int w = s.val & 0xfff1;                   //      偶数を認める
      if (s.type==SYMEQU &&                     //      (他にも可能な場合が
	  (w==0xfff0 || w==0)) return true;     //        あるけど、C--に不要)
    }
  }
  return false;
}

boolean isCnst0(int tok) {                      // オペランドは定数 0 か
  if (tok==INTEGER && val==0)                   //   整数の 0 なら
    return true;                                //    表現できる
  if (tok==NAME) {                              //   ラベルの場合は
    int n=srcName(oprlab, false);               //    名前表を検索し
    if (n>=0) {                                 //    見つかったなら
      struct SymTbl s = symTbl[n];              //     EQU ラベルで値が 0 の
      if (s.type==SYMEQU && s.val==0)           //      場合が該当する
	return true;                            //
    }
  }
  return false;
}

int getVal(int tok, int addr) {                 // オペランドの値を計算する
  if (tok==INTEGER) return val;                 //   整数ならその値
  if (tok!=NAME) error("オペランドに整数もラベルもない");

  int n = srcName(oprlab, true);                //   ラベルなら名前表を探索
  int val = symTbl[n].val;                      //     見つからなければエラー

  if (symTbl[n].type==SYMSTR)                   //   STRINGセグメントの場合
    val = val + textSize;                       //     アドレスを計算する
  else if (symTbl[n].type==SYMDATA)             //   DATAセグメントの場合
    val = val + textSize + strSize;             //     アドレスを計算する
  else if (symTbl[n].type==SYMBSS)              //   BSSセグメントの場合は
    val = 0;                                    //     未定義並の扱いになる
  /* else SYMTEXT */                            //   TEXTセグメントはそのまま

  if (symTbl[n].type!=SYMEQU)                   //   EQUラベルは再配置の対象外
    defRel(addr, n);                            //     再配置表に情報を登録する
  return val;                                   //   ラベルの値を絶対番地で返す
}

void putLst0(int adr) {                         // ラベルだけの行、EQU命令の行を
  fprintf(lst, "%04x          \t%s\n",adr&0xffff,line);        // 出力する
}

void putLstA(int adr) {                         // アライメント調整の出力
  fprintf(lst, "%04x %02x\n",adr&0xffff,0);
  putB(0);
}

void putLstB(int adr, int code) {               // 1バイトデータ行の出力
  fprintf(lst, "%04x %02x       \t%s\n",adr&0xffff,code&0xff,line);
  putB(code);
}

void putLst1(int adr, int code) {               // 1ワード命令行の出力
  fprintf(lst, "%04x %04x     \t%s\n",adr&0xffff,code&0xffff,line);
  putW(code);
}

void putLst2(int adr, int code, int n) {        // 2ワード命令行の出力
  fprintf(lst, "%04x %04x %04x\t%s\n",adr&0xffff,code&0xffff,n&0xffff,line);
  putW(code);
  putW(n);
}

void putLstS(int adr, char*str) {               // STRING 命令行の出力
  int n = str[0];
  int i;
  fprintf(lst, "%04x",adr);
  for (i=1; i<4; i++) {
    fprintf(lst, " ");
    if (i<=n) fprintf(lst, "%02x",str[i]&0x00ff);
    else fprintf(lst, "  ");
  }
  fprintf(lst, "\t%s\n",line);
  
  while (i<=n) {
    int j;
    fprintf(lst, "%04x",adr+i-1);
    for (j=i; j<i+3; j++) {
      if (j>n) break;
      fprintf(lst, " %02x",str[j]&0x00ff);
    }
    i = j;
    fprintf(lst, "\n");
  }
  for (int i=1; i<=n; i=i+1)
    putB(str[i]&0x00ff);
}

void pass0() {                                  // pass0は、EQU命令だけ処理する
  for (ln=1; getLine()!=EOF; ln=ln+1) {         // EQUラベルはTR,DR,SYMへ出力し
    if (op==EQU) {                              // ないので特別な処理になる
      if (!lab) error("EQUにはラベルが必要");
      if (opr[0]!=INTEGER) error("EQUの後ろに定数がない");
      defName(val,SYMEQU);
      putLst0(val);
      opr[0] = 0;                               // 使用したオペランドはクリア
      chkOpr();                       // オペランドをすべて使ったかチェッする
    }
  }
  ln = -1;                      // 以後、エラーが発生してもリストは表示しない
}

// アドレッシングモード
#define DRCT  0x0000   // ダイレクトモード
#define INDX  0x0100   // インデクスドモード
#define IMMD  0x0200   // イミディエイトモード
#define FPRL  0x0300   // FP相対モード
#define REG   0x0400   // レジスタモード
#define SIMMD 0x0500   // ショートイミディエイト
#define INDR  0x0600   // インダイレクト
#define BINDR 0x0700   // バイトインダイレクト

int amode() {                                    // アドレッシングモードを調べる
  int mode = -1;                                 //  オペコードの変更を決める
  if (bind) {                                    //   @ 付きなら
    bind = false;                                //    バイトインダイレクト(=7)
    mode = BINDR;
  } else if (ind) {                              //   % 付きなら
    ind  = false;                                //    ワードインダイレクト(=6)
    mode = INDR;
  } else if (imd && isShrtImd(opr[1])) {         //   # の後に小さな定数なら
    imd  = false;                                //    4bit イミディエイト(=5)
    mode = SIMMD;
  } else if (isReg(opr[1])||opr[1]==FLAG) {      //   @,% なしレジスタオペランド
    mode = REG;                                  //    レジスタモード(=4)
  } else if (opr[2]==FP && isShrtOfs(opr[1])) {  //   N,FP で N が5bit以内で
    mode = FPRL;                                 //    偶数なら FP 4bit相対(=3)
  } else if (imd) {                              //   # 付きなら
    imd  = false;                                //    イミディエイト(=2)
    mode = IMMD;
  } else if (isReg(opr[2])) {                    //   第３オペランドがあれば
    mode = INDX;                                 //    インデクスド(=1)
    if (isCnst0(opr[1])) {                       //    オフセットが0なら
      mode = INDR;                               //      ワードインダイレクトに
      opr[1] = opr[2];                           //        置換える
      opr[2] = 0;                                //
    }
  } else {                                       //   それ以外は
    mode = DRCT;                                 //    ダイレクト(=0)
  }
  return mode;
}

void pass1() {                  // pass1 はラベル表を作成する
  int lc_t = 0;                 // text セグメントのロケーションカウンタ
  int lc_d = 0;                 // data セグメントのロケーションカウンタ
  int lc_s = 0;                 // string セグメントのロケーションカウンタ
  for (ln=1; getLine()!=EOF; ln=ln+1) {
    if (op==0) {                                // 命令のない行にラベルがある
      if (lab) defName(lc_t,SYMTEXT);           //   TEXTセグメントに登録
    } else {                                    // 命令がある
      struct InstTbl *inst = srcInst(op);       // 命令表を探索、
      int typ = inst->type;                     //   見つからない場合はエラー
      if (lab && typ<P1) defName(lc_t,SYMTEXT); // 機械語命令ならラベルを登録
      if (typ==M1 || typ==M2) {                 // NO,RET,RETI,HALT,
	lc_t = lc_t + WORD;                     // PUSH,POP これらは1ワード命令
      } else if (typ==M3) {                     // IN,OUT
	if (isReg(opr[1])) {                    // レジスタによるポート指定なら
	  lc_t = lc_t + WORD;                   //   1ワード命令
	} else {                                // 定数によるポート指定の場合
	  trSize = chkName(opr[1]) + trSize;    //   ラベル参照ありならTr増加
	  lc_t = lc_t + 2*WORD;                 //   2ワード命令
	}
      } else if (typ==M4 || typ==M5 ||          // LD,ADD,SUB,...,MOD,ST,
		 typ==M6 || typ==M7) {          // JMP,SHXX
	int l = 1;                              //   基本的に1ワード命令
	int mode = amode();                     //   アドレッシングモードを調べ
	if (mode==DRCT || mode==INDX ||         //   これら３種類なら
	    mode==IMMD) {                       //     2ワード命令
	  l = 2;                                //     このアドレッシングなら
	  trSize = chkName(opr[1]) + trSize;    //     ラベル参照によりTr増加
	}                                       //   例外的にシフト
	if (typ==M7 && mode==IMMD) l = 1;       //     イミディエイトは1ワード
	lc_t = lc_t + l*WORD;                   //   1 または 2 ワード命令
      } else if (typ==P1) {                     // アセンブラ命令
	if (op==BS || op==WS) {                 //  BS, WS 命令
	  if (opr[0]!=INTEGER) error("BS/WS命令の後ろに定数がない");
	  if (!lab) error("BS/WSにはラベルが必要");
	  if (val<=0) error("BS/WSの領域サイズが不正");
	  if (op==WS) val = val * WORD;         //   WS ならバイト単位に変換
	  else val = (val + 1) & ~1;            //   BS ならアライメント調整
	  defName(val,SYMBSS);                  //   BS, WS 命令のラベルを登録
	  putLst0(val);                         //  ここでリスト出力する
	} else if (op==DB || op==DW) {          //  DB, DW 命令
	  if (op==DW) lc_d = (lc_d + 1) & ~1;   //   アライメント調整
	  if (lab) defName(lc_d,SYMDATA);       //   DB, DW 命令のラベルを登録
	  int inc = chkName(opr[0]);            //   ラベル参照によりDr増加
	  drSize = drSize + inc;
	  if (op==DB) {
	    if (inc!=0) error("バイトデータにアドレスは格納できない");
	    lc_d = lc_d + 1;                    //   DB命令は1バイト命令
	  } else
	    lc_d = lc_d + WORD;                 //   DW命令は1ワード命令
	}
      } else if (inst->type==P2) {              //  STRING命令
	if (lab) defName(lc_s,SYMSTR);          //    STRING命令のラベルを登録
	if (opr[0]!=STRING) error("STRINGの後ろに文字列がない");
	lc_s = lc_s + str[0];
      } else error("バグ？");
    }
  }
  ln = -1;                      // 以後、エラーが発生してもリストは表示しない
  textSize = lc_t;              // 各セグメントの大きさを確定する
  dataSize = (lc_d + 1) & ~1;
  strSize  = (lc_s + 1) & ~1;
  for (int i=0; i<symIdx; i=i+1) {
    if (symTbl[i].type==SYMBSS)
      bssSize = bssSize + symTbl[i].val;
  }
  symSize  = (symIdx - equIdx) * 2 * WORD;
}

void pass2() {                  // pass2は、機械語命令だけ処理する
  int lc_t = 0;                 // text セグメントのロケーションカウンタ
  for (ln=1; getLine()!=EOF; ln=ln+1) {
    if (op==0) {                                 // 空行の場合は
      if (lab) putLst0(lc_t);                    //   ラベルのある行だけ出力
    } else {                                     // 命令がある場合は
      struct InstTbl *inst = srcInst(op);        //   命令表を検索
      int type = inst->type;
      int code = inst->code;
      if (type==M1) {                            // NO,RET,RETI,HALT命令
        if (op==RETI) code = code | 0x00f0;      // RETIのRdは0xf
	putLst1(lc_t, code);                     // オペランドの無い1ワード命令
	lc_t = lc_t + WORD;
      } else if (type==M2) {                     // PUSH,POP 命令
	chkReg();                                // 第１オペランドはレジスタか
	putLst1(lc_t, code|(regNo(opr[0])<<4));
	opr[0] = 0;                              // 使用したオペランドはクリア
	lc_t = lc_t + WORD;
      } else if (type==M3) {                     // IN,OUT 命令
	chkReg();                                // 第１オペランドはレジスタか
	code = code | (regNo(opr[0])<<4);
	int mode =amode();
	if (mode==REG) mode = INDR;              // % なしの表記も認める
	if (mode==DRCT) {                        // ダイレクトモードなら
	  int a = getVal(opr[1],lc_t+WORD);      //   第2オペランドの値を解決
	  putLst2(lc_t, code, a);                //   2ワード命令として出力
	  lc_t = lc_t + 2*WORD;
	} else if (mode==INDR || mode==BINDR) {  // インダイレクトなら
	  code = code | mode | regNo(opr[1]);    //   レジスタを含む命令コード
	  putLst1(lc_t, code);                   //   1ワード命令として出力
	  lc_t = lc_t + WORD;
	} else {
	  error("IN/OUT命令で使用できないアドレッシングモード");
	}
	opr[0] = opr[1] = 0;                     // 使用したオペランドはクリア
      } else if (type==M4 || type==M5 ||         // LD,ADD,SUB,...,MOD,ST,
		 type==M6 || type==M7) {         // JMP,SHXX
	chkReg();                                // 第１オペランドはレジスタか
	int mode =amode();
	if (type==M5 && (mode==REG||mode==IMMD||mode==SIMMD))
	  error("ST命令で使用できないアドレッシングモード");
	if (type==M6 && mode!=DRCT && mode!=INDX)
	  error("JMP/CALL命令で使用できないアドレッシングモード");
	if (type==M7 && mode==IMMD) mode=SIMMD;  // シフト命令で16以上は無意味
	if (type==M7 && mode==SIMMD) {           //   SIMMD で足りるはず
	  int v = getVal(opr[1],lc_t+WORD) - 1;  // シフトビット数の範囲は
	  if ((v & 0xfff0) != 0)                 //   1 〜 16 がのはず
	    error("シフトのビット数が範囲外");
	}
	code = code | (regNo(opr[0])<<4);
	if (FPRL<=mode) {                        // 1ワード命令なら
	  int rx = 0;
	  if (mode==SIMMD) {                     //   4bit整数を使用するなら
	    rx = getVal(opr[1],lc_t+WORD) & 0xf;
	  } else if (mode==FPRL) {               //   4bit相対を使用するなら
	    rx = (getVal(opr[1],lc_t+WORD)>>1) & 0xf;
	  } else {                               //   レジスタ
	    rx = regNo(opr[1]);
          }
	  putLst1(lc_t, code|mode|rx);           //   1ワード命令として出力
	  if (mode==FPRL) opr[2] = 0;            //   使用オペランドをクリア
	  lc_t = lc_t + WORD;
	} else {                                 // 2ワード命令なら
	  int a = getVal(opr[1], lc_t+WORD);     //  第２ワードを決定
	  int rx = 0;                            //  Rx フィールド
	  if (mode==INDX) {                      //  インデクスドモードなら
	    rx = regNo(opr[2]);                  //    Rx フィールドを決定
	    opr[2] = 0;                          // 使用したオペランドはクリア
	  }
	  putLst2(lc_t, code|mode|rx, a);        //  2ワード命令として出力
	  lc_t = lc_t + 2*WORD;
	}
	opr[0] = opr[1] = 0;                     // 使用したオペランドはクリア
      } else {
	continue;                                // 機械語命令以外はスキップ
      }
    }
    chkOpr();                   // オペランドはすべて使ったかチェッする
  }
  ln = -1;                      // 以後、エラーが発生してもリストは表示しない
}

void pass3() {                  // pass3は、STRING命令だけ処理する
  int lc_s = 0;                 // STRING セグメントのロケーションカウンタ
  for (ln=1; getLine()!=EOF; ln=ln+1) {
    if (op==STR) {                               // STRING 命令がある
      putLstS(lc_s+textSize,str);                // 文字列を出力
      opr[0] = 0;                                // 使用したオペランドはクリア
      lc_s = lc_s + str[0];                      // str[0]は文字列の長さ(getStr)
      chkOpr();                 // オペランドをすべて使ったかチェッする
    }
  }
  if ((lc_s & 1)!=0) {          // 奇数アドレスなら
    putLstA(lc_s+textSize);
    lc_s = lc_s + 1;
  }
}

void pass4() {                  // pass4は、DB,DW命令だけ処理する
  int lc_d = 0;                 // DATA セグメントのロケーションカウンタ
  for (ln=1; getLine()!=EOF; ln=ln+1) {
    if (op==DB) {                                // DB 命令がある
      int a = getVal(opr[0],lc_d);               // 第1オペランドの値を解決する
      opr[0] = 0;                                // 使用したオペランドはクリア
      putLstB(lc_d+textSize+strSize,a);          // 1バイトのデータを出力
      lc_d = lc_d + 1;
      chkOpr();                 // オペランドをすべて使ったかチェッする
    } else if (op==DW) {                         // DW 命令がある
      if ((lc_d & 1)!=0) {                       //   奇数アドレスなら
	putLstA(lc_d+textSize+strSize);          //     1バイト出力
	lc_d = lc_d + 1;
      }
      int a = getVal(opr[0],lc_d);               // 第1オペランドの値を解決する
      opr[0] = 0;                                // 使用したオペランドはクリア
      putLst1(lc_d+textSize+strSize,a);          // 1ワードのデータを出力
      lc_d = lc_d + WORD;
      chkOpr();                 // オペランドをすべて使ったかチェッする
    }
  }
  if ((lc_d & 1)!=0) {          // 奇数アドレスなら
    putLstA(lc_d+textSize+strSize);
    lc_d = lc_d + 1;
  }
  ln = -1;                      // 以後、エラーが発生してもリストは表示しない
}

#define MAX_PATH 30

// 使い方表示関数
void usage(char *name) {
  fprintf(stderr, "使用方法 : %s [-h] [-v] [<source file>]\n", name);
  fprintf(stderr, "    <source file> は AS--言語ソースコード\n");
  fprintf(stderr, "    <source file> の拡張子は必ず '.s'\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "    -h, -v  : このメッセージを表示\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "%s version %s\n", name, VER);
  fprintf(stderr, "(build date : %s)\n", DATE);
  fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
  ln = -1;                        // ソースを読むまでは、エラー時リスト表示なし

  if (argc>1 &&
      (strcmp(argv[1],"-v")==0 ||              //  "-v", "-h" で、使い方と
       strcmp(argv[1],"-h")==0   ) ) {         //   バージョンを表示
    usage(argv[0]);
    exit(0);
  }

  if (argc!=2) {
    usage(argv[0]);
    exit(1);
  }

  int e = strlen(argv[1]);                     // パス名の最後
  int d = e - 1;                               // ファイル名と拡張子の境界
  while (d>=0 && argv[1][d]!='.') {
    d = d - 1;
  }
  if (e-d!=2 || argv[1][d]!='.' || argv[1][d+1]!='s' || argv[1][d+2]!='\0')
    error("ファイル名の拡張子は '.s' でなければならない");

  int  f = d - 1;                              // ファイル名の先頭
  while (f>=0 && argv[1][f]!='/') {
    f = f - 1;
  }
  f = f + 1;

  char filePath[MAX_PATH+3];
  if (e>MAX_PATH) error("パス名が長すぎる");
  strcpy(filePath, argv[1]);

  if ((in = fopen(filePath,"r"))==NULL) {      // ソースファイルオープン
    perror(filePath);
    exit(1);
  }

  filePath[d] = '\0';
  strcat(filePath, ".o");                      // 拡張子は '.o'
  if ((out=fopen(filePath,"wb"))==NULL) {      // オブジェクトファイルオープン
    perror(filePath);
    exit(1);
  }

  filePath[d] = '\0';
  strcat(filePath, ".lst");                    // 拡張子は '.o'
  if ((lst = fopen(filePath,"w"))==NULL) {     // リストファイルオープン
    perror(filePath);
    exit(1);
  }

  fprintf(lst, "*** 定数 ***\n");
  rewindSrc();                                 // 最初の1文字を読み込む
  pass0();                                     // ラベル表(symTbl)にEQUを登録
  equIdx = symIdx;                             // これ以前はEQUラベル

  strcpy(filePath, "@");                       // 名前表に登録する
  strcat(filePath, argv[1]+f);                 //  ファイル名は"@"で始まる
  int n = addName(filePath);                   // 名前表にファイル名を登録
  symTbl[n].type = SYMTEXT;

  fprintf(lst, "\n*** BSSセグメント ***\n");
  rewindSrc();                                 // 最初の1文字を読み込む
  pass1();                                     // ラベル表(symTbl)を完成する

  fprintf(lst, "\n*** ヘッダ ***\n");
  outHdr();                                    // ヘッダを出力する

  fprintf(lst, "\n*** TEXTセグメント ***\n");
  rewindSrc();                                 // 最初に戻って1文字を読み込む
  pass2();                                     // TEXTセグメントに機械語を出力

  rewindSrc();                                 // 最初に戻って1文字を読み込む
  pass3();                                     // TEXTセグメントに文字列を出力

  endTr();                                     // TRの登録を終了する

  fprintf(lst, "\n*** DATAセグメント ***\n");
  rewindSrc();                                 // 最初に戻って1文字を読み込む
  pass4();                                     // DATAセグメントを出力する

  fprintf(lst, "\n*** 再配置表 ***\n");        // 再配置表を出力する
  fprintf(lst, "TR領域\n");
  outRelTbl(0,drIdx);                          // TR領域の出力
  fprintf(lst, "\nDR領域\n");
  outRelTbl(drIdx,relIdx);                     // DR領域の出力

  fprintf(lst, "\n*** 名前表 ***\n");
  outSymTbl();                                 // 名前表を出力する

  fprintf(lst, "\n*** 文字列表 ***\n");
  outStrTbl();                                 // 文字列表を出力する

  fclose(in);
  fclose(out);

  return 0;
}
