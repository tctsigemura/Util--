/*
 * TaC-OS Source Code
 *    Tokuyama kousen Advanced educational Computer.
 *
 * Copyright (C) 2011 - 2019 by
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

// objexe.c : OBJEXE--
// リロケータブルファイル（.oファイル）を入力し、TacOSの
// 実行可能プログラム（.exeファイル）を出力する

/*
 *
 * 2022.06.29  : EXEファイルをページングに対応
 * 2017.01.1i  : IOPR を -P オプションに変更
 * 2017.01.11  : 実行モードの種類を変更（KERN 廃止、IOPR 追加）
 * 2016.12/28  : EXEファイルのマジック番号を、コマンドラインから与えられた
 *               モードに変更できるように修正
 * 2016.01.07  : Util-- に収録（統合）
 * 2015.06.10  : __endの定義を追加
 *               (__endは元々再配置対象になっている、再配置表に登録不要）
 * 2015.06.04  : EXEファイルのヘッダに、コマンドラインから与えられた
 *               スタックサイズを書き加えるように変更
 * 2015.05.14  : TaCでは1Word=2Byteのため、再配置情報のサイズをrelIdx * 2に変更
 * 2014.12.09  : _endの決定を削除、EXEファイルフォーマットの出力に変更、
 *               objexe使用法変更
 * 2014.12.03  : objbin.cをベースに開発開始
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define boolean int
#define true     1
#define false    0
#define HDRSIZ   16                           // .oファイルのヘッダは16byte
#define MAGIC    0x0107                       // .oファイルのマジック番号
#define WORD     2                            // 1ワード2byte

int textBase;                                 // 入力ファイルの
int dataBase;                                 // 各セグメントの
int bssBase;                                  // ロードアドレス

int textSize;                                 // 入力ファイルのTEXTサイズ
int dataSize;                                 // 入力ファイルのDATAサイズ
int bssSize;                                  // 入力ファイルのBSS サイズ
int symSize;                                  // 入力ファイルのSYMSサイズ
int trSize;                                   // 入力ファイルのTr  サイズ
int drSize;                                   // 入力ファイルのDr  サイズ

/*
//EXEファイルのヘッダ
struct Exe_head {
  int magic;                                  // マジック番号
  int text;                                   // Textサイズ
  int data;                                   // Dataサイズ
  int bss;                                    // BSSサイズ
  int rel;                                    // 再配置情報のサイズ
  int stkSiz;                                 // ユーザモード用スタックサイズ
};
*/

void error(char *str) {
  fprintf(stderr, "%s\n", str);
  exit(1);
}

//----------------------------- ファイル関係---------------------------------
FILE* out;                                    // 出力ファイル
FILE* in;                                     // 入力ファイル
char *fileName;                               // 入力ファイルの名前

#define getB()   fgetc(in)
#define putB(c)  fputc(c,out);

void fError(char *str) {                      // 入力ファイルでエラー発生時使用
  perror(fileName);
  error(str);
}

