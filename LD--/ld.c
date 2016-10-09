/*
 * TaC linker Source Code
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
 * ld.c : LD-- 本体
 *
 * 2016.03.12           : 使用方法メッセージ表示用のオプション -h, -v 追加
 * 2012.08.02  v.2.0    : TaC-CPU V2 に対応
 * 2010.12.07           : 1. Usage メッセージがバージョン情報を含むように改良
 *                      : 2. 次のバグを訂正
 *                      :   (1) 綴を共有したローカルシンボルをもつ .o ファイル
 *                      :       を更に新しい .o のローカルシンボルと綴を共有
 *                      :       するときに綴を間違える mergeStrTbl のバグ
 *             v.0.0.4  :   (2) PTR 連鎖を一段しかたどらない readRel のバグ
 * 2010.07.20  v.0.0.3  : Subversion による管理を開始
 * 2009.08.01  v.0.0.2  : 表がパンクしたとき、各表の込み具合を表示する
 * 2009.04.21  v.0.0.1  : バイナリファイルの fopen のモードに "b" を追加
 * 2008.07.08  v.0.0    : 開発開始
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define boolean int                        // boolean 型のつもり
#define true     1
#define false    0
#define WORD     2                         // 1ワード2バイト
#define MAGIC    0x0107                    // .o 形式のマジック番号
#define HDRSIZ   16                        // .o 形式のヘッダーサイズ

int cTextSize;                             // 現在の入力ファイルのTEXTサイズ
int cDataSize;                             // 現在の入力ファイルのDATAサイズ
int cBssSize;                              // 現在の入力ファイルのBSS サイズ
int cSymSize;                              // 現在の入力ファイルのSYMSサイズ
int cTrSize;                               // 現在の入力ファイルのTr  サイズ
int cDrSize;                               // 現在の入力ファイルのDr  サイズ
int textBase;                              // 現在の入力ファイルの
int dataBase;                              //   各セグメントの
int bssBase;                               //     ロードアドレス

int textSize;                              // 出力ファイルのTEXTサイズ
int dataSize;                              // 出力ファイルのDATAサイズ
int bssSize;                               // 出力ファイルのBSS サイズ
int symSize;                               // 出力ファイルのSYMSサイズ
int trSize;                                // 出力ファイルのTr  サイズ
int drSize;                                // 出力ファイルのDr  サイズ

void error(char *str) {                   // エラーメッセージを表示して終了
  fprintf(stderr, "%s\n", str);
  exit(1);
}

void tblError(char *str);

// ファイル関係
FILE* out;                                 // 出力ファイル
FILE* in;                                  // 入力ファイル
char *curFile = "";                        // 現在の入力ファイル

#define getB()    fgetc(in)
#define putB(c)   fputc(c,out)

void fError(char *str) {                   // ファイル名付きでエラー表示
  perror(curFile);
  error(str);
}

void xOpen(char *fname) {                  // エラーチェック付きの fopen
  curFile = fname;
  if ((in = fopen(fname, "rb"))==NULL) {   // 入力ファイルオープン
    fError("can't open");
  }
}
  
void xSeek(int offset) {                   // エラーチェック付きの SEEK ルーチン
  if ((offset&1)!=0 || fseek(in, (long)offset, SEEK_SET)!=0)
    fError("file format");
}

void putW(int x) {                          // 1ワード出力ルーチン
  putB(x>>8);
  putB(x);
}

int getW() {                                // 1ワード入力ルーチン
  int x1 = getB();
  int x2 = getB();
  if (x1==EOF || x2==EOF) fError("undexpected EFO");
  return (x1 << 8) | x2;
}

void writeHdr() {                           // ヘッダ書き出しルーチン
  putW(MAGIC);                             //   マジックナンバー
  putW(textSize);                           //   TEXTサイズ
  putW(dataSize);                           //   DATAサイズ
  putW(bssSize);                            //   BSS サイズ
  putW(symSize);                            //   SYMSサイズ
  putW(0);                                  //   ENTRY
  putW(trSize);                             //   Trサイズ
  putW(drSize);                             //   Drサイズ
}

void readHdr() {                            // ヘッダ読込みルーチン
  if (getW()!=MAGIC)                        //   マジックナンバー
    fError("扱えないファイル");
  cTextSize=getW();                         //   TEXTサイズ
  cDataSize=getW();                         //   DATAサイズ
  cBssSize=getW();                          //   BSS サイズ
  cSymSize=getW();                          //   SYMSサイズ
  getW();                                   //   ENTRY
  cTrSize=getW();                           //   Trサイズ
  cDrSize=getW();                           //   Drサイズ
}

/* 文字列表 */
#define STR_SIZ  16000                      // 文字列表の大きさ(<=16kB)

