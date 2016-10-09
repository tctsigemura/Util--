/*
 * TaC objbin utility Source Code
 *    Tokuyama kousen Educational Computer 16bit Ver.
 *
 * Copyright (C) 2008-2016 by
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
 * objbin.c : OBJBIN-- 本体
 * リロケータブルオブジェクトをメモリイメージに変換する
 *
 * 2016.10.09 v.3.0.0   : バージョン番号等を表示するように改良
 * 2016.01.07           : C-Lang の -Wlogical-op-parentheses 警告に対応
 * 2012.09.25           : 標準出力にメモリマップを書き出す
 * 2012.08.05  v.2.0.0  : TaC-CPU V2 に対応
 * 2010.07.20           : Subversion による管理を開始
 * 2010.04.09  v.0.6.3  : fprintf の警告を回避 (Ubuntu 対応)
 * 2009.07.10  v.0.6.2  : BSSアドレスを明示した場合BSS部分をファイルに出力しない
 * 2009.04.21  v.0.6.1  : fopen のモードに "b" を追加
 * 2009.04.10  v.0.6    : BSSのアドレスを明示できるように変更
 * 2008.08.25  v.0.5    : "__end"シンボルの定義機能追加
 * 2008.07.18  v.0.0    : 初期バージョン
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define boolean int      // boolean 型がC言語でも使えるとよいのに ...
#define true     1
#define false    0
#define HDRSIZ   16                          // .o 形式のヘッダは16バイト
#define MAGIC    0x0107                      // .o 形式のマジック番号
#define WORD     2                           // 1ワード2バイト

int textBase;                                // 現在の入力ファイルの
int dataBase;                                // 各セグメントの
int bssBase;                                 // ロードアドレス

int textSize;                                // 入力ファイルのTEXTサイズ
int dataSize;                                // 入力ファイルのDATAサイズ
int bssSize;                                 // 入力ファイルのBSS サイズ
int symSize;                                 // 入力ファイルのSYMSサイズ
int trSize;                                  // 入力ファイルのTr  サイズ
int drSize;                                  // 入力ファイルのDr  サイズ

void error(char *str) {
  fprintf(stderr, "%s\n", str);
  exit(1);
}

/* ファイル関係 */
FILE* out;                                   // 出力ファイル
FILE* in;                                    // 入力ファイル
char *fileName;                              // 入力ファイルの名前

#define getB()   fgetc(in)
#define putB(c)  fputc(c,out);

void fError(char *str) {                     // 入力ファイルでエラー発生時使用
  perror(fileName);
  error(str);
}

void xOpen(char *fname) {                    // 入力ファイルの open
  fileName = fname;
  if ((in = fopen(fname, "rb"))==NULL)
    fError("can't open");
}

void xSeek(int offset) {                      // 入力ファイルの seek
  if ((offset&1)!=0 || fseek(in, (long)offset, SEEK_SET)!=0)
    fError("file format");
}

int getW() {                                  // 1ワード入力ルーチン
  int x1 = getB();
  int x2 = getB();
  if (x1==EOF || x2==EOF) fError("unexpected EOF");
  return (x1 << 8) | x2;
}

void putW(int x) {                            // 1ワード出力ルーチン
  putB(x>>8);
  putB(x);
}

/* 入力ファイルのヘッダーを読む */
void readHdr() {
  if (getW()!=MAGIC)                          // マジックナンバー
    fError("Bad magic number");
  textSize=getW();                            // TEXTサイズ
  dataSize=getW();                            // DATAサイズ
  bssSize=getW();                             // BSS サイズ
  symSize=getW();                             // SYMSサイズ
  getW();                                     // ENTRY
  trSize=getW();                              // Trサイズ
  drSize=getW();                              // Drサイズ
}

/* 文字列表 */
#define STR_SIZ  15000                        // 文字列表の大きさ(<=16kB)

char strTbl[STR_SIZ];                         // 文字列表
int  strIdx = 0;                              // 表のどこまで使用したか

void putStr(FILE* fp,int n) {                 // 文字列表の文字列を出力
  while (strTbl[n]!='\0') {
    putc(strTbl[n],fp);
    n = n + 1;
  }
}