void xOpen(char *fname) {                     // 入力ファイルの open
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

// 入力ファイルのヘッダーを読む
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

//------------------------------- 文字列表----------------------------------- 
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

//-------------------------------- 名前表------------------------------------
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

void readSymTbl() {                           // 名前表の読み込み
  int lc_b = bssBase;                         // BSSのロケーションカウンタ
  xSeek(HDRSIZ+textSize+dataSize+trSize+drSize); // 記号表の位置に移動
  for (int i=0; i<symSize; i=i+2*WORD) {      // ファイルの全記号について
    int strx = getW();
    int type = (strx >> 14) & 0x3;            // type を分離
    strx = strx & 0x3fff;                     // 名前表のインデクスを分離
    int val  = getW();                        // 名前の値はセグメントのロード
    //--2015.06.10追加  __endの定義---------------------------------------
    if (sameStr("__end",strx)) {              // "__end"ならここで定義する
      if (type!=SYMUNDF)
        error("__end is already defined");    // "__end"は未定義であること
      type = SYMBSS;
      val  = bssBase + bssSize;               // BSSセグメントの最後
    //-------------------------------------------------------------------
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

//------------------------------- 再配置表----------------------------------
#define REL_SIZ  4500                         // 再配置表の大きさ

struct Reloc {                                // 再配置表
  int addr;                                   // ポインタのセグメント内 Offs
  int symx;                                   // シンボルテーブル上の番号
};

struct Reloc relTbl[REL_SIZ];                 // 再配置表の定義
int relIdx;                                   // 表のどこまで使用したか

void readRelTbl() {                           // 再配置表を読み込む
  xSeek(HDRSIZ+textSize+dataSize);
  int base=textBase;
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
    base=dataBase;
    size=drSize;
  }
}

//--------------------------------ファイル出力部------------------------------
#define PAGESIZ 256                                //ページサイズ

// コードのコピー
int copyCode(int base , int size,int rel) {        //リロケートしながらコピー
  fseek(out,(long)base+PAGESIZ,SEEK_SET);          //出力をページ境界へシーク
  for (int i=0; i<size; i=i+WORD) {
    int w = getW();
    if (rel<relIdx && relTbl[rel].addr==i+base) {  // ポインタのアドレスに達した
      int symx = relTbl[rel].symx;                 // 名前表のインデクスに変換
      w = symTbl[symx].val;
      rel = rel + 1;                               // 次のポインタに進む
    }
    putW(w);
  }
  return rel;                                      //終了時のrelの値を返す
}

// 使い方表示関数
void usage(char *name) {
  fprintf(stderr, "使用方法 : %s [-Phv] <exefile> <objfile>\n", name);
  fprintf(stderr, "    <objfile> 単一の .o ファイルから入力し\n");
  fprintf(stderr, "    <exefile> へ出力\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "    -P      : I/O 特権モードの exe ファイルを作る\n");
  fprintf(stderr, "    -h, -v  : このメッセージを表示\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "%s version %s\n", name, VER);
  fprintf(stderr, "(build date : %s)\n", DATE);
  fprintf(stderr, "\n");
}

// main 関数
int main(int argc, char **argv) {
  if (argc>1 && (strcmp(argv[1],"-v")==0 ||   // "-v",
		 strcmp(argv[1],"-h")==0)) {              // "-h" で、
    usage(argv[0]);                           // 使い方とバージョンを表示
    exit(0);
  }

  int i = 1;                                  // コマンド行引数の位置
  int magic = 0x108;                          // exe ファイルのマジック番号

  if (argc!=3) {
    usage(argv[0]);                           // 使い方とバージョンを表示
    exit(0);
  }

  textBase = 0x0000;                          // 仮のロードアドレスを0x0000に

  if ((out = fopen(argv[i],"wb"))==NULL) {    // 出力ファイルオープン
    perror(argv[i]);
    exit(1);
  }

  xOpen(argv[i+1]);                           // 入力ファイルオープン
  readHdr();                                  // ヘッダを読み込む
  dataBase = (textBase + textSize + PAGESIZ - 1)
              & ~(PAGESIZ - 1);               // DATAセグメントのアドレスを決め
  bssBase  = dataBase + dataSize;             // BSSセグメントのアドレスを決め
  readStrTbl();                               // 文字列表を読み込む
  fclose(in);                                 // EOFに達したオープンしなおし

  xOpen(argv[i+1]);                           // 入力ファイルオープン
  readSymTbl();                               // 名前表を読み込む
  readRelTbl();                               // 再配置表を読み込む

  //EXEファイル出力部

  // ヘッダ出力
  putW(magic);                                // マジック番号を出力
  putW(textSize);                             // Textサイズ (ヘッダ情報のまま)
  putW((dataSize + bssSize + PAGESIZ - 1)
              & ~(PAGESIZ - 1));              //Dataとbssのサイズの合計をページサイズで切り上げ

  // プログラム本体出力
  xSeek(HDRSIZ);                              //入力ファイルをTEXTセグメントへ
  int rel = copyCode(textBase, textSize, 0);  //出力ファイルにTEXTセグメントをコピー
  copyCode(dataBase, dataSize, rel);          //出力ファイルにDATAセグメントをコピー
  if(dataSize%PAGESIZ!=0){
    fseek(out,((2*PAGESIZ+dataBase+dataSize)
                & ~(PAGESIZ-1))-1,SEEK_SET);  //出力をdataとbssの次のページ境界の一つ前にシーク
    putB(0);                                  //出力に0を書き込み
  }

  fclose(in);
  fclose(out);
  exit(0);
}