char strTbl[STR_SIZ];                       // 文字列表
int  strIdx = 0;                            // 表のどこまで使用したか

int strLen(int n) {                         // 文字列表中の文字列(n)の長さ
  int i = n;
  while(strTbl[i]!='\0')
    i = i + 1;
  return i - n + 1;                         // '\0' も数えた値を返す
}

boolean cmpStr(int n, int m) {              // 文字列表[n]〜と[m]〜 を比較する
  for (int i=0; ; i=i+1) {
    char t = strTbl[n+i];
    char s = strTbl[m+i];
    if (t!=s) return false;                 //   異なる
    if (t=='\0') break;                     //   同じ
  }
  return true;
}

void putStr(FILE* fp,int n) {               // 文字列表の文字列[n]を表示する
  if (n>0x3fff || n>=strIdx) error("putStr:バグ");
  while (strTbl[n]!='\0') {
    putc(strTbl[n],fp);
    n = n + 1;
  }
}

void readStrTbl(int offs) {                 // 文字列表の読み込み
  xSeek(offs);                              // 文字列表の位置に移動
  int c;
  while ((c=getB())!=EOF) {                 // EOFになるまで読み込む
    if (strIdx>=STR_SIZ) tblError("文字列表がパンクした");
    strTbl[strIdx] = c;
    strIdx = strIdx + 1;
  }
}

void writeStrTbl() {                        // 文字列表をファイルへ出力
  for (int i=0; i<strIdx; i=i+1) {          // 全ての文字について
    putB(strTbl[i]);                        //   出力する
  }
}

/* 名前表 */
#define SYM_SIZ  3000                       // 名前表の大きさ (<=16kエントリ)

struct SymTbl {                             // 名前表の型定義
  int strx;                                 // 文字列表の idx (14bitが有効)
  int type;                                 // type の意味は下に #define
  int val;                                  // 名前の値
};

#define SYMUNDF 0                           // 未定義ラベル
#define SYMTEXT 1                           // TEXTのラベル
#define SYMDATA 2                           // DATAのラベル
#define SYMBSS  3                           // BSSのラベル
#define SYMPTR  4                           // 表の他要素へのポインタ

struct SymTbl symTbl[SYM_SIZ];              // 名前表本体の定義
int symIdx = 0;                             // 表のどこまで使用したか

void readSymTbl(int offs, int sSize) {      // 名前表の読み込み
  xSeek(offs);                              // 名前表の位置に移動
  for (int i=0; i<sSize; i=i+4) {           // ファイルの名前表について
    int strx = getW();
    int type = (strx >> 14) & 0x3;          // 名前の型を分離
    strx = strIdx + (strx & 0x3fff);        // 名前のインデクスを分離
    int val  = getW();                      // 名前の値はセグメントの
    if (type==SYMTEXT)                      //   ロードアドレスにより変化する
      val = val + textBase;                 //     TEXTセグメントの場合
    else if (type==SYMDATA)                 //     DATAセグメントの場合
      val = val + dataBase;                 //     BSSセグメントの場合はサイズ
    if (symIdx>=SYM_SIZ) tblError("名前表がパンクした");
    symTbl[symIdx].strx = strx;             // 名前の綴
    symTbl[symIdx].type = type;             // 名前の型
    symTbl[symIdx].val  = val;              // 名前の値
    symIdx = symIdx + 1;
  }
}

