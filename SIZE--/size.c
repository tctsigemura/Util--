/*
 * TaC size utility Source Code
 *    Tokuyama kousen Educational Computer 16bit Ver.
 *
 * Copyright (C) 2009-2017 by
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
 * size.c : SIZE-- 本体
 * リロケータブル形式から text, data, bss サイズを調べて表示する
 *
 * 2017.01.11          : .exeファイルの実行モードによるマジックナンバーに対応
 * 2016.10.09  v3.0.0  : バージョン番号はUtil--全体で同じものを使うようにする
 * 2016.10.09  v2.1.0  : .exe ファイルも扱えるように変更
 * 2012.09.12          : ファイルがオープンできない時のエラー処理追加
 * 2012.08.06  v2.0.0  : TaC-CPU V2 対応
 * 2010.07.20          : Subversion による管理を開始
 * 2009.04.20          : 作成 by T.Shigemura
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int magic;                                  /* マジックナンバー             */
int textSize;                               /* TEXT セグメントサイズ        */
int dataSize;                               /* DATA セグメントサイズ        */
int bssSize;                                /* BSS セグメントサイズ         */
int symSize;                                /* 名前表サイズ                 */
int trSize;                                 /* TEXT 再配置情報サイズ        */
int drSize;                                 /* DATA 再配置情報サイズ        */

FILE *in;                                   /* 入力ファイル                 */

int getW() {                                /* 1ワード入力ルーチン          */
  int x1 = getc(in);
  int x2 = getc(in);
  if (x1==EOF || x2==EOF) {
    fprintf(stderr, "bad file format\n");
    exit(1);
  }
  return (x1 << 8) | x2;
}

void readHdr() {
  magic=getW();                            /* マジックナンバー              */
  textSize=getW();                         /* TEXTサイズ                    */
  dataSize=getW();                         /* DATAサイズ                    */
  bssSize=getW();                          /* BSS サイズ                    */
  symSize=getW();                          /* SYMSサイズ                    */
  getW();                                  /* ENTRY                         */
  trSize=getW();                           /* Trサイズ                      */
  drSize=getW();                           /* Drサイズ                      */
}

// 使い方表示関数
void usage(char *name) {
  fprintf(stderr, "使用方法 : %s <file name>\n", name);
  fprintf(stderr, "    <file name> : TaC リロケータブル形式のファイル(.o)\n");
  fprintf(stderr, "                  または、TaC 実行形式のファイル(.exe)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "    -h, -v  : このメッセージを表示\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "%s version %s\n", name, VER);
  fprintf(stderr, "(build date : %s)\n", DATE);
  fprintf(stderr, "\n");
}

// main 関数
int main(int argc, char*argv[]) {
  if (argc!=2 || (argc>1 &&
      (strcmp(argv[1],"-v")==0 ||              //  "-v", "-h" で、使い方と
       strcmp(argv[1],"-h")==0   ))) {         //   バージョンを表示) {
    usage(argv[0]);
    exit(1);
  }

  if ((in = fopen(argv[1], "rb"))==NULL) {
    perror(argv[1]);
    exit(1);
  }

  readHdr();

  if (magic!=0x0107 && magic!=0x0108 && magic!=0x0109) {
    fprintf(stderr, "マジックナンバーが 0x0107,0x0108,0x0109 以外\n");
    exit(1);
  }

  printf("text\tdata\tbss\tdec\tfilename\n");
  printf("%5d\t%5d\t%5d\t%5d\t%s\n",
	 textSize,dataSize,bssSize,
	 textSize+dataSize+bssSize,
	 argv[1]);
  printf("(%04x)\t(%04x)\t(%04x)\t(%04x)\t(hex)\n",
	 textSize,dataSize,bssSize,
	 textSize+dataSize+bssSize
	 );
  return 0;
}