boolean sameStr(char* s, int n) {             // 文字列表と文字列の比較
  for (int i=0; s[i]==strTbl[n+i]; i=i+1)
    if (s[i]=='\0') return true;
  return false;
}

void readStrTbl() {                           // 文字列表の読み込み
  xSeek(HDRSIZ+textSize+dataSize+trSize+drSize+symSize);
  int c;
  while ((c=getB())!=EOF) {
    if (strIdx>=STR_SIZ) error("strTbl is full");
    strTbl[strIdx] = c;
    strIdx = strIdx + 1;
  }
}

/* 名前表 */
#define SYM_SIZ  3000                         // 名前表の大きさ

struct SymTbl {                               // 名前表
  int strx;                                   // 文字列表の idx
  int type;                                   // type の意味は下に #define
  int val;                                    // 名前の値
};

#define SYMUNDF 0                             // 未定義ラベル
#define SYMTEXT 1                             // TEXTのラベル
#define SYMDATA 2                             // DATAのラベル
#define SYMBSS  3                             // BSSのラベル

struct SymTbl symTbl[SYM_SIZ];                // 名前表の定義
int symIdx = 0;                               // 表のどこまで使用したか

void putFname(FILE *stderr) {                 // 現在処理中のファイル名を印刷
  for (int j=symIdx-1; j>=0; j=j-1) {         // 新しいシンボルから順に
    int sx = symTbl[j].strx;                  //   ファイル名を探す
    if (strTbl[sx]=='@') {
      fprintf(stderr,"[");                    // ファイル名を表示
      putStr(stderr,sx+1);
      fprintf(stderr,"]");
      break;
    }
  }
}

void readSymTbl() {                           // 記号表の読み込み
  int lc_b = bssBase;                         // BBSのロケーションカウンタ
  xSeek(HDRSIZ+textSize+dataSize+trSize+drSize); // 記号表の位置に移動
  for (int i=0; i<symSize; i=i+2*WORD) {      // ファイルの全記号について
    int strx = getW();
    int type = (strx >> 14) & 0x3;            // type を分離
    strx = strx & 0x3fff;                     // 名前表のインデクスを分離
    int val  = getW();                        // 記号の値はセグメントのロード
    if (sameStr("__end",strx)) {              // "__end"ならここで定義する
      if (type!=SYMUNDF)
	error("__end is already defined");    // "__end"は未定義であること
      type = SYMBSS;
      val  = bssBase + bssSize;               // BSSセグメントの最後
    } else if (type==SYMTEXT) {               // アドレスにより変化する
      val = val + textBase;                   // TEXTセグメントの場合
    } else if (type==SYMDATA) {               // DATAセグメントの場合
      val = val + dataBase;
    } else if (type==SYMBSS) {                // BSSセグメントの場合
      int lc_bak = lc_b;                      // ここで領域を割り当てる
      lc_b = lc_b + val;
      val = lc_bak;
    } else /* if (type==UNDEF) */ {           // UNDFのシンボルは許されない
      putStr(stderr,strx);                    // シンボルの綴りを表示
      putFname(stderr);                       // @で始まるファイル名を表示
      error(":undefined symbol");             // エラーで終了
    }
    if (symIdx>=SYM_SIZ) error("symTbl is full");
    symTbl[symIdx].strx = strx;
    symTbl[symIdx].type = type;
    symTbl[symIdx].val  = val;                // 配置後のアドレスを記録
    printf("%04x\t",val);
    if (type==SYMTEXT)      printf("TEXT\t");
    else if (type==SYMDATA) printf("DATA\t");
    else if (type==SYMBSS)  printf("BSS\t");
    else error("bug?");
    putStr(stdout,strx);
    printf("\n");
    symIdx = symIdx + 1;
  }
}

/* 再配置表 */
#define REL_SIZ  4500                         // 再配置表の大きさ

struct Reloc {                                // 再配置表
  int addr;                                   // ポインタのセグメント内 Offs
  int symx;                                   // シンボルテーブル上の番号
};

struct Reloc relTbl[REL_SIZ];                 // 再配置表の定義
int relIdx;                                   // 表のどこまで使用したか