void mergeStrTbl(int symIdxB,int strIdxB) { // 文字列表に新しく追加した綴りに
                                            //   重複があれば統合する
  for (int i=symIdxB; i<symIdx; i=i+1) {   // 追加された文字列について
    int idxI = symTbl[i].strx;
    if (idxI < strIdxB) continue;           //  既に統合済みなら処理しない
    for (int j=0; j<symIdxB; j=j+1) {      //  以前からある文字列と比較
      int idxJ = symTbl[j].strx;
      if (cmpStr(idxI, idxJ)) {             //  同じ綴が見つかったら
	int len=strLen(idxI);
	for (int k=i; k<symIdx; k=k+1) {    //  名前表の残り部分について
	  int idxK = symTbl[k].strx;
	  if (idxK == idxI)                 //   同じ文字列は
	    symTbl[k].strx = idxJ;          //     以前からある方を使用する
	  else if (idxK > idxI)             //   前につめる部分にある文字列は
	    symTbl[k].strx = idxK - len;    //     位置調整
	}
	for (int k=idxI; k<strIdx-len; k=k+1)//  文字列表から統合した綴り削除
	  strTbl[k] = strTbl[k+len];        //     文字列を前につめる
	strIdx = strIdx - len;              //   文字列表を縮小
	break;
      }
    }
  }
}

void mergeSymTbl() {                        // 名前の結合を行う
  for (int i=0; i<symIdx; i=i+1) {          // 全ての名前について
    int typeI = symTbl[i].type;
    if (strTbl[symTbl[i].strx]=='.')        // ローカルは無視する
      continue;
    for (int j=0; j<i; j=j+1) {
      int typeJ = symTbl[j].type;           // PTR以外で同じ綴りを探す
      if (typeJ!=SYMPTR && cmpStr(symTbl[i].strx,symTbl[j].strx)) {
	if (typeJ==SYMUNDF && typeI!=SYMUNDF) {        // 後ろ(i)に統合
	  symTbl[j].type = SYMPTR;
	  symTbl[j].val  = i;
	} else if (typeJ!=SYMUNDF && typeI==SYMUNDF) { // 前(j)に統合
	  symTbl[i].type = SYMPTR;
	  symTbl[i].val  = j;
	} else if (typeJ==SYMUNDF && typeI==SYMUNDF) { // 前(j)に統合
	  symTbl[i].type = SYMPTR;
	  symTbl[i].val  = j;
	} else if(typeJ==SYMBSS  && typeI==SYMDATA) {  // BSSとDATAはDATAに統合
	  bssSize = bssSize - symTbl[j].val;
	  symTbl[j].type = SYMPTR;
	  symTbl[j].val  = i;
	} else if(typeJ==SYMDATA  && typeI==SYMBSS) { // DATAとBSSもDATAに統合
	  bssSize = bssSize - symTbl[i].val;
	  symTbl[i].type = SYMPTR;
	  symTbl[i].val  = j;
	} else if (typeJ==SYMBSS && typeI==SYMBSS) {  // BSS同士は
	  int valJ = symTbl[j].val;
	  int valI = symTbl[i].val;
	  if (valJ<valI) {                            //   サイズの大きい方に
	    bssSize = bssSize - valJ;                 //      統合する
	    symTbl[j].type = SYMPTR;
	    symTbl[j].val  = i;
	  } else {
	    bssSize = bssSize - valI;
	    symTbl[i].type = SYMPTR;
	    symTbl[i].val  = j;
	  }
	} else {
	  putStr(stderr,symTbl[i].strx);
	  error(":ラベルの二重定義");
	}
	symSize = symSize - 4;                        // 1項目4バイト減少
	break;
      }
    }
  }
}

void writeSymTbl() {                        // 名前表をファイルへ出力
  for (int i=0; i<symIdx; i=i+1) {
    putW((symTbl[i].type<<14) | symTbl[i].strx);
    putW(symTbl[i].val);
  }
}

void printSymType(int type) {               // 名前の種類を印刷
  if (type==SYMTEXT) printf("TEXT");        //   = 1
  else if (type==SYMDATA) printf("DATA");   //   = 2
  else if (type==SYMBSS)  printf("BSS");    //   = 3
  else if (type==SYMUNDF) printf("UNDF");   //   = 0
  else error("printSymType:バグ");
}

void printSymTbl() {                        // 名前表をリストへ出力
  printf("*** 名前表 ***\n");
  printf("No.\tName\tType\tValue\n");
  for (int i=0; i<symIdx; i=i+1) {
    int strx = symTbl[i].strx;
    int type = symTbl[i].type;
    int val  = symTbl[i].val;

    printf("%d\t",i);
    putStr(stdout,strx);
    printf("\t");
    printSymType(type);
    printf("\t%04x\n", val&0xffff);
  }
}

/* 再配置表 */
#define REL_SIZ  4500                       // 再配置表の大きさ

struct Reloc {                              // 再配置表
  int addr;                                 // ポインタのセグメント内 Offs
  int symx;                                 // シンボルテーブル上の番号
};

struct Reloc relTbl[REL_SIZ];               // 再配置表の定義
int relIdx;                                 // 表のどこまで使用したか

void readRelTbl(int offs, int relSize, int symBase, int textBase){
  xSeek(offs);
  for (int i=0; i<relSize; i=i+4) {         // 再配置表の1エントリは4バイト
    int addr = getW() + textBase;           // 再配置アドレス
    int symx = getW() & 0x3fff;             // 名前表のエントリ番号
    symx = symx + symBase / 4;              //   名前表の1エントリは4バイト
    while (symTbl[symx].type==SYMPTR)       // PTRならポインターをたぐる
      symx = symTbl[symx].val;              //   PTRを使用する再配置情報はない
    if (relIdx>=REL_SIZ) tblError("再配置表がパンクした");
    if ((addr&1)!=0) fError("再配置表に奇数アドレスがある");
    relTbl[relIdx].addr = addr;
    relTbl[relIdx].symx = symx;             // PTRではなく本体を指す
    relIdx = relIdx + 1;
  }
}

void packSymTbl()  {                        // 名前表の不要エントリーを削除
  int i = 0;
  while (i<symIdx) {                        // 全てのエントリーについて
    if (symTbl[i].type==SYMPTR) {           // PTRなら以下のように削除する
      for (int j=0; j<relIdx; j=j+1) {      //   再配置情報全てについて
	if (relTbl[j].symx>=i)              //     名前表の削除位置より後ろを
	  relTbl[j].symx=relTbl[j].symx-1;  //     参照しているインデクスを調整
      }
      for (int j=i; j<symIdx-1; j=j+1) {    //   名前表を前につめる
	symTbl[j].strx = symTbl[j+1].strx;
	symTbl[j].type = symTbl[j+1].type;
	symTbl[j].val  = symTbl[j+1].val;
      }
      symIdx = symIdx - 1;                  //   名前表を縮小する
    } else
      i = i + 1;                            // PTR以外なら進める
  }
}

void writeRelTbl() {                       // 再配置表をファイルへ出力
  for (int i=0; i<relIdx; i=i+1) {
    int addr = relTbl[i].addr;
    int symx = relTbl[i].symx;
    int type = symTbl[symx].type;
    putW(addr);
    putW((type<<14) | symx);
  }
}

void printRelTbl() {                       // 再配置表をリスト出力
  printf("*** 再配置表 ***\n");
  printf("Addr\tName\tType\tNo.\n");
  for (int i=0; i<relIdx; i=i+1) {
    int addr = relTbl[i].addr;
    int symx = relTbl[i].symx;
    int type = symTbl[symx].type;
    
    printf("%04x\t",addr);
    putStr(stdout,symTbl[symx].strx);
    printf("\t");
    printSymType(type);
    printf("\t%d\n", symx);
  }
  printf("\n");
}

/* 表の込み具合を確認する */
int maxStrIdx = 0;                         // 文字列表の最大値
int maxSymIdx = 0;                         // 名前表の最大値

void tblReport(void) {
  fprintf(stderr, "文字列表\t%5d/%5d\n", maxStrIdx, STR_SIZ);
  fprintf(stderr, "  名前表\t%5d/%5d\n", maxSymIdx, SYM_SIZ);
  fprintf(stderr, "再配置表\t%5d/%5d\n", relIdx, REL_SIZ);
}

/* 表がパンクしたときに使用する */
void tblError(char *str) {
  fprintf(stderr, "%s\n", str);
  tblReport();
  exit(1);
}

/* プログラムやデータをリロケートしながらコピーする */
void copyCode(int offs, int segSize, int segBase, int relBase) {
  xSeek(offs);
  int rel = relBase;
  for (int i=segBase; i<segBase+segSize; i=i+WORD) {
    int w = getW();
    if (rel<relIdx && relTbl[rel].addr==i) {  // ポインタのアドレスに達した
      int symx = relTbl[rel].symx;            // 名前表のインデクスに変換
      int type = symTbl[symx].type;
      if (type!=SYMUNDF && type!=SYMBSS) {    // UNDF と BSS は 0 のまま
	w = symTbl[symx].val;
	if (type==SYMDATA) w=w+textSize;      // データセグメントなら(一応)
      }                                       // 絶対番地を書き込んでおく
      rel = rel + 1;                          // 次のポインタに進む
    }
    putW(w);
  }
}