void readRelTbl() {                           // 再配置表を読み込む
  xSeek(HDRSIZ+textSize+dataSize);
  int base=0;
  int size=trSize;
  for (int j=0; j<2; j=j+1) {                 // Tr, Dr の２つについて
    for (int i=0; i<size; i=i+2*WORD) {       // 1エントリ2ワード
      int addr = getW() + base;
      int symx = getW() & 0x3fff;
      if (relIdx>=REL_SIZ) error("relTbl is full");
      relTbl[relIdx].addr = addr;
      relTbl[relIdx].symx = symx;
      relIdx = relIdx + 1;
    }
    base=textSize;
    size=drSize;
  }
}

/* コードのコピー */
void copyCode() {          // プログラムとデータをリロケートしながらコピーする
  xSeek(HDRSIZ);
  int rel = 0;
  for (int i=0; i<textSize+dataSize; i=i+WORD) {
    int w = getW();
    if (rel<relIdx && relTbl[rel].addr==i) {  // ポインタのアドレスに達した
      int symx = relTbl[rel].symx;            // 名前表のインデクスに変換
      w = symTbl[symx].val;
      rel = rel + 1;                          // 次のポインタに進む
    }
    putW(w);
  }
}

/* コマンドラインオプションなど */
void herror(char *hstr) {
  fprintf(stderr, "%s", hstr);
  error(":Hex number format error");
}

boolean isHex(char c) {
  return ('0'<=c && c<='9')||('a'<=c && c<='f')||('A'<=c && c<='F');
}

int hToi(char c) {
  if ('0'<=c && c<='9') return c - '0';
  if ('a'<=c && c<='f') return c - 'a' + 10;
  if ('A'<=c && c<='F') return c - 'A' + 10;
  return 0;
}

int hToInt(char *hstr) {
  if (strlen(hstr)<3) herror(hstr);

  if (hstr[0]!='0' || (hstr[1]!='x' && hstr[1]!='X') || !isHex(hstr[2]))
    herror(hstr);

  int i = 3;
  int x = hToi(hstr[2]);
  while (isHex(hstr[i])) {
    x = x * 16 + hToi(hstr[i]);
    i = i + 1;
  }
  if (hstr[i]!='\0') herror(hstr);

  return x;
}

// 使い方表示関数
void usage(char *name) {
  fprintf(stderr, "使用方法 : %s 0xTTTT <outfile> <objfile> [0xBBBB]\n", name);
  fprintf(stderr, "    0xTTTT : Text , Data segment address\n");
  fprintf(stderr, "    0xBBBB : BSS segment address\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "    -h, -v  : このメッセージを表示\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "%s version %s\n", name, VER);
  fprintf(stderr, "(build date : %s)\n", DATE);
  fprintf(stderr, "\n");
}

// main 関数
int main(int argc, char **argv) {
  if ((argc!=4 && argc!=5) || (argc>1 &&
      (strcmp(argv[1],"-v")==0 ||              //  "-v", "-h" で、使い方と
       strcmp(argv[1],"-h")==0   ))) {         //   バージョンを表示
    usage(argv[0]);
    exit(0);
  }

  textBase = hToInt(argv[1]);

  if ((out = fopen(argv[2],"wb"))==NULL) {    // 出力ファイルオープン
    perror(argv[2]);
    exit(1);
  }

  xOpen(argv[3]);                             // 入力ファイルオープン
  readHdr();                                  // ヘッダを読み込む
  dataBase = textBase + textSize;             // 各セグメントのアドレスを決める
  if (argc==4)
    bssBase  = textBase + textSize + dataSize;
  else
    bssBase  = hToInt(argv[4]);
  readStrTbl();                               // 文字列表を読み込む
  fclose(in);                                 // EOFに達したオープンしなおし

  xOpen(argv[3]);                             // 入力ファイルオープン
  readSymTbl();                               // 記号表を読み込む
  readRelTbl();                               // 再配置表を読み込む

  if (argc==5) bssSize = 0;                   // BSSアドレス明示の場合
					      // ファイルにBSSを含めない
  putW(textBase);                             // ロードアドレス
  putW(textSize+dataSize+bssSize);            // ファイルサイズ
  copyCode();                                 // TEXT、DATAを出力

  for (int i=0; i<bssSize; i=i+1)             // BSS 領域分の0を出力
    putB(0);

  fclose(in);
  fclose(out);
  exit(0);
}