// 使い方表示関数
void usage(char *name) {
  fprintf(stderr,"使用方法 : %s [-h] [-v] <outfile> <objfile>...\n", name);
  fprintf(stderr, "    <objfile> (複数)から入力し\n");
  fprintf(stderr, "    <outfile> へ出力\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "    -h, -v  : このメッセージを表示\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "%s version %s\n", name, VER);
  fprintf(stderr, "(build date : %s)\n", DATE);
  fprintf(stderr, "\n");
}

// main 関数
int main(int argc, char **argv) {
  if (argc>1 &&
      (strcmp(argv[1],"-v")==0 ||              //  "-v", "-h" で、使い方と
       strcmp(argv[1],"-h")==0   ) ) {         //   バージョンを表示
    usage(argv[0]);
    exit(0);
  }

  if (argc<3) {
    usage(argv[0]);
    exit(0);
  }

  if ((out = fopen(argv[1],"wb"))==NULL) {    // 出力ファイルオープン
    perror(argv[1]);
    exit(1);
  }

  /* 入力ファイルのシンボルテーブルを読み込んで統合する */
  textBase = dataBase = bssBase = 0;
  trSize = drSize = symSize = 0;
  for (int i=2; i<argc; i=i+1) {
    xOpen(argv[i]);
    int newSymBase = symIdx;
    int newStrBase = strIdx;
    readHdr();
    //printf("%s:text=%04x,data=%04x,bss=%04x,Tr=%04x,Dr=%04x,Sym=%04x\n",
    //	   argv[i],cTextSize,cDataSize,cBssSize,cTrSize,cDrSize,cSymSize);
    readSymTbl(HDRSIZ+cTextSize+cDataSize+cTrSize+cDrSize,cSymSize);
    readStrTbl(HDRSIZ+cTextSize+cDataSize+cTrSize+cDrSize+cSymSize);
    if (symIdx > maxSymIdx) maxSymIdx = symIdx;
    if (strIdx > maxStrIdx) maxStrIdx = strIdx;

    mergeStrTbl(newSymBase, newStrBase);

    textBase = textBase + cTextSize;
    dataBase = dataBase + cDataSize;
    bssBase  = bssBase  + cBssSize;

    trSize   = trSize   + cTrSize;
    drSize   = drSize   + cDrSize;
    symSize  = symSize  + cSymSize;

    fclose(in);
  }
  textSize = textBase;
  dataSize = dataBase;
  bssSize  = bssBase;

  mergeSymTbl();                                // シンボルテーブルの統合をする
                                                // bssSize, symSize も再計算する
  writeHdr();                                   // ヘッダを出力する

  /* テキストセグメントを入力して結合後出力する */
  int symBase = 0;
  textBase = relIdx = 0;
  for (int i=2; i<argc; i=i+1) {
    xOpen(argv[i]);
    readHdr();
    int relBase = relIdx;
    readRelTbl(HDRSIZ+cTextSize+cDataSize,cTrSize,symBase,textBase);
    copyCode(HDRSIZ,cTextSize,textBase,relBase);  // テキストをコピー

    textBase = textBase + cTextSize;
    symBase  = symBase  + cSymSize;
    fclose(in);
  }

  /* データセグメントを入力して結合後出力する */
  dataBase = symBase = 0;
  for (int i=2; i<argc; i=i+1) {
    xOpen(argv[i]);
    readHdr();
    int relBase = relIdx;
    readRelTbl(HDRSIZ+cTextSize+cDataSize+cTrSize,cDrSize,symBase,dataBase);
    copyCode(HDRSIZ+cTextSize,cDataSize,dataBase,relBase);   // データをコピー

    dataBase = dataBase + cDataSize;
    symBase  = symBase  + cSymSize;
    fclose(in);
  }

  packSymTbl();                            // 名前表から結合した残骸を削除

  writeRelTbl();                           // 再配置表を出力する
  printRelTbl();                           // 再配置表をリスト出力する
  writeSymTbl();                           // 名前表を出力する
  printSymTbl();                           // 名前表をリスト出力する
  writeStrTbl();                           // 文字列表を出力する

  fclose(out);
  //tblReport();                           // 各表の込み具合を確認する
  exit(0);
}
